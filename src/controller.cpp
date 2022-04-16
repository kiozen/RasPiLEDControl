/**********************************************************************************************
    Copyright (C) 2022 Oliver Eichler <oliver.eichler@gmx.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

**********************************************************************************************/
#include "controller.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "session.hpp"

constexpr const char* kSystemFile = "system.json";
constexpr const char* kConfigFile = "config.json";
constexpr const char* kConfigPath = "/home/pi/.config/led_control/";


#ifndef _MKSTR_1
#define _MKSTR_1(x)    #x
#define _MKSTR(x)      _MKSTR_1(x)
#endif

#define VER_STR       _MKSTR(VER_MAJOR) "." _MKSTR (VER_MINOR) "." _MKSTR (VER_STEP)
#define WHAT_STR      _MKSTR(APPLICATION_NAME) ", Version " VER_STR


Controller::Controller() : Log("ctrl")
{
    I(fmt::format("-------------- {} --------------", WHAT_STR));
    const std::filesystem::path path{kConfigPath};
    std::filesystem::create_directories(path);


    memset(&ledstring_, 0, sizeof (ledstring_));
    ledstring_.freq = kTargetFreq;
    ledstring_.dmanum = kDma;
    ledstring_.channel[0].gpionum = kGpioPin;
    ledstring_.channel[0].count = kLedCount;
    ledstring_.channel[0].invert = 0;
    ledstring_.channel[0].brightness = 100;
    ledstring_.channel[0].strip_type = kStripeType;

    try
    {
        std::filesystem::path path {kConfigPath};
        path /= kSystemFile;
        std::ifstream file(path);

        nlohmann::json cfg;
        file >> cfg;

        name_ = cfg.value("name", "");
        ledstring_.channel[0].count = cfg.value("led_count", kLedCount);
        ledstring_.channel[0].brightness = cfg.value("max_brightness", 100);
        fadeout_.SetNormalBrightness(ledstring_.channel[0].brightness);
    }
    catch(const nlohmann::json::exception& e)
    {
        E(fmt::format("Parsing system config failed: {}", e.what()));
    }
}

Controller::~Controller()
{
}

static bool restore_state_active = false;

void Controller::SaveState()
{
    if(restore_state_active)
    {
        return;
    }

    nlohmann::json cfg;
    cfg["light"] = light_.SaveState();
    cfg["alarm"] = alarm_.SaveState();

    std::filesystem::path path {kConfigPath};
    path /= kConfigFile;

    std::ofstream file(path);
    file << cfg;
    file.flush();
}

void Controller::RestoreState()
{
    restore_state_active = true;
    try
    {
        std::filesystem::path path {kConfigPath};
        path /= kConfigFile;
        std::ifstream file(path);

        nlohmann::json cfg;
        file >> cfg;

        light_.RestoreState(cfg.value("light", nlohmann::json()));
        alarm_.RestoreState(cfg.value("alarm", nlohmann::json()));

        SetPowerLight(cfg.value("light", nlohmann::json()).value("power", false));
        I("Restored all configurations");
    }
    catch(const nlohmann::json::exception& e)
    {
        E(fmt::format("Parsing config failed: {}", e.what()));
    }

    restore_state_active = false;
}


int Controller::exec()
{
    ws2811_return_t ret = WS2811_SUCCESS;

    sig_.async_wait([this](const asio::error_code& error, int signal_number) {
        OnSignal(error, signal_number);
    });

    if(!SetupUdp())
    {
        return -1;
    }

    if(!StartServer())
    {
        return -1;
    }

    if ((ret = ws2811_init(&ledstring_)) != WS2811_SUCCESS)
    {
        E(fmt::format("ws2811_init failed: {} ({})", ws2811_get_return_t_str(ret), ret));
        if(ret == WS2811_ERROR_MMAP)
        {
            E("Try to run the app as root.");
        }
        return ret;
    }

    RestoreState();

    D("*** start asio loop ***");

    io_.run();

    Clear();

    ws2811_fini(&ledstring_);
    return ret;
}

bool Controller::SetupUdp()
{
    udp_socket_.open(asio::ip::udp::v4());
    asio::socket_base::broadcast option(true);
    udp_socket_.set_option(option);
    udp_socket_.bind(asio::ip::udp::endpoint(asio::ip::address_v4::any(), kPort ));


    struct ifreq s;
    strcpy(s.ifr_name, "wlan0");
    if (0 == ioctl(udp_socket_.native_handle(), SIOCGIFHWADDR, &s))
    {
        const char* mac = s.ifr_addr.sa_data;
        mac_ = fmt::format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        D(fmt::format("MAC address is {}", mac_));
    }
    else
    {
        E(fmt::format("Failed to read MAC address from wlan0: {}", strerror(errno)));
    }

    udp_socket_.async_receive_from(asio::buffer(recv_buffer_),
                                   remote_endpoint_,
                                   [this](const asio::error_code& error, std::size_t size){
        OnReceiveUdp(error, size);
    });

    return true;
}

bool Controller::StartServer()
{
    session_ = std::unique_ptr<Session>(new Session(io_, *this));
    acceptor_.async_accept(session_->Socket(), [this](const asio::error_code& error){
        if(!error)
        {
            D("New connection");
            session_->Exec();
        }
        else
        {
            E("Failed");
        }
    });
    return true;
}


void Controller::OnSignal(const asio::error_code& error, int signal_number)
{
    if (!error)
    {
        std::cout << std::endl;
        I(fmt::format("Controller stopped with signal {}", signal_number));
        io_.stop();
    }
    else
    {
        sig_.async_wait([this](const asio::error_code& error, int signal_number) {
            OnSignal(error, signal_number);
        });
    }
}

void Controller::OnReceiveUdp(const asio::error_code& error, std::size_t size)
{
    if(!error && (size < recv_buffer_.size()))
    {
        recv_buffer_[size] = 0;
        try
        {
            const nlohmann::json& msg = nlohmann::json::parse(recv_buffer_);
            if(msg["cmd"] == "identify")
            {
                nlohmann::json resp;
                resp["rsp"] = "identify";
                resp["name"] = name_;
                resp["mac"] = mac_;

                std::size_t s = udp_socket_.send_to(asio::buffer(resp.dump()), remote_endpoint_);
                //D(fmt::format("{} {} {}", remote_endpoint_.address().to_string(), remote_endpoint_.port(), recv_buffer_.data()));
            }
        }
        catch(const std::exception& e)
        {
            E(fmt::format("Parsing message failed: {}", e.what()));
        }
    }
    else
    {
        E(fmt::format("On receive UDP failed: {}", error.message()));
    }

    udp_socket_.async_receive_from(asio::buffer(recv_buffer_),
                                   remote_endpoint_,
                                   [this](const asio::error_code& error, std::size_t size){
        OnReceiveUdp(error, size);
    });
}

ws2811_return_t Controller::Render(const std::vector<ws2811_led_t>& matrix)
{
    std::size_t size = std::min(matrix.size(), static_cast<std::size_t>(ledstring_.channel[0].count));
    memcpy(ledstring_.channel[0].leds, matrix.data(), size * sizeof(ws2811_led_t));

    ws2811_return_t ret = WS2811_SUCCESS;
    if ((ret = ws2811_render(&ledstring_)) != WS2811_SUCCESS)
    {
        E(fmt::format("ws2811_render failed: {} ({})", ws2811_get_return_t_str(ret), ret));
    }
    return ret;
}

ws2811_return_t Controller::Render(ws2811_led_t color)
{
    std::vector<ws2811_led_t> matrix(ledstring_.channel[0].count, color);
    return Render(matrix);
}

ws2811_return_t Controller::Clear()
{
    std::vector<ws2811_led_t> matrix(ledstring_.channel[0].count, 0x00000000);
    return Render(matrix);
}

void Controller::SetColorRgb(uint8_t red, uint8_t green, uint8_t blue)
{
    SetColor(red << 16 | green << 8 | blue);
}

std::tuple<uint8_t, uint8_t, uint8_t> Controller::GetColorRgb() const
{
    ws2811_led_t color = light_.GetColor();
    return {color >> 16 & 0xff, color >> 8 & 0x0FF, color & 0x0FF};
}

void Controller::SetColor(uint32_t color)
{
    light_.SetColor(color);
    SaveState();
}

void Controller::SetAlarm(const Alarm::alarm_t& alarm)
{
    alarm_.SetAlarm(alarm);
    SaveState();
}

void Controller::SetAnimation(const std::string& hash)
{
    animation_.SetAnimation(hash);
    SaveState();
}

void Controller::SetPowerLight(bool on)
{
    if(on != light_.GetPower())
    {
        I(fmt::format("SetPowerLight {}", on));
        fadeout_.Stop();
        light_.SetPower(on);
        SaveState();
    }

    SendPowerStatus();
}

bool Controller::GetPowerLight() const
{
    return light_.GetPower();
}

void Controller::SetPredefinedColors(const ColorVector& colors)
{
    light_.SetPredefinedColors(colors);
    SaveState();
}

void Controller::SetPowerAnimation(bool on)
{
    if(on != animation_.GetPower())
    {
        I(fmt::format("SetPowerAnimation {}", on));
        fadeout_.Stop();
        animation_.SetPower(on);
        SaveState();
    }

    SendPowerStatus();
}

bool Controller::GetPowerAnimation() const
{
    return animation_.GetPower();
}

void Controller::SetPowerTimeout(const std::string& target, std::chrono::minutes minutes)
{
    fadeout_.SetTimeout(target, minutes);
}

std::tuple<std::string, int, uint8_t> Controller::GetSystemConfig() const
{
    return {name_, ledstring_.channel[0].count, ledstring_.channel[0].brightness};
}

void Controller::SetSystemConfig(const std::string& name, int led_count, uint8_t max_brightness)
{
    Clear();

    name_ = name;
    ledstring_.channel[0].count = led_count;
    ledstring_.channel[0].brightness = max_brightness;

    ws2811_fini(&ledstring_);

    ws2811_return_t ret = WS2811_SUCCESS;
    if ((ret = ws2811_init(&ledstring_)) != WS2811_SUCCESS)
    {
        E(fmt::format("ws2811_init failed: {} ({})", ws2811_get_return_t_str(ret), ret));
        io_.stop();
        return;
    }

    fadeout_.SetNormalBrightness(max_brightness);
    light_.SetColor();

    nlohmann::json cfg;
    cfg["name"] = name_;
    cfg["led_count"] = ledstring_.channel[0].count;
    cfg["max_brightness"] = ledstring_.channel[0].brightness;

    std::filesystem::path path {kConfigPath};
    path /= kSystemFile;

    std::ofstream file(path);
    file << cfg;
    file.flush();
}

void Controller::SetBrightness(uint8_t brightness)
{
    ledstring_.channel[0].brightness = brightness;
    I(fmt::format("Set brightness to {}", ledstring_.channel[0].brightness));
    ws2811_return_t ret = WS2811_SUCCESS;
    if ((ret = ws2811_init(&ledstring_)) != WS2811_SUCCESS)
    {
        E(fmt::format("ws2811_init failed: {} ({})", ws2811_get_return_t_str(ret), ret));
    }
    light_.SetColor();
}

uint8_t Controller::GetBrightness() const
{
    return ledstring_.channel[0].brightness;
}

void Controller::SendPowerStatus() const
{
    if(session_ != nullptr)
    {
        session_->SendPowerStatus();
    }
}

#include "controller.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "session.hpp"

constexpr const char* kConfigFile = "config.json";
constexpr const char* kConfigPath = "/home/pi/.config/led_control/";

Controller::Controller() : Log("ctrl")
{
    memset(&ledstring, 0, sizeof (ledstring));
    ledstring.freq = TARGET_FREQ;
    ledstring.dmanum = DMA;
    ledstring.channel[0].gpionum = GPIO_PIN;
    ledstring.channel[0].count = LED_COUNT;
    ledstring.channel[0].invert = 0;
    ledstring.channel[0].brightness = 255;
    ledstring.channel[0].strip_type = STRIP_TYPE;

    const std::filesystem::path path{kConfigPath};
    std::filesystem::create_directories(path);
}

Controller::~Controller()
{
}

void Controller::SaveState()
{
    nlohmann::json cfg;
    cfg["name"] = name_;

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
    try
    {
        std::filesystem::path path {kConfigPath};
        path /= kConfigFile;
        std::ifstream file(path);

        nlohmann::json cfg;
        file >> cfg;

        name_ = cfg.value("name", "");

        light_.RestoreState(cfg.value("light", nlohmann::json()));
        alarm_.RestoreState(cfg.value("alarm", nlohmann::json()));
        I("Restored all configurations");
    }
    catch(const nlohmann::json::exception& e)
    {
        E(fmt::format("Parsing config failed: {}", e.what()));
    }
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

    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS)
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

    ws2811_fini(&ledstring);
    return ret;
}

bool Controller::SetupUdp()
{
    udp_socket_.open(asio::ip::udp::v4());
    asio::socket_base::broadcast option(true);
    udp_socket_.set_option(option);
    udp_socket_.bind(asio::ip::udp::endpoint(asio::ip::address_v4::any(), PORT ));


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
                // D(fmt::format("{} {} {}", s, remote_endpoint_.address().to_string(), remote_endpoint_.port()));
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
    std::size_t size = std::min(matrix.size(), static_cast<std::size_t>(ledstring.channel[0].count));
    memcpy(ledstring.channel[0].leds, matrix.data(), size * sizeof(ws2811_led_t));

    ws2811_return_t ret = WS2811_SUCCESS;
    if ((ret = ws2811_render(&ledstring)) != WS2811_SUCCESS)
    {
        E(fmt::format("ws2811_render failed: {} ({})", ws2811_get_return_t_str(ret), ret));
    }
    return ret;
}

ws2811_return_t Controller::Render(ws2811_led_t color)
{
    std::vector<ws2811_led_t> matrix(LED_COUNT, color);
    return Render(matrix);
}

ws2811_return_t Controller::Clear()
{
    std::vector<ws2811_led_t> matrix(LED_COUNT, 0x00000000);
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
    light_.SetPower(on);
    session_->sendPowerStatus();
    SaveState();
}
bool Controller::GetPowerLight() const
{
    return light_.GetPower();
}

void Controller::SetPowerAnimation(bool on)
{
    animation_.SetPower(on);
    session_->sendPowerStatus();
    SaveState();
}

bool Controller::GetPowerAnimation() const
{
    return animation_.GetPower();
}



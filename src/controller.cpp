#include "controller.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>

#include "session.hpp"

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
}

Controller::~Controller()
{
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

    LoadAnimation("example.json");

    D("***");

    timer_.async_wait([this](const asio::error_code& error){
        OnAnimate(error);
    });

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

void Controller::LoadAnimation(const std::string& filename)
{
    I(fmt::format("Load animation {}", filename));
    std::ifstream ifs(filename);
    const auto& json_ = nlohmann::json::parse(ifs);
    animation_ = json_["data"].get<animation_t>();
    index_ = 0;
}

void Controller::OnAnimate(const asio::error_code& error)
{
    if(error)
    {
        E(fmt::format("Cyclic loop failed: {}", error.message()));
        Clear();
        return;
    }

    if (index_ == animation_.size())
    {
        index_ = 0;
    }
    const auto& [time, matrix] = animation_[index_++];
    Render(matrix);
    timer_.expires_at( timer_.expiry() + std::chrono::milliseconds(time));
    timer_.async_wait([this](const asio::error_code& error){
        OnAnimate(error);
    });
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
                D(fmt::format("{} {} {}", s, remote_endpoint_.address().to_string(), remote_endpoint_.port()));
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

void Controller::Render(const std::vector<ws2811_led_t>& matrix)
{
    std::size_t size = std::min(matrix.size(), static_cast<std::size_t>(ledstring.channel[0].count));
    memcpy(ledstring.channel[0].leds, matrix.data(), size * sizeof(ws2811_led_t));

    ws2811_return_t ret = WS2811_SUCCESS;
    if ((ret = ws2811_render(&ledstring)) != WS2811_SUCCESS)
    {
        E(fmt::format("ws2811_render failed: {} ({})", ws2811_get_return_t_str(ret), ret));
    }
}

void Controller::Blue()
{
    std::vector<ws2811_led_t> matrix(LED_COUNT, 0x10000020);
    Render(matrix);
}

void Controller::Clear()
{
    std::vector<ws2811_led_t> matrix(LED_COUNT, 0x00000000);
    Render(matrix);
}

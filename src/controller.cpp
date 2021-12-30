#include "controller.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>

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

int Controller::exec()
{
    ws2811_return_t ret = WS2811_SUCCESS;

    sig_.async_wait([this](const asio::error_code& error, int signal_number) {
        OnSignal(error, signal_number);
    });

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

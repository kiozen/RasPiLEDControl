#include "controller.hpp"

#include <fmt/format.h>

#include <iostream>

Controller::Controller() : matrix_(LED_COUNT, 0)
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
        std::cerr << fmt::format("ws2811_init failed: {} ({})", ws2811_get_return_t_str(ret), ret) << std::endl;
        if(ret == WS2811_ERROR_MMAP)
        {
            std::cerr << "Try to run the app as root." << std::endl;
        }
        return ret;
    }

    Blue();

    timer_.async_wait([this](const asio::error_code& error){
        OnTimeout(error);
    });

    io_.run();

    Clear();

    ws2811_fini(&ledstring);
    return ret;
}

void Controller::OnTimeout(const asio::error_code& error)
{
    if(error)
    {
        std::cerr << std::endl << fmt::format("Cyclic loop failed: {}", error.message()) << std::endl;
        Clear();
        return;
    }

    static int count = 0;
    if(count & 1)
    {
        Blue();
    }
    else
    {
        Clear();
    }
    count++;

    timer_.expires_at( timer_.expiry() + UPDATE_PERIOD);
    timer_.async_wait([this](const asio::error_code& error){
        OnTimeout(error);
    });
}

void Controller::OnSignal(const asio::error_code& error, int signal_number)
{
    if (!error)
    {
        std::cout << std::endl << fmt::format("Controller stopped with signal {}", signal_number) << std::endl;
        io_.stop();
    }
    else
    {
        sig_.async_wait([this](const asio::error_code& error, int signal_number) {
            OnSignal(error, signal_number);
        });
    }
}

void Controller::Render()
{
    memcpy(ledstring.channel[0].leds, matrix_.data(), matrix_.size() * sizeof(ws2811_led_t));

    ws2811_return_t ret = WS2811_SUCCESS;
    if ((ret = ws2811_render(&ledstring)) != WS2811_SUCCESS)
    {
        std::cerr << fmt::format("ws2811_render failed: {} ({})", ws2811_get_return_t_str(ret), ret) << std::endl;
    }
}

void Controller::Blue()
{
    std::fill(matrix_.begin(), matrix_.end(), 0x10000020);
    Render();
}

void Controller::Clear()
{
    std::fill(matrix_.begin(), matrix_.end(), 0);
    Render();
}

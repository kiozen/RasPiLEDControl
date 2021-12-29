#include "controller.hpp"

#include <fmt/format.h>

#include <iostream>

Controller::Controller() : matrix(LED_COUNT, 0)
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

    sig.async_wait([this](const asio::error_code& error, int signal_number) {
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

    io.run();

    Clear();

    ws2811_fini(&ledstring);
    return ret;
}

void Controller::OnSignal(const asio::error_code& error, int signal_number)
{
    if (!error)
    {
        std::cout << std::endl << fmt::format("Controller stopped with signal {}", signal_number) << std::endl;
        io.stop();
    }
    else
    {
        sig.async_wait([this](const asio::error_code& error, int signal_number) {
            OnSignal(error, signal_number);
        });
    }
}

void Controller::Render()
{
    memcpy(ledstring.channel[0].leds, matrix.data(), matrix.size() * sizeof(ws2811_led_t));

    ws2811_return_t ret = WS2811_SUCCESS;
    if ((ret = ws2811_render(&ledstring)) != WS2811_SUCCESS)
    {
        std::cerr << fmt::format("ws2811_render failed: {} ({})", ws2811_get_return_t_str(ret), ret) << std::endl;
    }
}

void Controller::Blue()
{
    std::fill(matrix.begin(), matrix.end(), 0x00000020);
    Render();
}

void Controller::Clear()
{
    std::fill(matrix.begin(), matrix.end(), 0);
    Render();
}

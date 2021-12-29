#ifndef SRC_CONTROLER_HPP
#define SRC_CONTROLER_HPP

#include <asio.hpp>
#include <vector>
#include <ws2811/ws2811.h>

class Controller
{
public:
    Controller();
    virtual ~Controller() = default;

    int exec();

private:
    static constexpr int TARGET_FREQ = WS2811_TARGET_FREQ;
    static constexpr int GPIO_PIN = 18;
    static constexpr int DMA = 10;
    static constexpr int STRIP_TYPE = SK6812_STRIP_RGBW;  // SK6812RGBW (NOT SK6812RGB)
    static constexpr int LED_COUNT = 300;
    static constexpr auto UPDATE_PERIOD = asio::chrono::milliseconds(30);

    ws2811_t ledstring;

    void OnTimeout(const asio::error_code& error);
    void OnSignal(const asio::error_code& error, int signal_number);
    void Render();
    void Clear();
    void Blue();

    asio::io_context io_;
    asio::signal_set sig_ = {io_, SIGINT, SIGTERM};
    asio::steady_timer timer_ {io_, UPDATE_PERIOD};

    std::vector<ws2811_led_t> matrix_;
};

#endif // SRC_CONTROLER_HPP

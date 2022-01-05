#ifndef SRC_CONTROLER_HPP
#define SRC_CONTROLER_HPP

#include <array>
#include <asio.hpp>
#include <memory>
#include <tuple>
#include <vector>
#include <ws2811/ws2811.h>

#include "alarm_clock.hpp"
#include "log.hpp"

class Controller : public Log
{
public:
    Controller();
    virtual ~Controller();

    int exec();

    bool StartServer();

    void SetColor(uint8_t red, uint8_t green, uint8_t blue);
    std::tuple<uint8_t, uint8_t, uint8_t> GetColor() const;

    std::string getName() const {return name_;}

    void SetPower(bool on);
    bool GetPower() const {return power_;}

    void SetAlarm(const AlarmClock::alarm_t& alarm);
    AlarmClock::alarm_t GetAlarm() const {return alarm_clock_.GetAlarm();}

private:
    static constexpr int TARGET_FREQ = WS2811_TARGET_FREQ;
    static constexpr int GPIO_PIN = 18;
    static constexpr int DMA = 10;
    static constexpr int STRIP_TYPE = SK6812_STRIP_GRBW;  // SK6812RGBW (NOT SK6812RGB)
    static constexpr int LED_COUNT = 300;
    static constexpr auto UPDATE_PERIOD = std::chrono::milliseconds(5000);
    static constexpr uint16_t PORT = 7755;

    ws2811_t ledstring;

    void LoadAnimation(const std::string& filename);
    void OnAnimate(const asio::error_code& error);
    void OnSignal(const asio::error_code& error, int signal_number);
    void OnReceiveUdp(const asio::error_code& error, std::size_t size);
    void Render(const std::vector<ws2811_led_t>& matrix);
    void Clear();
    void Blue();
    bool SetupUdp();
    void SetColor(uint32_t color);

    void SaveState();
    void RestoreState();

    asio::io_context io_;
    asio::signal_set sig_ = {io_, SIGINT, SIGTERM};

    asio::ip::udp::socket udp_socket_ {io_};
    asio::ip::udp::endpoint remote_endpoint_;
    std::array<int8_t, 1024> recv_buffer_;

    asio::ip::tcp::endpoint endpoint_ {asio::ip::tcp::v4(), 7756};
    asio::ip::tcp::acceptor acceptor_ {io_, endpoint_};
    std::unique_ptr<class Session> session_;

    using animation_step_t = std::tuple<int, std::vector<ws2811_led_t> >;
    using animation_t = std::vector<animation_step_t >;
    int index_ {0};
    animation_t animation_;
    asio::steady_timer timer_animation_ {io_, UPDATE_PERIOD};

    std::string name_;
    std::string mac_;
    ws2811_led_t last_color_ {0};
    bool power_ {false};

    AlarmClock alarm_clock_ {io_, *this};
};

#endif // SRC_CONTROLER_HPP

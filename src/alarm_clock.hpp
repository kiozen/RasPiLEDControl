#ifndef SRC_ALARM_CLOCK
#define SRC_ALARM_CLOCK

#include <asio.hpp>
#include <set>

#include "log.hpp"

class Controller;

class AlarmClock : public Log
{
public:
    AlarmClock(asio::io_context& io, Controller& parent);
    virtual ~AlarmClock();

    struct alarm_t
    {
        std::string name;
        bool active {false};
        int32_t hour {-1};
        int32_t minute {-1};
        std::set<int> days;
    };

    void SetAlarm(const alarm_t& alarm)
    {
        alarm_ = alarm;
    }

    alarm_t GetAlarm() const
    {
        return alarm_;
    }

private:
    void OnTimeout(const asio::error_code& error);

    static constexpr auto UPDATE_PERIOD = std::chrono::minutes(5);
    asio::io_context& io_;
    Controller& controller_;

    asio::steady_timer timer_ {io_, std::chrono::seconds(1)};

    alarm_t alarm_;
};

#endif // SRC_ALARM_CLOCK

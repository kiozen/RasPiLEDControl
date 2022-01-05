#ifndef SRC_ALARM_HPP
#define SRC_ALARM_HPP

#include <asio.hpp>
#include <nlohmann/json.hpp>
#include <set>

#include "log.hpp"

class Controller;

class Alarm : public Log
{
public:
    Alarm(asio::io_context& io, Controller& parent);
    virtual ~Alarm();

    void RestoreState(const nlohmann::json& cfg);
    nlohmann::json SaveState() const;

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

#endif // SRC_ALARM_HPP

#include "alarm_clock.hpp"

#include <chrono>
#include <fmt/chrono.h>
#include <fmt/format.h>


AlarmClock::AlarmClock(asio::io_context& io, Controller& parent)
    : Log("alarm")
    , io_(io)
    , controller_(parent)
{
    timer_.async_wait([this](const asio::error_code& error){OnTimeout(error);});
}

AlarmClock::~AlarmClock()
{
}

void AlarmClock::OnTimeout(const asio::error_code& error)
{
    if(error)
    {
        E(fmt::format("Timer failed with {}", error.message()));
    }

    using namespace std::chrono;

    // system clock as now
    const time_point<system_clock>& now = system_clock::now();
    // now as seconds since epoche
    const time_t& tt_now = system_clock::to_time_t(now);
    // now as good old time structure
    const tm& local_tm_now = *localtime(&tt_now);

    // truncate to past round 5 minutes
    tm local_tm_past = local_tm_now;
    local_tm_past.tm_min = (local_tm_now.tm_min / 5) * 5;
    local_tm_past.tm_sec = 0;

    // get seconds since epoche for the next round five minutes in the future
    time_t tt_next = mktime(&local_tm_past) + 300;

    // set new expiry time relative to timer expiry
    timer_.expires_at(timer_.expiry() + seconds(tt_next - tt_now));
    timer_.async_wait([this](const asio::error_code& error){OnTimeout(error);});

    if(alarm_.active)
    {
        if(alarm_.hour == local_tm_now.tm_hour
           && alarm_.minute == local_tm_now.tm_min
           && alarm_.days.count(local_tm_now.tm_wday))
        {
            I("Trigger alarm");
        }
    }
}

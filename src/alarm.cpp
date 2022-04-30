/**********************************************************************************************
    Copyright (C) 2022 Oliver Eichler <oliver.eichler@gmx.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

**********************************************************************************************/
#include "alarm.hpp"

#include <fmt/format.h>

#include "animation.hpp"
#include "power.hpp"

Alarm::Alarm(const std::string &config_path, asio::io_context &io, Power &power,
             Animation &animation)
    : Log("alarm"), config_path_(config_path), timer_(io, std::chrono::seconds(1)), power_(power),
      animation_(animation) {
  timer_.async_wait([this](const asio::error_code &error) { OnTimeout(error); });

  restore_state_active_ = true;
  try {
    const nlohmann::json &cfg = LoadState(config_path_, kConfigFile);
    alarm_.name = cfg.value("name", "");
    alarm_.active = cfg.value("active", false);
    alarm_.hour = cfg.value("hour", -1);
    alarm_.minute = cfg.value("minute", -1);
    alarm_.days = cfg.value<std::set<int>>("days", std::set<int>());
    alarm_.animation_hash = cfg.value("animation_hash", "");

    I("Restored alarm");
  } catch (const nlohmann::json::exception &e) {
    E(fmt::format("Parsing config failed: {}", e.what()));
  }
  restore_state_active_ = false;
}
Alarm::~Alarm() {}

void Alarm::Stop() {
  is_alive_.store(false);
  timer_.cancel();
}

void Alarm::SaveState() {
  nlohmann::json alarm;
  alarm["name"] = alarm_.name;
  alarm["active"] = alarm_.active;
  alarm["hour"] = alarm_.hour;
  alarm["minute"] = alarm_.minute;
  alarm["days"] = alarm_.days;
  alarm["animation_hash"] = alarm_.animation_hash;

  IModule::SaveState(config_path_, kConfigFile, alarm);
}

void Alarm::SetAlarm(const alarm_t &alarm) {
  alarm_ = alarm;
  SaveState();
}

void Alarm::OnTimeout(const asio::error_code &error) {
  if (error) {
    E(fmt::format("Timer failed with {}", error.message()));
  }

  if (!is_alive_.load()) {
    return;
  }

  using namespace std::chrono;

  // system clock as now
  const time_point<system_clock> &now = system_clock::now();
  // now as seconds since epoche
  const time_t &tt_now = system_clock::to_time_t(now);
  // now as good old time structure
  const tm &local_tm_now = *localtime(&tt_now);

  // truncate to past round 5 minutes
  tm local_tm_past = local_tm_now;
  local_tm_past.tm_min = (local_tm_now.tm_min / 5) * 5;
  local_tm_past.tm_sec = 0;

  // get seconds since epoche for the next round five minutes in the future
  time_t tt_next = mktime(&local_tm_past) + 300;

  // set new expiry time relative to timer expiry
  timer_.expires_at(timer_.expiry() + seconds(tt_next - tt_now));
  timer_.async_wait([this](const asio::error_code &error) { OnTimeout(error); });

  if (alarm_.active) {
    if (alarm_.hour == local_tm_now.tm_hour && alarm_.minute == local_tm_now.tm_min &&
        alarm_.days.count(local_tm_now.tm_wday)) {
      I("Trigger alarm");
      animation_.SetAnimation(alarm_.animation_hash);
      power_.SetChannelState(Power::kAnimation, true);
    }
  }
}

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
#ifndef SRC_ALARM_HPP
#define SRC_ALARM_HPP

#include <asio.hpp>
#include <set>

#include "i_module.hpp"
#include "log.hpp"

class Power;
class Animation;

class Alarm : public Log, public IModule {
public:
  Alarm(const std::string &config_path, asio::io_context &io, Power &power, Animation &animation);
  virtual ~Alarm();

  struct alarm_t {
    std::string name;
    bool active{false};
    int32_t hour{-1};
    int32_t minute{-1};
    std::set<int> days;
    std::string animation_hash;
  };

  void SetAlarm(const alarm_t &alarm);

  alarm_t GetAlarm() const { return alarm_; }

  void Stop();

private:
  static constexpr const char *kConfigFile = "alarm.json";
  void SaveState() override;
  void OnTimeout(const asio::error_code &error);

  const std::string config_path_;
  asio::steady_timer timer_;
  Power &power_;
  Animation &animation_;

  alarm_t alarm_;

  std::atomic_bool is_alive_{true};
};

#endif // SRC_ALARM_HPP

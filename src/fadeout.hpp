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
#ifndef SRC_FADEOUT_HPP
#define SRC_FADEOUT_HPP

#include <asio.hpp>

#include "log.hpp"
#include "power.hpp"

class Power;
class WS2811Control;

class Fadeout : public Log {
public:
  Fadeout(asio::io_context &io, Power &power, WS2811Control &ws2811_control);
  ~Fadeout();

  void SetTimeout(const std::string &target, std::chrono::minutes minutes);
  bool GetTimeoutActive() const { return target_ != Power::kNone; }

  void Stop();

private:
  static constexpr auto kFadeoutInterval = std::chrono::seconds(1);
  static constexpr auto kFadeoutSteps = 100;

  void OnStartFadeOut();
  void OnFadeOut(uint8_t count, const asio::error_code &error);
  void OnPowerStatusChanged();

  Power &power_;
  WS2811Control &ws2811_control_;

  asio::steady_timer timeout_power_;
  asio::steady_timer timer_fade_out_;

  Power::channel_e target_{Power::kNone};
};

#endif // SRC_FADEOUT_HPP

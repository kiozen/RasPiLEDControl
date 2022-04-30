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

#include "fadeout.hpp"

#include <fmt/format.h>

#include "ws2811_control.hpp"

Fadeout::Fadeout(asio::io_context &io, Power &power, WS2811Control &ws2811_control)
    : Log("fadeout"), power_(power), ws2811_control_(ws2811_control), timeout_power_(io),
      timer_fade_out_(io) {
  power_.SigPowerStatusChanged.connect(&Fadeout::OnPowerStatusChanged, this);
}

Fadeout::~Fadeout() {}

void Fadeout::Stop() {
  std::size_t n = 0;
  n += timeout_power_.cancel();
  n += timer_fade_out_.cancel();
  if (n != 0) {
    I("StopPowerTimeout");
    target_ = Power::kNone;
  }
}

void Fadeout::OnPowerStatusChanged() {
  I("OnPowerStatusChanged");
  for (Power::channel_e channel : Power::GetAvailableChannels()) {
    if (power_.GetChannelState(channel) == false && target_ == channel) {
      Stop();
    }
  }
}

void Fadeout::SetTimeout(const std::string &target, std::chrono::minutes minutes) {
  I("SetPowerTimeout");
  Stop();

  target_ = Power::kNone;
  if (target == "animation") {
    target_ = Power::kAnimation;
  } else if (target == "light") {
    target_ = Power::kLight;
  } else {
    return;
  }

  if (power_.GetChannelState(target_) == false) {
    power_.SetChannelState(target_, true);
  } else {
    power_.SigPowerStatusChanged();
  }

  timeout_power_.expires_after(minutes - kFadeoutSteps * kFadeoutInterval);
  timeout_power_.async_wait([this](const asio::error_code &error) {
    I(fmt::format("PowerTimeout {} {}", error.message(), error.value()));
    if (!error) {
      OnStartFadeOut();
    }
  });
}

void Fadeout::OnStartFadeOut() {
  I("OnStartFadeOut");
  uint8_t count = 1;

  timer_fade_out_.expires_after(kFadeoutInterval);
  timer_fade_out_.async_wait(
      [this, count](const asio::error_code &error) { OnFadeOut(count, error); });
}

void Fadeout::OnFadeOut(uint8_t count, const asio::error_code &error) {
  if (!error) {
    float brightness = 1.0 - count++ / float(kFadeoutSteps);
    if (brightness > 0) {
      ws2811_control_.SetBrightness(brightness);

      timer_fade_out_.expires_at(timer_fade_out_.expiry() + kFadeoutInterval);
      timer_fade_out_.async_wait(
          [this, count](const asio::error_code &error) { OnFadeOut(count, error); });
    } else {
      Power::channel_e channel = target_;
      target_ = Power::kNone; // must be set before calling SetChannelState()
      power_.SetChannelState(channel, false);
    }
  } else {
    target_ = Power::kNone;
  }
}

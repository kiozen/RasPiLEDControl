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
#include "power.hpp"

#include <fmt/format.h>

#include "ws2811_control.hpp"

const std::vector<ws2811_led_t> Power::kBlackFrame(WS2811Control::kLedCount, 0);

Power::Power(const std::string &config_path, WS2811Control &ws2811_control)
    : Log("power"), config_path_(config_path), ws2811_control_(ws2811_control) {
  restore_state_active_ = true;
  try {
    const nlohmann::json &cfg = LoadState(config_path_, kConfigFile);

    SetChannelState(kLight, cfg.value("light", false));
    SetChannelState(kAnimation, cfg.value("animation", false));
  } catch (const nlohmann::json::exception &e) {
    E(fmt::format("Parsing system config failed: {}", e.what()));
  }

  restore_state_active_ = false;
}
Power::~Power() {}

void Power::SetChannelState(channel_e id, bool on) {

  channel_e active_id = kNone;
  for (channel_t &channel : channels_) {
    if (channel.active) {
      active_id = channel.id;
    }
  }

  D("------------- SetChannelState start -------------");
  D(fmt::format("active channel: {}", active_id));
  D(fmt::format("current channel: {} new state: {}", id, on));

  channel_t &channel = channels_[id];

  if (channel.active != on) {
    if (active_id != kNone) {
      channels_[active_id].active = false;
    }
    channel.active = on;
    ws2811_control_.SetFrame(on ? channel.frame : kBlackFrame);
    ws2811_control_.SetBrightness(1.0);
  }
  SigPowerStatusChanged();
  D("------------- SetChannelState done -------------");
  SaveState();
}

void Power::SetChannelFrame(channel_e channel, const std::vector<ws2811_led_t> &frame) {
  channel_t &channel_ = channels_[channel];
  channel_.frame = frame;
  if (channel_.active) {
    ws2811_control_.SetFrame(channel_.frame);
  }
}

void Power::SetChannelFrame(channel_e channel, const ws2811_led_t &color) {
  SetChannelFrame(channel, std::vector<ws2811_led_t>(WS2811Control::kLedCount, color));
}

void Power::SaveState() {
  nlohmann::json cfg;
  cfg["light"] = GetChannelState(kLight);
  cfg["animation"] = GetChannelState(kAnimation);
  IModule::SaveState(config_path_, kConfigFile, cfg);
}

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
#ifndef SRC_POWER_HPP
#define SRC_POWER_HPP

#include <array>
#include <sigslot/signal.hpp>
#include <ws2811/ws2811.h>

#include "i_module.hpp"
#include "log.hpp"

class WS2811Control;

class Power : public Log, public IModule {
public:
  Power(const std::string &config_path, WS2811Control &ws2811_control);
  virtual ~Power();

  enum channel_e : int { kLight, kAnimation, kMaxChannel, kNone = -1 };

  bool GetChannelState(channel_e channel) const { return channels_[channel].active; }
  static std::vector<channel_e> GetAvailableChannels() { return {kLight, kAnimation}; }

  void SetChannelState(channel_e channel, bool on);
  void SetChannelFrame(channel_e channel, const std::vector<ws2811_led_t> &frame);
  void SetChannelFrame(channel_e channel, const ws2811_led_t &color);

  sigslot::signal_st<> SigPowerStatusChanged;

private:
  static constexpr const char *kConfigFile = "power.json";
  static const std::vector<ws2811_led_t> kBlackFrame;
  const std::string config_path_;

  void SaveState() override;

  WS2811Control &ws2811_control_;
  struct channel_t {
    channel_t(channel_e id) : id(id) {}
    const channel_e id;
    bool active{false};
    std::vector<ws2811_led_t> frame{kBlackFrame};
  };

  std::array<channel_t, kMaxChannel> channels_{channel_t(kLight), channel_t(kAnimation)};
};

#endif // SRC_POWER_HPP

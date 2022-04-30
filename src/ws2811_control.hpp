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
#ifndef SRC_WS2811_CONTROL_HPP
#define SRC_WS2811_CONTROL_HPP

#include <vector>
#include <ws2811/ws2811.h>

#include "i_module.hpp"
#include "log.hpp"

class WS2811Control : public Log, public IModule {
public:
  static constexpr int kLedCount = 300;
  WS2811Control(const std::string &config_path);
  virtual ~WS2811Control();

  uint8_t GetMaxBrightness() const;
  int GetLedCount() const;

  void SetParameters(int led_count, uint8_t brightness);

  bool SetFrame(const std::vector<ws2811_led_t> &frame);
  void SetBrightness(float brightness);

private:
  static constexpr int kTargetFreq = WS2811_TARGET_FREQ;
  static constexpr int kGpioPin = 18;
  static constexpr int kDma = 10;
  static constexpr int kStripeType = SK6812_STRIP_GRBW; // SK6812RGBW (NOT SK6812RGB)
  static constexpr const char *kConfigFile = "ws2811.json";

  void SaveState() override;
  bool WriteHardwareInit();

  const std::string config_path_;
  ws2811_t ledstring_;

  uint8_t max_brightness_{255};
  float brightness_{1.0};
  std::vector<ws2811_led_t> current_frame_;
};

#endif // SRC_WS2811_CONTROL_HPP

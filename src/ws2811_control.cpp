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
#include "ws2811_control.hpp"

#include <fmt/format.h>
#include <memory.h>
#include <nlohmann/json.hpp>

WS2811Control::WS2811Control(const std::string &config_path)
    : Log("ws2811"), config_path_(config_path), current_frame_(kLedCount, 0) {
  memset(&ledstring_, 0, sizeof(ledstring_));
  ledstring_.freq = kTargetFreq;
  ledstring_.dmanum = kDma;
  ledstring_.channel[0].gpionum = kGpioPin;
  ledstring_.channel[0].count = kLedCount;
  ledstring_.channel[0].invert = 0;
  ledstring_.channel[0].brightness = max_brightness_;
  ledstring_.channel[0].strip_type = kStripeType;

  restore_state_active_ = true;
  try {
    const nlohmann::json &cfg = LoadState(config_path_, kConfigFile);
    ledstring_.channel[0].count = cfg.value("led_count", kLedCount);
    max_brightness_ = cfg.value("max_brightness", max_brightness_);
  } catch (const nlohmann::json::exception &e) {
    E(fmt::format("Parsing system config failed: {}", e.what()));
  }

  WriteHardwareInit();
  restore_state_active_ = false;
}

WS2811Control::~WS2811Control() { ws2811_fini(&ledstring_); }

uint8_t WS2811Control::GetMaxBrightness() const { return max_brightness_; }

void WS2811Control::SetBrightness(float brightness) {
  brightness_ = brightness;
  WriteHardwareInit();
}

int WS2811Control::GetLedCount() const { return ledstring_.channel[0].count; }

void WS2811Control::SetParameters(int led_count, uint8_t brightness) {
  max_brightness_ = brightness;
  ledstring_.channel[0].count = led_count;

  if (WriteHardwareInit()) {
    SaveState();
  }
}

bool WS2811Control::SetFrame(const std::vector<ws2811_led_t> &frame) {
  current_frame_ = frame;
  std::size_t size =
      std::min(current_frame_.size(), static_cast<std::size_t>(ledstring_.channel[0].count));
  memcpy(ledstring_.channel[0].leds, current_frame_.data(), size * sizeof(ws2811_led_t));

  D(fmt::format("SetFrame of size {} with brightness {} {:08X}..{:08X}", size,
                ledstring_.channel[0].brightness, frame[0], frame[size - 1]));

  ws2811_return_t ret = WS2811_SUCCESS;
  if ((ret = ws2811_render(&ledstring_)) != WS2811_SUCCESS) {
    E(fmt::format("ws2811_render failed: {} ({})", ws2811_get_return_t_str(ret), ret));
    return false;
  }
  if ((ret = ws2811_wait(&ledstring_)) != WS2811_SUCCESS) {
    E(fmt::format("ws2811_wait failed: {} ({})", ws2811_get_return_t_str(ret), ret));
    return false;
  }

  return true;
}

bool WS2811Control::WriteHardwareInit() {
  ledstring_.channel[0].brightness = round(max_brightness_ * brightness_);

  D(fmt::format("WriteHardwareInit count: {} brightness: {}", ledstring_.channel[0].count,
                ledstring_.channel[0].brightness));

  ws2811_return_t ret = WS2811_SUCCESS;
  if ((ret = ws2811_init(&ledstring_)) != WS2811_SUCCESS) {
    E(fmt::format("ws2811_init failed: {} ({})", ws2811_get_return_t_str(ret), ret));
    if (ret == WS2811_ERROR_MMAP) {
      E("Try to run the app as root.");
    }
    return false;
  }
  return SetFrame(current_frame_);
}

void WS2811Control::SaveState() {
  nlohmann::json cfg;
  cfg["led_count"] = ledstring_.channel[0].count;
  cfg["max_brightness"] = ledstring_.channel[0].brightness;

  IModule::SaveState(config_path_, kConfigFile, cfg);
}

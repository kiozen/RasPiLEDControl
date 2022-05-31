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
#include "light.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "power.hpp"

Light::Light(const std::string &config_path, Power &power)
    : Log("light"), power_(power), config_path_(config_path) {
  restore_state_active_ = true;
  try {
    const nlohmann::json &cfg = LoadState(config_path_, kConfigFile);

    ws2811_led_t color = cfg.value("color", 0);
    SetColor(color >> 16 & 0xff, color >> 8 & 0x0FF, color & 0x0FF);
    SetPredefinedColors(cfg.value("predefined_colors", ColorVector()));
  } catch (const nlohmann::json::exception &e) {
    E(fmt::format("Parsing system config failed: {}", e.what()));
  }
  restore_state_active_ = false;
}

Light::~Light() {}

void Light::SaveState() {
  nlohmann::json cfg;
  cfg["color"] = color_;
  cfg["predefined_colors"] = predefined_colors_;

  IModule::SaveState(config_path_, kConfigFile, cfg);
}

std::tuple<uint8_t, uint8_t, uint8_t> Light::GetColor() const {
  return {color_ >> 16 & 0xff, color_ >> 8 & 0x0FF, color_ & 0x0FF};
}

void Light::SetColor(uint8_t red, uint8_t green, uint8_t blue) {
  color_ = (red << 16 | green << 8 | blue);
  power_.SetChannelFrame(Power::kLight, color_);
  SaveState();
}

void Light::SetPredefinedColors(const ColorVector &colors) {
  predefined_colors_ = colors;
  SaveState();
}

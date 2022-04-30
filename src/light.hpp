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
#ifndef SRC_LIGHT_HPP
#define SRC_LIGHT_HPP

#include <vector>
#include <ws2811/ws2811.h>

#include "log.hpp"
#include "i_module.hpp"

using ColorVector = std::vector<ws2811_led_t>;
class Power;

class Light : public Log, public IModule {
public:
  Light(const std::string &config_path, Power &power);
  virtual ~Light();

  void SetColor(uint8_t red, uint8_t green, uint8_t blue);
  std::tuple<uint8_t, uint8_t, uint8_t> GetColor() const;

  void SetPredefinedColors(const ColorVector &colors);
  ColorVector GetPredefinedColors() const { return predefined_colors_; }

private:
  static constexpr const char *kConfigFile = "light.json";
  const std::string config_path_;

  void SaveState() override;

  Power &power_;
  ws2811_led_t color_{0};
  ColorVector predefined_colors_;
};

#endif // SRC_LIGHT_HPP

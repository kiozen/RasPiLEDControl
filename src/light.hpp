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

#include <asio.hpp>
#include <nlohmann/json.hpp>
#include <tuple>
#include <vector>
#include <ws2811/ws2811.h>

#include "log.hpp"
#include "power.hpp"

class Controller;

using ColorVector = std::vector<ws2811_led_t>;

class Light : public Power, public Log
{
public:
    Light(asio::io_context& io, Controller& parent);
    virtual ~Light();

    void RestoreState(const nlohmann::json& cfg);
    nlohmann::json SaveState() const;

    void SetColor();
    void SetColor(ws2811_led_t color);
    ws2811_led_t GetColor() const {return color_;}

    void SetPredefinedColors(const ColorVector& colors){predefined_colors_ = colors;}
    ColorVector GetPredefinedColors() const {return predefined_colors_;}

protected:
    bool SwitchOn() override;
    void SwitchOff() override;

private:
    asio::io_context& io_;
    Controller& controller_;

    ws2811_led_t color_ {0};

    ColorVector predefined_colors_;
};

#endif // SRC_LIGHT_HPP

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

#include "controller.hpp"


Light::Light(asio::io_context& io, Controller& parent)
    : Power(module_e::light, parent)
    , Log("light")
    , io_(io)
    , controller_(parent)
{
}

Light::~Light()
{
}

void Light::RestoreState(const nlohmann::json& cfg)
{
    try
    {
        color_ = cfg.value("color", 0);
        I("Restored light");
    }
    catch(const nlohmann::json::exception& e)
    {
        E(fmt::format("Parsing config failed: {}", e.what()));
    }
}

nlohmann::json Light::SaveState() const
{
    nlohmann::json cfg;
    cfg["color"] = color_;
    cfg["power"] = GetPower();
    return cfg;
}

void Light::SetColor()
{
    if(GetPower())
    {
        SwitchOn();
    }
}

void Light::SetColor(ws2811_led_t color)
{
    color_ = color;
    if(GetPower())
    {
        SwitchOn();
    }
}

bool Light::SwitchOn()
{
    if(!GetPower())
    {
        I("Power on");
    }
    return controller_.Render(color_) == WS2811_SUCCESS;
}

void Light::SwitchOff()
{
    if(GetPower())
    {
        I("Power off");
    }
}

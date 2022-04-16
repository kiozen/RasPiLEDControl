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

#include "controller.hpp"


Fadeout::Fadeout(asio::io_context& io, Controller& parent)
    : Log("timeout")
    , io_(io)
    , controller_(parent)
{
}

Fadeout::~Fadeout()
{
}

void Fadeout::SetNormalBrightness(uint8_t brightness)
{
    I(fmt::format("Set normal brightness to {}", brightness));
    normal_brightness_ = brightness;
}

void Fadeout::Stop()
{
    std::size_t n = 0;
    n += timeout_power_.cancel();
    n += timer_fade_out_.cancel();
    if(n != 0)
    {
        I("StopPowerTimeout");
        timeout_active_ = false;
    }
    controller_.SetBrightness(normal_brightness_);
}

void Fadeout::SetTimeout(const std::string& target, std::chrono::minutes minutes)
{
    I("SetPowerTimeout");
    Stop();

    if(target == "animation")
    {
        controller_.SetPowerAnimation(true);
    }
    else if(target == "light")
    {
        controller_.SetPowerLight(true);
    }
    else
    {
        return;
    }

    timeout_active_ = true;
    timeout_power_.expires_after(minutes - kFadeoutSteps * kFadeoutInterval);
    timeout_power_.async_wait([this](const asio::error_code& error){
        I(fmt::format("PowerTimeout {} {}", error.message(), error.value()));
        if(!error)
        {
            StartFadeOut();
        }
    });

    controller_.SendPowerStatus();
}

void Fadeout::StartFadeOut()
{
    I("StartFadeOut");
    uint8_t count = 1;

    timer_fade_out_.expires_after(kFadeoutInterval);
    timer_fade_out_.async_wait([this, count](const asio::error_code& error){
        FadeOut(count, error);
    });
}

void Fadeout::FadeOut(uint8_t count, const asio::error_code& error)
{
    if(!error)
    {
        uint8_t new_brightness = round((1.0 - count++ / float(kFadeoutSteps)) * normal_brightness_);
        if(new_brightness > 0)
        {
            controller_.SetBrightness(new_brightness);
            timer_fade_out_.expires_after(kFadeoutInterval);
            timer_fade_out_.async_wait([this, count](const asio::error_code& error){
                FadeOut(count, error);
            });
        }
        else
        {
            timeout_active_ = false;
            controller_.SetPowerAnimation(false);
            controller_.SetPowerLight(false);
        }
    }
    else
    {
        timeout_active_ = false;
    }
}

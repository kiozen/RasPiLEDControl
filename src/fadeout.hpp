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
#ifndef SRC_TIMEOUT_HPP
#define SRC_TIMEOUT_HPP

#include <asio.hpp>

#include "log.hpp"

class Controller;

class Fadeout : public Log
{
public:
    Fadeout(asio::io_context& io, Controller& parent);
    ~Fadeout();

    void SetTimeout(const std::string& target, std::chrono::minutes minutes);
    bool GetTimeout() const {return timeout_active_;}

    void Stop();

    void SetNormalBrightness(uint8_t brightness);

private:
    static constexpr auto kFadeoutInterval = std::chrono::seconds(1);
    static constexpr auto kFadeoutSteps = 100;


    void StartFadeOut();
    void FadeOut(uint8_t count, const asio::error_code& error);

    asio::io_context& io_;
    Controller& controller_;

    asio::steady_timer timeout_power_{io_};
    asio::steady_timer timer_fade_out_{io_};
    bool timeout_active_{false};

    uint8_t normal_brightness_{0};
};


#endif // SRC_TIMEOUT_HPP

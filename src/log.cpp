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
#include "log.hpp"

#include <chrono>
#include <fmt/chrono.h>
#include <iostream>

Log::Log(const std::string& tag) : tag_(tag)
{
}

Log::~Log()
{
}

void Log::E(const std::string& msg)
{
    print("[E]", msg);
}

void Log::I(const std::string& msg)
{
    print("[I]", msg);
}

void Log::D(const std::string& msg)
{
    print("[D]", msg);
}

void Log::print(const std::string& level, const std::string& msg)
{
    const auto& now = std::chrono::system_clock::now();
    const auto& duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

    std::cout << fmt::format("{:%y-%m-%d %H:%M:%S}.{:03} {} {}: ", now, millis, level, tag_) << msg << std::endl;
}

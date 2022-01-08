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
#ifndef SRC_LOG_HPP
#define SRC_LOG_HPP

#include <string>

class Log
{
protected:
    Log(const std::string& tag);
    virtual ~Log();

    void E(const std::string& msg);
    void I(const std::string& msg);
    void D(const std::string& msg);

private:
    void print(const std::string& level, const std::string& msg);
    std::string tag_;
};

#endif // SRC_LOG_HPP

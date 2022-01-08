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
#ifndef SRC_POWER_HPP
#define SRC_POWER_HPP

#include <vector>

class Controller;

class Power
{
protected:
    enum class module_e
    {
        light,
        animation,
        none
    };

public:
    Power(module_e module, Controller& controller);
    virtual ~Power();

    void SetPower(bool on);
    bool GetPower() const;

protected:
    virtual bool SwitchOn() = 0;
    virtual void SwitchOff() = 0;

private:
    Controller& controller_;

    const module_e module_;
    static module_e active_module_;
    static std::vector<Power*> modules_;
};

#endif // SRC_POWER_HPP

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
#include "power.hpp"

#include "controller.hpp"

Power::module_e Power::active_module_ = Power::module_e::none;
std::vector<Power*> Power::modules_;

Power::Power(module_e module, Controller& controller)
    : controller_(controller)
    , module_(module)
{
    modules_.push_back(this);
}

Power::~Power()
{
}

void Power::SetPower(bool on)
{
    // already in the correct state - nothing to do
    if(on == GetPower())
    {
        return;
    }


    for(Power* module : modules_)
    {
        module->SwitchOff();
    }

    if(on)
    {
        if(!SwitchOn())
        {
            SwitchOff();
            controller_.Clear();
            active_module_ = module_e::none;
        }
        active_module_ = module_;
    }
    else
    {
        controller_.Clear();
        active_module_ = module_e::none;
    }
}

bool Power::GetPower() const
{
    return active_module_ == module_;
}

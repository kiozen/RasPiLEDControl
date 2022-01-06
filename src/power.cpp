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
        active_module_ = module_;
        if(!SwitchOn())
        {
            controller_.Clear();
            active_module_ = module_e::none;
        }
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

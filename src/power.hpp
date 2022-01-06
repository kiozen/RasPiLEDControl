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

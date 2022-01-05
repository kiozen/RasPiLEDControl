#include "light.hpp"

#include <fmt/format.h>

#include "controller.hpp"


Light::Light(asio::io_context& io, Controller& parent)
    : Log("light")
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
    return cfg;
}

void Light::SetColor(ws2811_led_t color)
{
    color_ = color;
    controller_.SwitchPower();
}

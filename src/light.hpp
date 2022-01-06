#ifndef SRC_LIGHT_HPP
#define SRC_LIGHT_HPP

#include <asio.hpp>
#include <nlohmann/json.hpp>
#include <tuple>
#include <ws2811/ws2811.h>

#include "log.hpp"
#include "power.hpp"

class Controller;

class Light : public Power, public Log
{
public:
    Light(asio::io_context& io, Controller& parent);
    virtual ~Light();

    void RestoreState(const nlohmann::json& cfg);
    nlohmann::json SaveState() const;

    void SetColor(ws2811_led_t color) {color_ = color;}
    ws2811_led_t GetColor() const {return color_;}

protected:
    bool SwitchOn() override;
    void SwitchOff() override {}

private:
    asio::io_context& io_;
    Controller& controller_;

    ws2811_led_t color_ {0};
};

#endif // SRC_LIGHT_HPP

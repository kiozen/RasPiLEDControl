#ifndef SRC_LIGHT_HPP
#define SRC_LIGHT_HPP

#include <asio.hpp>
#include <nlohmann/json.hpp>
#include <tuple>
#include <ws2811/ws2811.h>

#include "log.hpp"

class Controller;

class Light : public Log
{
public:
    Light(asio::io_context& io, Controller& parent);
    virtual ~Light();

    void RestoreState(const nlohmann::json& cfg);
    nlohmann::json SaveState() const;

    void SetColor(ws2811_led_t color);
    ws2811_led_t GetColor() const {return color_;}
    std::tuple<uint8_t, uint8_t, uint8_t> GetColorRgb() const
    {
        return {color_ >> 16 & 0xff, color_ >> 8 & 0x0FF, color_ & 0x0FF};
    }


private:
    asio::io_context& io_;
    Controller& controller_;

    ws2811_led_t color_ {0};
};

#endif // SRC_LIGHT_HPP

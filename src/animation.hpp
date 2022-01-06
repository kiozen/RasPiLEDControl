#ifndef SRC_ANIMATION_HPP
#define SRC_ANIMATION_HPP

#include <asio.hpp>
#include <filesystem>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <ws2811/ws2811.h>

#include "log.hpp"

class Controller;

class Animation : public Log
{
public:
    Animation(asio::io_context& io, Controller& parent);
    virtual ~Animation();

    struct info_t
    {
        std::string name;
        std::string desc;
        std::filesystem::path path;
    };

    nlohmann::json GetAnimationInfo() const;

    bool StartAnimation(const std::string& hash);

private:
    void LoadAnimation(const std::string& filename);
    void OnAnimate(const asio::error_code& error);

    Controller& controller_;

    using animation_step_t = std::tuple<int, std::vector<ws2811_led_t> >;
    using animation_t = std::vector<animation_step_t >;
    animation_t animation_;

    int index_ {0};

    asio::steady_timer timer_;
    std::map<std::string, info_t> animations_;
};

#endif // SRC_ANIMATION_HPP

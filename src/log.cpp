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

    std::cout << fmt::format("{:%y-%m-%d %H:%M:%S}.{} {} {}: ", now, millis, level, tag_) << msg << std::endl;
}

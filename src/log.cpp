#include "log.hpp"

#include <chrono>
#include <fmt/chrono.h>
#include <iostream>

Log::Log(const std::string& tag) : tag_(tag)
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
    std::cout << fmt::format("{:%y-%m-%d %H:%M:%S} {}: ", std::chrono::system_clock::now(), level) << msg << std::endl;
}

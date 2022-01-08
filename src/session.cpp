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
#include "session.hpp"

#include <chrono>
#include <fmt/format.h>

#include "controller.hpp"

Session::Session(asio::io_context& io, Controller& parent)
    : Log("session")
    , socket_(io)
    , timer_(io)
    , controller_(parent)
{
}

Session::~Session()
{
    D("~Session");
}


void Session::Exec()
{
    // timeout stall sessions
    timer_.cancel();
    timer_.expires_at(std::chrono::steady_clock::now() + std::chrono::minutes(1));
    timer_.async_wait([this](const asio::error_code& error){
        if(error != asio::error::operation_aborted)
        {
            I("Session timeout");
            socket_.close();
        }
    });

    // start receiving
    std::shared_ptr<asio::streambuf> buffer(new asio::streambuf());
    asio::async_read_until(socket_, *buffer.get(), '\n', [this, buffer](const asio::error_code& error, std::size_t size){
        OnMessageReceived(buffer, error, size);
    });
}

void Session::OnMessageReceived(std::shared_ptr<asio::streambuf> buffer, const asio::error_code& error, std::size_t size)
{
    if(error)
    {
        E(fmt::format("OnMessageReceived failed: {}", error.message()));
        asio::post([this](){controller_.StartServer();});
        return;
    }

    std::istream is(buffer.get());
    std::string s;
    std::getline(is, s);

    while(s.size())
    {
        D(fmt::format("recv {}", s));

        try
        {
            const nlohmann::json& msg = nlohmann::json::parse(s);
            if(msg["cmd"] == "set_color")
            {
                uint8_t red = msg["red"];
                uint8_t green = msg["green"];
                uint8_t blue = msg["blue"];

                controller_.SetColorRgb(red, green, blue);
            }
            else if(msg["cmd"] == "get_color")
            {
                auto [red, green, blue] = controller_.GetColorRgb();
                nlohmann::json resp;
                resp["rsp"] = "get_color";
                resp["red"] = red;
                resp["green"] = green;
                resp["blue"] = blue;
                sendJson(resp);
            }
            else if(msg["cmd"] == "set_alarm")
            {
                Alarm::alarm_t alarm;
                alarm.name = msg["name"];
                alarm.active = msg["active"];
                alarm.hour = msg["hour"];
                alarm.minute = msg["minute"];
                alarm.days = msg["days"].get<std::set<int> >();
                alarm.animation_hash = msg["animation_hash"];
                controller_.SetAlarm(alarm);
            }
            else if(msg["cmd"] == "get_alarm")
            {
                const Alarm::alarm_t& alarm = controller_.GetAlarm();
                nlohmann::json resp;
                resp["rsp"] = "get_alarm";
                resp["name"] = alarm.name;
                resp["active"] = alarm.active;
                resp["hour"] = alarm.hour;
                resp["minute"] = alarm.minute;
                resp["days"] = alarm.days;
                resp["animation_hash"] = alarm.animation_hash;
                sendJson(resp);
            }
            else if(msg["cmd"] == "get_animations")
            {
                nlohmann::json resp;
                resp["rsp"] = "get_animations";
                resp["animations"] = controller_.GetAnimationInfo();
                sendJson(resp);
            }
            else if(msg["cmd"] == "set_animation")
            {
                controller_.SetAnimation(msg["hash"]);
            }
            else if(msg["cmd"] == "get_animation")
            {
                nlohmann::json resp;
                resp["rsp"] = "get_animation";
                resp["hash"] = controller_.GetAnimation();
                sendJson(resp);
            }
            else if(msg["cmd"] == "set_power_light")
            {
                controller_.SetPowerLight(msg["power"]);
            }
            else if(msg["cmd"] == "set_power_animation")
            {
                controller_.SetPowerAnimation(msg["power"]);
            }
            else if(msg["cmd"] == "get_power")
            {
                sendPowerStatus();
            }
        }
        catch(const nlohmann::json::exception& e)
        {
            E(fmt::format("Parsing message failed: {}", e.what()));
            break;
        }

        std::getline(is, s);
    }

    Exec();
}

void Session::sendPowerStatus()
{
    nlohmann::json resp;
    resp["rsp"] = "get_power";
    resp["light"] = controller_.GetPowerLight();
    resp["animation"] = controller_.GetPowerAnimation();
    sendJson(resp);
}

void Session::sendJson(const nlohmann::json& msg)
{
    if(socket_.is_open())
    {
        D(fmt::format("send: {}", msg.dump()));
        socket_.send(asio::buffer(msg.dump() + "\n"));
    }
}

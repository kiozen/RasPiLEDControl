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
        D(fmt::format("OnMessageReceived {}", s));

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
            else if(msg["cmd"] == "set_power")
            {
                controller_.SetPower(msg["power"]);

                nlohmann::json resp;
                resp["rsp"] = "get_power";
                resp["power"] = controller_.GetPower();
                sendJson(resp);
            }
            else if(msg["cmd"] == "get_power")
            {
                nlohmann::json resp;
                resp["rsp"] = "get_power";
                resp["power"] = controller_.GetPower();
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
                controller_.StartAnimation(msg["hash"]);

                nlohmann::json resp;
                resp["rsp"] = "get_power";
                resp["power"] = controller_.GetPower();
                sendJson(resp);
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

void Session::sendJson(const nlohmann::json& msg)
{
    D(fmt::format("send: {}", msg.dump()));
    socket_.send(asio::buffer(msg.dump() + "\n"));
}

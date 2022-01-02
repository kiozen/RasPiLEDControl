#include "session.hpp"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "controller.hpp"

Session::Session(asio::io_context& io, Controller& parent)
    : Log("session")
    , socket_(io)
    , controller_(parent)
{
}

Session::~Session()
{
    D("~Session");
}


void Session::Exec()
{
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
                uint8_t red = msg["red"].get<uint8_t>();
                uint8_t green = msg["green"].get<uint8_t>();
                uint8_t blue = msg["blue"].get<uint8_t>();

                D(fmt::format("Red {} Green {} Blue {}", red, green, blue));
                controller_.SetColor(red, green, blue);
            }
            else if(msg["cmd"] == "get_color")
            {
                auto [red, green, blue] = controller_.GetColor();
                nlohmann::json resp;
                resp["rsp"] = "get_color";
                resp["red"] = red;
                resp["green"] = green;
                resp["blue"] = blue;
                socket_.send(asio::buffer(resp.dump()));
            }
        }
        catch(const nlohmann::json::parse_error& e)
        {
            E(fmt::format("Parsing message failed: {}", e.what()));
            break;
        }

        std::getline(is, s);
    }

    Exec();
}

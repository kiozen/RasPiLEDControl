#include "session.hpp"

#include <fmt/format.h>

#include "controller.hpp"

Session::Session(asio::io_context& io, Controller& parent)
    : Log("session")
    , socket_(io)
    , parent_(parent)
{
}

Session::~Session()
{
    D("~Session");
}


void Session::Exec()
{
    asio::async_read_until(socket_, buffer_, '\0', [this](const asio::error_code& error, std::size_t size){
        OnMessageReceived(error, size);
    });
}

void Session::OnMessageReceived(const asio::error_code& error, std::size_t size)
{
    if(error)
    {
        E(fmt::format("OnMessageReceived failed: {}", error.message()));
        asio::post([this](){parent_.StartServer();});
        return;
    }

    D("OnMessageReceived");
}

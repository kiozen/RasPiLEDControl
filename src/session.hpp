#ifndef SRC_SESSION_HPP
#define SRC_SESSION_HPP

#include <asio.hpp>
#include <nlohmann/json.hpp>

#include "log.hpp"

class Controller;

class Session : public Log
{
public:
    Session(asio::io_context& io, Controller& parent);
    virtual ~Session();

    asio::ip::tcp::socket& Socket(){return socket_;}

    void Exec();

    void sendPowerStatus();

private:
    void OnMessageReceived(std::shared_ptr<asio::streambuf> buffer, const asio::error_code& error, std::size_t size);
    void sendJson(const nlohmann::json& msg);


    asio::ip::tcp::socket socket_;
    asio::steady_timer timer_;

    Controller& controller_;
};

#endif // SRC_SESSION_HPP

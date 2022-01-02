#ifndef SRC_SESSION_HPP
#define SRC_SESSION_HPP

#include <asio.hpp>

#include "log.hpp"

class Controller;

class Session : public Log
{
public:
    Session(asio::io_context& io, Controller& parent);
    virtual ~Session();

    asio::ip::tcp::socket& Socket(){return socket_;}

    void Exec();

private:
    void OnMessageReceived(std::shared_ptr<asio::streambuf> buffer, const asio::error_code& error, std::size_t size);

    asio::ip::tcp::socket socket_;

    Controller& controller_;
};

#endif // SRC_SESSION_HPP

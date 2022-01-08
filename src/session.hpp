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

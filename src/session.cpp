/***********************************************************************************************
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

#include <fmt/format.h>

#include "controller.hpp"

Session::Session(asio::io_context &io, Controller &controller)
    : Log("session"), socket_(io), timer_(io), controller_(controller) {}

Session::~Session() { D("~Session"); }

void Session::Exec() {
  // timeout stall sessions
  timer_.cancel();
  timer_.expires_at(std::chrono::steady_clock::now() + std::chrono::minutes(1));
  timer_.async_wait([this](const asio::error_code &error) {
    if (error != asio::error::operation_aborted) {
      I("Session timeout");
      socket_.close();
    }
  });

  // start receiving
  std::shared_ptr<asio::streambuf> buffer(new asio::streambuf());
  asio::async_read_until(socket_, *buffer.get(), '\n',
                         [this, buffer](const asio::error_code &error, std::size_t size) {
                           OnMessageReceived(buffer, error, size);
                         });
}

void Session::Stop() {
  is_alive_.store(false);
  socket_.close();
  timer_.cancel();
}

void Session::OnMessageReceived(std::shared_ptr<asio::streambuf> buffer,
                                const asio::error_code &error, std::size_t size) {
  if (error) {
    E(fmt::format("OnMessageReceived failed: {}", error.message()));
    timer_.cancel();
    if (is_alive_.load()) {
      asio::post([this]() { controller_.StartServer(); });
    }
    return;
  }

  std::istream is(buffer.get());
  std::string s;
  std::getline(is, s);

  while (s.size()) {
    D(fmt::format("recv {}", s));

    try {
      const nlohmann::json &msg = nlohmann::json::parse(s);
      const std::string &cmd = msg["cmd"];

      if (cmd == "get_system_config") {
        nlohmann::json resp;
        resp["rsp"] = "get_system_config";
        resp["name"] = controller_.GetName();
        resp["led_count"] = controller_.GetWS2811Control().GetLedCount();
        resp["max_brightness"] = controller_.GetWS2811Control().GetMaxBrightness();
        sendJson(resp);
      } else if (cmd == "set_system_config") {
        controller_.SetName(msg["name"]);
        controller_.GetWS2811Control().SetParameters(msg["led_count"], msg["max_brightness"]);
      } else if (cmd == "get_power") {
        SendPowerStatus();
      } else if (cmd == "set_power_light") {
        controller_.GetFadeout().Stop();
        controller_.GetPower().SetChannelState(Power::kLight, msg["power"]);
      } else if (cmd == "set_power_animation") {
        controller_.GetFadeout().Stop();
        controller_.GetPower().SetChannelState(Power::kAnimation, msg["power"]);
      } else if (cmd == "get_color") {
        auto [red, green, blue] = controller_.GetLight().GetColor();
        nlohmann::json resp;
        resp["rsp"] = "get_color";
        resp["red"] = red;
        resp["green"] = green;
        resp["blue"] = blue;
        sendJson(resp);
      } else if (cmd == "set_color") {
        controller_.GetLight().SetColor(msg["red"], msg["green"], msg["blue"]);
      } else if (cmd == "set_predefined_colors") {
        controller_.GetLight().SetPredefinedColors(msg["colors"].get<ColorVector>());
      } else if (cmd == "get_predefined_colors") {
        nlohmann::json resp;
        resp["rsp"] = "get_predefined_colors";
        resp["colors"] = controller_.GetLight().GetPredefinedColors();
        sendJson(resp);
      } else if (cmd == "get_animations") {
        nlohmann::json resp;
        resp["rsp"] = "get_animations";
        resp["animations"] = controller_.GetAnimation().GetAnimationInfo();
        sendJson(resp);
      } else if (cmd == "get_animation") {
        nlohmann::json resp;
        resp["rsp"] = "get_animation";
        resp["hash"] = controller_.GetAnimation().GetAnimation();
        sendJson(resp);
      } else if (cmd == "set_animation") {
        controller_.GetAnimation().SetAnimation(msg["hash"]);
      } else if (cmd == "set_alarm") {
        Alarm::alarm_t alarm;
        alarm.name = msg["name"];
        alarm.active = msg["active"];
        alarm.hour = msg["hour"];
        alarm.minute = msg["minute"];
        alarm.days = msg["days"].get<std::set<int>>();
        alarm.animation_hash = msg["animation_hash"];
        controller_.GetAlarm().SetAlarm(alarm);
      } else if (cmd == "get_alarm") {
        const Alarm::alarm_t &alarm = controller_.GetAlarm().GetAlarm();
        nlohmann::json resp;
        resp["rsp"] = "get_alarm";
        resp["name"] = alarm.name;
        resp["active"] = alarm.active;
        resp["hour"] = alarm.hour;
        resp["minute"] = alarm.minute;
        resp["days"] = alarm.days;
        resp["animation_hash"] = alarm.animation_hash;
        sendJson(resp);
      } else if (cmd == "set_timeout") {
        controller_.GetFadeout().SetTimeout(msg["target"], std::chrono::minutes(msg["minutes"]));
      }
    } catch (const nlohmann::json::exception &e) {
      E(fmt::format("Parsing message failed: {}", e.what()));
      break;
    }

    std::getline(is, s);
  }

  Exec();
}

void Session::sendJson(const nlohmann::json &msg) {
  if (socket_.is_open()) {
    D(fmt::format("send: {}", msg.dump()));
    socket_.send(asio::buffer(msg.dump() + "\n"));
  }
}

void Session::SendPowerStatus() {
  nlohmann::json resp;
  const Power &power = controller_.GetPower();
  resp["rsp"] = "get_power";
  resp["light"] = power.GetChannelState(Power::kLight);
  resp["animation"] = power.GetChannelState(Power::kAnimation);

  const Fadeout &fadeout = controller_.GetFadeout();
  resp["timeout_active"] = fadeout.GetTimeoutActive();
  sendJson(resp);
}

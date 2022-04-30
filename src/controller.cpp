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
#include "controller.hpp"

#include <filesystem>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include "session.hpp"

#ifndef _MKSTR_1
#define _MKSTR_1(x) #x
#define _MKSTR(x) _MKSTR_1(x)
#endif

#define VER_STR _MKSTR(VER_MAJOR) "." _MKSTR(VER_MINOR) "." _MKSTR(VER_STEP)
#define WHAT_STR _MKSTR(APPLICATION_NAME) ", Version " VER_STR

Controller::Controller() : Log("ctrl") {
  I(fmt::format("-------------- {} --------------", WHAT_STR));
  const std::filesystem::path path{kConfigPath};
  std::filesystem::create_directories(path);

  power_.SigPowerStatusChanged.connect(&Controller::OnPowerStatusChanged, this);

  restore_state_active_ = true;
  try {
    const nlohmann::json &cfg = LoadState(kConfigPath, kConfigFile);

    name_ = cfg.value("name", "");
  } catch (const nlohmann::json::exception &e) {
    E(fmt::format("Parsing system config failed: {}", e.what()));
  }
  restore_state_active_ = false;
}

Controller::~Controller() {}

int Controller::Exec() {
  sig_.async_wait(
      [this](const asio::error_code &error, int signal_number) { OnSignal(error, signal_number); });

  if (!SetupUdp()) {
    return -1;
  }

  if (!StartServer()) {
    return -1;
  }

  D("*** start asio loop ***");
  io_.run();

  return 0;
}

bool Controller::SetupUdp() {
  udp_socket_.open(asio::ip::udp::v4());
  asio::socket_base::broadcast option(true);
  udp_socket_.set_option(option);
  udp_socket_.bind(asio::ip::udp::endpoint(asio::ip::address_v4::any(), kPort));

  struct ifreq s;
  strcpy(s.ifr_name, "wlan0");
  if (0 == ioctl(udp_socket_.native_handle(), SIOCGIFHWADDR, &s)) {
    const char *mac = s.ifr_addr.sa_data;
    mac_ = fmt::format("{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}", mac[0], mac[1], mac[2], mac[3],
                       mac[4], mac[5]);
    D(fmt::format("MAC address is {}", mac_));
  } else {
    E(fmt::format("Failed to read MAC address from wlan0: {}", strerror(errno)));
  }

  udp_socket_.async_receive_from(
      asio::buffer(recv_buffer_), remote_endpoint_,
      [this](const asio::error_code &error, std::size_t size) { OnReceiveUdp(error, size); });

  return true;
}

void Controller::SetName(const std::string &name) {
  name_ = name;
  SaveState();
}

void Controller::SaveState() {
  nlohmann::json cfg;
  cfg["name"] = name_;

  IModule::SaveState(kConfigPath, kConfigFile, cfg);
}

void Controller::OnSignal(const asio::error_code &error, int signal_number) {
  I(fmt::format("Controller stopped with signal {}", signal_number));

  is_alive_.store(false);
  udp_socket_.close();
  acceptor_.close();
  session_->Stop();
  animation_.Play(false);
  alarm_.Stop();
  fadeout_.Stop();
}

void Controller::OnReceiveUdp(const asio::error_code &error, std::size_t size) {
  if (!error && (size < recv_buffer_.size())) {
    recv_buffer_[size] = 0;
    try {
      const nlohmann::json &msg = nlohmann::json::parse(recv_buffer_);
      if (msg["cmd"] == "identify") {
        nlohmann::json resp;
        resp["rsp"] = "identify";
        resp["name"] = name_;
        resp["mac"] = mac_;

        udp_socket_.send_to(asio::buffer(resp.dump()), remote_endpoint_);
      }
    } catch (const std::exception &e) {
      E(fmt::format("Parsing message failed: {}", e.what()));
    }
  } else {
    E(fmt::format("On receive UDP failed: {}", error.message()));
  }

  if (is_alive_.load()) {
    udp_socket_.async_receive_from(
        asio::buffer(recv_buffer_), remote_endpoint_,
        [this](const asio::error_code &error, std::size_t size) { OnReceiveUdp(error, size); });
  }
}

bool Controller::StartServer() {
  session_ = std::unique_ptr<Session>(new Session(io_, *this));
  acceptor_.async_accept(session_->Socket(), [this](const asio::error_code &error) {
    if (!error) {
      D("New connection");
      session_->Exec();
    } else {
      E(fmt::format("Server failed: {}", error.message()));
    }
  });
  return true;
}

void Controller::OnPowerStatusChanged() {
  D("OnPowerStatusChanged");
  if (session_) {
    session_->SendPowerStatus();
  }
}

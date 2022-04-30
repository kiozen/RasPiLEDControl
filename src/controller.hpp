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
#ifndef SRC_CONTROLER_HPP
#define SRC_CONTROLER_HPP

#include <asio.hpp>

#include "alarm.hpp"
#include "animation.hpp"
#include "fadeout.hpp"
#include "i_module.hpp"
#include "light.hpp"
#include "log.hpp"
#include "power.hpp"
#include "ws2811_control.hpp"

class Controller : public Log, public IModule {
public:
  Controller();
  virtual ~Controller();

  int Exec();
  bool StartServer();
  void SetName(const std::string &name);
  const std::string &GetName() const { return name_; }

  WS2811Control &GetWS2811Control() { return ws2811_control_; }
  Power &GetPower() { return power_; }
  Light &GetLight() { return light_; }
  Animation &GetAnimation() { return animation_; }
  Alarm &GetAlarm() { return alarm_; }
  Fadeout &GetFadeout() { return fadeout_; }

private:
  static constexpr uint16_t kPort = 7755;
  static constexpr const char *kConfigPath = "/home/pi/.config/led_control/";
  static constexpr const char *kConfigFile = "controller.json";

  void OnSignal(const asio::error_code &error, int signal_number);
  void OnReceiveUdp(const asio::error_code &error, std::size_t size);
  void OnPowerStatusChanged();

  bool SetupUdp();
  void SaveState() override;

  std::atomic_bool is_alive_{true};

  asio::io_context io_;
  asio::signal_set sig_ = {io_, SIGINT, SIGTERM};

  asio::ip::udp::socket udp_socket_{io_};
  asio::ip::udp::endpoint remote_endpoint_;
  std::array<int8_t, 1024> recv_buffer_;

  asio::ip::tcp::endpoint endpoint_{asio::ip::tcp::v4(), 7756};
  asio::ip::tcp::acceptor acceptor_{io_, endpoint_};
  std::unique_ptr<class Session> session_;

  std::string name_;
  std::string mac_;

  WS2811Control ws2811_control_{kConfigPath};
  Power power_{kConfigPath, ws2811_control_};
  Fadeout fadeout_{io_, power_, ws2811_control_};

  Light light_{kConfigPath, power_};
  Animation animation_{io_, power_};
  Alarm alarm_{kConfigPath, io_, power_, animation_};
};

#endif // SRC_CONTROLER_HPP

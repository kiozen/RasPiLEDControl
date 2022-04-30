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
#ifndef SRC_ANIMATION_HPP
#define SRC_ANIMATION_HPP

#include <asio.hpp>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <tuple>
#include <ws2811/ws2811.h>

#include "log.hpp"

class Power;

class Animation : public Log {
public:
  Animation(asio::io_context &io, Power &power);
  virtual ~Animation();

  nlohmann::json GetAnimationInfo() const;

  void SetAnimation(const std::string &hash);
  std::string GetAnimation() const { return hash_; }

  void Play(bool on);

private:
  void LoadAnimation(const std::string &filename);
  void OnAnimate(const asio::error_code &error);
  void OnPowerStatusChanged();

  enum class mode_e { single, cyclic };
  static constexpr const char *kAnimationPath = "/home/pi";

  asio::steady_timer timer_;
  Power &power_;

  std::string hash_;
  using animation_step_t = std::tuple<int, std::vector<ws2811_led_t>>;
  using animation_t = std::vector<animation_step_t>;
  animation_t animation_;
  int index_{0};
  mode_e mode_{mode_e::single};
  bool active_{false};

  struct info_t {
    mode_e mode{mode_e::single};
    std::string name;
    std::string desc;
    std::filesystem::path path;
  };

  std::map<std::string, info_t> animations_;
};

#endif // SRC_ANIMATION_HPP

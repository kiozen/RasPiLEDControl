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
#include "animation.hpp"

#include <fmt/format.h>
#include <fstream>
#include <openssl/md5.h>

#include "power.hpp"

Animation::Animation(asio::io_context &io, Power &power)
    : Log("animation"), timer_(io), power_(power) {

  power_.SigPowerStatusChanged.connect(&Animation::OnPowerStatusChanged, this);

  for (const auto &p : std::filesystem::directory_iterator(kAnimationPath)) {
    const std::filesystem::path &path = p.path();

    if (path.extension() != ".json") {
      continue;
    }

    try {
      std::ifstream file(path);
      nlohmann::json animation;
      file >> animation;

      const std::string &name = animation["name"];
      const std::string &description = animation["description"];
      const std::string &content = name + description;

      mode_e mode = mode_e::single;
      const std::string &m = animation.value("mode", "single");
      if (m == "single") {
        mode = mode_e::single;
      } else if (m == "cyclic") {
        mode = mode_e::cyclic;
      }

      unsigned char buffer[MD5_DIGEST_LENGTH];
      MD5((unsigned char *)content.c_str(), content.size(), buffer);

      std::string hash;
      for (std::size_t i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        hash += "0123456789ABCDEF"[buffer[i] / 16];
        hash += "0123456789ABCDEF"[buffer[i] % 16];
      }

      animations_[hash] = {mode, name, description, path};

      D(fmt::format("Found animation '{} {} {}'.", animation["name"], hash,
                    animation.value("mode", "single")));
    } catch (const nlohmann::json::exception &e) {
      E(fmt::format("Parsing animation {} failed: {}", path.c_str(), e.what()));
    }
  }
}

Animation::~Animation() {}

void Animation::OnPowerStatusChanged() {
  I("OnPowerStatusChanged");
  bool active = power_.GetChannelState(Power::kAnimation);
  if (active_ != active) {
    Play(active);
  }
}

void Animation::Play(bool on) {
  if (on) {
    if (animation_.empty()) {
      E("animation is empty");
      return;
    }
    active_ = true;
    index_ = 0;
    timer_.expires_at(std::chrono::steady_clock::now() + std::chrono::milliseconds(100));
    timer_.async_wait([this](const asio::error_code &error) { OnAnimate(error); });

  } else {
    timer_.cancel();
  }
}

nlohmann::json Animation::GetAnimationInfo() const {
  std::vector<nlohmann::json> infos;

  for (const auto &animation : animations_) {
    auto [hash, info] = animation;
    nlohmann::json json;
    json["name"] = info.name;
    json["description"] = info.desc;
    json["hash"] = hash;

    infos.push_back(json);
  }

  return nlohmann::json(infos);
}

void Animation::SetAnimation(const std::string &hash) {
  if (animations_.count(hash) == 0) {
    hash_.clear();
    animation_.clear();
    index_ = -1;
    return;
  }
  if (hash_ != hash) {
    hash_ = hash;
    mode_ = animations_[hash].mode;
    LoadAnimation(animations_[hash].path.c_str());
  }
}

void Animation::LoadAnimation(const std::string &filename) {
  I(fmt::format("Load animation {}", filename));
  std::ifstream ifs(filename);
  const nlohmann::json &json_ = nlohmann::json::parse(ifs);
  animation_ = json_["data"].get<animation_t>();
  index_ = 0;
}

void Animation::OnAnimate(const asio::error_code &error) {
  if (error) {
    E(fmt::format("Cyclic loop failed: {}", error.message()));
    active_ = false;
    power_.SetChannelState(Power::kAnimation, false);
    return;
  }

  if (index_ == animation_.size()) {
    if (mode_ == mode_e::cyclic) {
      index_ = 0;
    } else {
      active_ = false;
      power_.SetChannelState(Power::kAnimation, false);
      return;
    }
  }
  const auto &[time, frame] = animation_[index_++];
  power_.SetChannelFrame(Power::kAnimation, frame);
  timer_.expires_at(timer_.expiry() + std::chrono::milliseconds(time));
  timer_.async_wait([this](const asio::error_code &error) { OnAnimate(error); });
}

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
#ifndef SRC_I_MODULE_HPP
#define SRC_I_MODULE_HPP

#include <nlohmann/json.hpp>
#include <string>

class IModule {
public:
  IModule() = default;
  virtual ~IModule() = default;

  virtual void SaveState() = 0;

protected:
  void SaveState(const std::string &path, const std::string &filename, const nlohmann::json &json);
  nlohmann::json LoadState(const std::string &path, const std::string &filename);

  bool restore_state_active_{false};
};

#endif // SRC_I_MODULE_HPP

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
#include "i_module.hpp"

#include <filesystem>
#include <fstream>

void IModule::SaveState(const std::string &path, const std::string &filename,
                       const nlohmann::json &json) {

  if (restore_state_active_) {
    return;
  }
  std::filesystem::path filepath{path};
  filepath /= filename;

  std::ofstream file(filepath);
  file << json;
  file.flush();
}

nlohmann::json IModule::LoadState(const std::string &path, const std::string &filename) {
  std::filesystem::path filepath{path};
  filepath /= filename;

  std::ifstream file(filepath);

  nlohmann::json cfg;
  file >> cfg;
  return cfg;
}

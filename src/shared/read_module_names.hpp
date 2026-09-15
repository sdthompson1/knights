/*
 * read_module_names.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2026.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Knights is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Knights.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef READ_MODULE_NAMES_HPP
#define READ_MODULE_NAMES_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

// Checks whether the given string is valid as a module folder name.
bool IsValidModuleName(const std::string &name);

// Read modules.txt from the prefs directory, and return module names
// in file order, deduplicated. If file not found, returns empty vector.
std::vector<std::string> ReadModuleNames(const std::optional<std::filesystem::path> &pref_path);

// Save the given modules list back to the prefs directory. Overwrites
// any current list.
void WriteModuleNames(const std::optional<std::filesystem::path> &pref_path,
                      const std::vector<std::string> &names);

#endif

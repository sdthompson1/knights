/*
 * read_module_names.cpp
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

#include "misc.hpp"

#include "read_module_names.hpp"
#include "trim.hpp"

#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

bool IsValidModuleName(const std::string &name)
{
    if (name.empty() || !std::isalpha(static_cast<unsigned char>(name[0]))) return false;
    for (size_t i = 1; i < name.size(); ++i) {
        char c = name[i];
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') return false;
    }
    return true;
}

std::vector<std::string> ReadModuleNames(const std::optional<std::filesystem::path> &pref_path)
{
    std::vector<std::string> result;

    if (pref_path) {
        std::ifstream f(*pref_path / "modules.txt");

        std::unordered_set<std::string> seen;
        std::string line;

        while (std::getline(f, line)) {
            // Strip comment
            auto hash_pos = line.find('#');
            if (hash_pos != std::string::npos) line.erase(hash_pos);

            line = Trim(line);
            if (line.empty()) continue;

            if (IsValidModuleName(line)) {
                if (seen.insert(line).second) {
                    result.push_back(line);
                }
            }
        }
    }

    return result;
}

void WriteModuleNames(const std::optional<std::filesystem::path> &pref_path,
                      const std::vector<std::string> &names)
{
    if (pref_path) {
        std::ofstream f(*pref_path / "modules.txt");
        for (const std::string &name : names) {
            f << name << "\n";
        }
    }
}

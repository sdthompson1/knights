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
#include "vfs.hpp"

#include <cctype>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

bool IsValidModuleName(const std::string &name)
{
    if (name.empty() || !std::isalpha(static_cast<unsigned char>(name[0]))) return false;
    for (size_t i = 1; i < name.size(); ++i) {
        char c = name[i];
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') return false;
    }
    return true;
}

std::vector<std::string> ReadModuleNames(const VFS &vfs, const std::string &filename)
{
    if (!vfs.exists(filename)) return {};

    std::ifstream f = vfs.open(filename);

    std::vector<std::string> result;
    std::unordered_set<std::string> seen;
    std::string line;

    while (std::getline(f, line)) {
        // Strip comment
        auto hash_pos = line.find('#');
        if (hash_pos != std::string::npos) line.erase(hash_pos);

        line = Trim(line);
        if (line.empty()) continue;

        if (!IsValidModuleName(line))
            throw std::runtime_error("Invalid module name in '" + filename + "': '" + line + "'");

        if (seen.insert(line).second) result.push_back(line);
    }

    return result;
}

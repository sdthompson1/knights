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

#include <string>
#include <vector>

class VFS;

struct ModuleNameList {
    std::vector<std::string> names;      // hard / required dependencies
    std::vector<std::string> soft_names; // optional dependencies (prefixed with '*' in file)
};

bool IsValidModuleName(const std::string &name);

// Reads a given file from the vfs and parses module names from it.
// Names prefixed with '*' are returned in soft_names; all others in names.
// Both vectors are sorted and deduplicated.
// If filename doesn't exist, returns an empty ModuleNameList.
ModuleNameList ReadModuleNames(const VFS &vfs, const std::string &filename);

#endif

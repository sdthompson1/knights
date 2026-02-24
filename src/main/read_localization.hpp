/*
 * read_localization.hpp
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

#ifndef READ_LOCALIZATION_HPP
#define READ_LOCALIZATION_HPP

#include <string>
#include <vector>

class Localization;
class OnlinePlatform;
class VFS;

// Read localization keys from a file named <prefix> + <language> +
// <suffix>, in the given VFS, for each of the given languages.
//
// If the same key is available in multiple languages, the leftmost
// language in the list will take priority.
//
// If the same key appears more than once in any single language file,
// then that is an error.
//
// If any key read from the file(s) already exists in the Localization
// object then it will be overwritten.
//
void ReadLocalization(Localization &localization,
                      const VFS &vfs,
#ifdef ONLINE_PLATFORM
                      OnlinePlatform &online_platform,
#endif
                      const std::string &prefix,
                      const std::string &suffix,
                      const std::vector<std::string> &languages);

#endif

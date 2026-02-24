/*
 * read_localization.cpp
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

#include "localization.hpp"
#include "online_platform.hpp"
#include "utf8string.hpp"
#include "vfs.hpp"

void ReadLocalization(Localization &localization,
                      const VFS &vfs,
#ifdef ONLINE_PLATFORM
                      OnlinePlatform &online_platform,
#endif
                      const std::string &prefix,
                      const std::string &suffix,
                      const std::vector<std::string> &languages)
{
    auto filter = [&](const UTF8String &str) {
#ifdef ONLINE_PLATFORM
        return online_platform.filterGameContent(str);
#else
        return str;
#endif
    };

    // Read the language files in reverse order, so that left-most
    // ones overwrite right-most ones.
    for (auto it = languages.rbegin(); it != languages.rend(); ++it) {
        std::string filename = prefix + *it + suffix;
        if (vfs.exists(filename)) {
            std::ifstream file = vfs.open(filename);
            localization.readStrings(file, filter);
        }
    }
}

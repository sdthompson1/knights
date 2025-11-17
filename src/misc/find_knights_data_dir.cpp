/*
 * find_knights_data_dir.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2025.
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

#include "find_knights_data_dir.hpp"

#include <iostream>

std::filesystem::path FindKnightsDataDir()
{
    std::filesystem::path p = "knights_data";

#ifdef DATA_DIR
#define _QUOTEME(x) #x
#define QUOTEME(x) _QUOTEME(x)
    std::filesystem::path p2 = QUOTEME(DATA_DIR);

    // In the case where p doesn't exist, but p2 does, we want to use p2 instead of p
    if (std::filesystem::exists(p2) && !std::filesystem::exists(p)) {
        p = p2;
    }
#endif

    return p;
}

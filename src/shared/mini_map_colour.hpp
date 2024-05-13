/*
 * mini_map_colour.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2024.
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

#ifndef MINI_MAP_COLOUR_HPP
#define MINI_MAP_COLOUR_HPP

enum MiniMapColour {
    // NOTE: If you change these, also change the colour definitions in mini_map.cpp.
    COL_WALL = 4,       // Tiles of this colour are mapped by Magic Mapping scrolls.
    COL_FLOOR,
    COL_UNMAPPED,
};

#endif

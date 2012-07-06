/*
 * coord_transform.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2012.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#ifndef COORD_TRANSFORM_HPP
#define COORD_TRANSFORM_HPP

#include "map_support.hpp"

#include <vector>

// This class keeps track of dungeon generator reflections/rotations (Trac #41)

class CoordTransform {
public:
    void add(const MapCoord &corner, int width, int height, bool x_reflect, int nrot);
    void clear();

    void transformOffset(const MapCoord &base, int &x, int &y) const;
    void transformDirection(const MapCoord &base, MapDirection &dir) const;

private:
    struct Zone {
        MapCoord corner;
        int width;
        int height;
        bool x_reflect;
        int nrot;
    };
    std::vector<Zone> zones;

    static bool inZone(const MapCoord &mc, const Zone &z);
};

#endif

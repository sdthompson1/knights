/*
 * coord_transform.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
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

#include "misc.hpp"

#include "coord_transform.hpp"

void CoordTransform::add(const MapCoord &corner, int width, int height, bool x_reflect, int nrot)
{
    Zone z;
    z.corner = corner;
    z.width = width;
    z.height = height;
    z.x_reflect = x_reflect;
    z.nrot = nrot;
    zones.push_back(z);
}

void CoordTransform::transformOffset(const MapCoord &base, int &x, int &y) const
{
    for (std::vector<Zone>::const_iterator it = zones.begin(); it != zones.end(); ++it) {
        if (inZone(base, *it)) {

            if (it->x_reflect) {
                x = - x;
            }

            for (int i = 0; i < it->nrot; ++i) {
                const int x_old = x;
                x = - y;
                y = x_old;
            }

            break;
        }
    }
}

void CoordTransform::transformDirection(const MapCoord &base, MapDirection &dir) const
{
    for (std::vector<Zone>::const_iterator it = zones.begin(); it != zones.end(); ++it) {
        if (inZone(base, *it)) {

            if (it->x_reflect) {
                if (dir == D_WEST) dir = D_EAST;
                else if (dir == D_EAST) dir = D_WEST;
            }

            for (int i = 0; i < it->nrot; ++i) {
                dir = Clockwise(dir);
            }

            break;
        }
    }
}

bool CoordTransform::inZone(const MapCoord &mc, const Zone &z)
{
    return (mc.getX() >= z.corner.getX() && mc.getX() < z.corner.getX() + z.width
            && mc.getY() >= z.corner.getY() && mc.getY() < z.corner.getY() + z.height);
}

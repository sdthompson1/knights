/*
 * monster_support.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2013.
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

#include "dungeon_map.hpp"
#include "monster_support.hpp"

bool KnightAt(DungeonMap &dmap, const MapCoord &mc, const std::vector<ItemType *> &fear)
{
    vector<shared_ptr<Entity> > ents;
    dmap.getEntities(mc, ents);
    for (vector<shared_ptr<Entity> >::iterator it = ents.begin(); it != ents.end(); ++it) {
        Knight * kt = dynamic_cast<Knight*>(it->get());
        if (kt && (std::find(fear.begin(), fear.end(), kt->getItemInHand()) == fear.end())) {
            return true;
        }
    }
    return false;
}


MapDirection DirectionFromTo(const MapCoord &from, const MapCoord &to)
{
    int dx = to.getX() - from.getX();
    int dy = to.getY() - from.getY();
    if (abs(dx) > abs(dy)) {
        if (dx > 0) return D_EAST;
        else return D_WEST;
    } else {
        if (dy > 0) return D_SOUTH;
        else return D_NORTH;
    }
}

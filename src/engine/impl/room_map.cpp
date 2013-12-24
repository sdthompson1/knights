/*
 * room_map.cpp
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

#include "my_exceptions.hpp"
#include "room_map.hpp"
#include "rng.hpp"

#include <algorithm>
using namespace std;

RoomMap::RoomMap()
    : ready(false)
{ }

void RoomMap::addRoom(const MapCoord &top_left, int w, int h)
{
    if (ready) throw InitError("RoomMap: addRoom after doneAddingRooms");
    RoomInfo ri;
    ri.pos = top_left;
    ri.w = w;
    ri.h = h;
    rooms.push_back(ri);
}

void RoomMap::doneAddingRooms()
{
    ready = true;

    // We randomize the order of the rooms. This means that the room
    // numbers (which are sent out to clients) are unpredictable and
    // do not give away any information.
    RNG_Wrapper myrng(g_rng);
    random_shuffle(rooms.begin(), rooms.end(), myrng);        
}

void RoomMap::getRoomAtPos(const MapCoord &mc, int &r1, int &r2) const
{
    // We just search through the room list until we find
    // something. This is inefficient -- in future might want to
    // create an index of some sort, so we can just look up the
    // MapCoord directly.

    r1 = r2 = -1;
        
    for (vector<RoomInfo>::const_iterator it = rooms.begin(); it != rooms.end(); ++it) {
        if (mc.getX() >= it->pos.getX() && mc.getX() < it->pos.getX() + it->w
        && mc.getY() >= it->pos.getY() && mc.getY() < it->pos.getY() + it->h) {
            // Found the room.
            const bool x_corner = mc.getX() == it->pos.getX() || mc.getX() == it->pos.getX() + it->w - 1;
            const bool y_corner = mc.getY() == it->pos.getY() || mc.getY() == it->pos.getY() + it->h - 1;
            if (x_corner && y_corner) continue;  // exclude the corners
            if (r1 == -1) {
                r1 = (it - rooms.begin());
            } else {
                r2 = (it - rooms.begin());
                return;
            }
        }
    }
}

bool RoomMap::isCorner(const MapCoord &mc) const
{
    for (vector<RoomInfo>::const_iterator it = rooms.begin(); it != rooms.end(); ++it) {
        if ((mc.getX()==it->pos.getX() || mc.getX() == it->pos.getX() + it->w - 1)
            && (mc.getY()==it->pos.getY() || mc.getY() == it->pos.getY() + it->h - 1)) {
            return true;
        }
    }
    return false;
}

bool RoomMap::inSameRoom(const MapCoord &mc1, const MapCoord &mc2) const
{
    int r1a, r1b;
    int r2a, r2b;
    getRoomAtPos(mc1, r1a, r1b);
    getRoomAtPos(mc2, r2a, r2b);
    if (r1a != -1 && (r1a == r2a || r1a == r2b)) return true;
    if (r1b != -1 && (r1b == r2a || r1b == r2b)) return true;
    return false;
}

void RoomMap::getRoomLocation(int r, MapCoord &top_left, int &w, int &h) const
{
    if (r < 0 || r >= rooms.size()) {
        top_left = MapCoord();
        w = 0;
        h = 0;
    } else {
        top_left = rooms[r].pos;
        w = rooms[r].w;
        h = rooms[r].h;
    }
}


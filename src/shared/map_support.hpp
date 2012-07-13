/*
 * map_support.hpp
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

/*
 * Support classes for DungeonMap.
 *
 */

#ifndef MAP_SUPPORT_HPP
#define MAP_SUPPORT_HPP

#include "boost/shared_ptr.hpp"

struct lua_State;


//
// MapCoord
//

class MapCoord {
public:
    MapCoord() : x(-2), y(-2) { } // construct "null" mapcoord
    MapCoord(int x_, int y_) : x(x_), y(y_) { }

    bool isNull() const { return x==-2; }
    bool operator==(const MapCoord &other) const { return x==other.x && y==other.y; }
    bool operator!=(const MapCoord &other) const { return x!=other.x || y!=other.y; }
    bool operator<(const MapCoord &rhs) const {
        return y < rhs.y || (y == rhs.y && x < rhs.x);
    }

    // Coord system: (0,0) is top left; (w-1,h-1) is bottom right. 
    // (For null mapcoords, getX() and getY() are undefined.)
    int getX() const { return x; }
    int getY() const { return y; }

    // modifiers
    void setX(int x_) { x = x_; }
    void setY(int y_) { y = y_; }

private:
    // invalid positions are currently represented as (-2,-2).
    int x, y;
};

size_t hash_value(const MapCoord &);

// get from lua (raises lua error on failure)
MapCoord GetMapCoord(lua_State *lua, int index);


//
// MapDirection -- the 4 compass points
//

enum MapDirection {
    D_NORTH, D_EAST, D_SOUTH, D_WEST
};

// get from lua (raises lua error on failure)
MapDirection GetMapDirection(lua_State *lua, int index);


//
// MapHeight -- measures height above ground of an entity
//
// NB: order is important -- rightmost is highest. (Eg this is
// currently used by the combat system to target "higher" entities
// first.) Also, H_MISSILES must always come last.
//
// NB: H_MISSILES is special. There are actually four H_MISSILES
// heights, determined by the facing direction, ie H_MISSILES+D_NORTH
// thru H_MISSILES+D_WEST. (This is what the dummy values below are
// for.) This is because missiles facing in different directions can
// actually pass through each other (and therefore are logically at
// different heights). Note that for MapAccess purposes these four
// heights are all considered equivalent to just H_MISSILES.
//

enum MapHeight {
    H_WALKING, H_FLYING, H_MISSILES, H_DUMMY1, H_DUMMY2, H_DUMMY3
};



//
// MapAccess -- describes whether squares can be entered by entities
//

enum MapAccess {
    A_BLOCKED, A_APPROACH, A_CLEAR
};


//
// MotionType
//

enum MotionType {
    // NOTE: In the messages SERVER_SET_ANIM_DATA and SERVER_ADD_ENTITY,
    // only 2 bits are reserved for MotionType. Therefore, be careful if adding
    // any new entries to this enum.
    MT_NOT_MOVING,
    MT_MOVE,
    MT_APPROACH,
    MT_WITHDRAW
};



//
// various free functions
//

MapDirection Opposite(MapDirection m);
MapDirection Clockwise(MapDirection m);
MapDirection Anticlockwise(MapDirection m);
MapCoord DisplaceCoord(const MapCoord &base, MapDirection dir, int amount = 1);

#endif

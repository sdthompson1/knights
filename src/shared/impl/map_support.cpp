/*
 * map_support.cpp
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

#include "misc.hpp"

#include "map_support.hpp"
#include "my_ctype.hpp"

#include "include_lua.hpp"

namespace {
    // check if two C strings are equal (case insensitive)
    bool EqualNoCase(const char *p, const char *q)
    {
        while (1) {
            if (*p == 0 && *q == 0) return true;
            if (*p == 0 || *q == 0) return false;
            if (ToUpper(*p) != ToUpper(*q)) return false;
            ++p;
            ++q;
        }
    }
}


size_t hash_value(const MapCoord &mc)
{
    return (mc.getX() << 16) + (mc.getY());
}


// Read a position (MapCoord) from the lua stack at the given index. (Should be a table or nil.)
// raise lua error if it is not a valid map coord
MapCoord GetMapCoord(lua_State *lua, int index)
{
    if (lua_isnil(lua, index)) {
        return MapCoord();
    } else {
        int isnum;
        
        lua_getfield(lua, index, "x");   // raises error if field couldn't be found.
        int x = lua_tointegerx(lua, -1, &isnum);
        lua_pop(lua, 1);
        
        int y = -1;
        if (isnum) {
            lua_getfield(lua, index, "y");   // raises error if field couldn't be found.
            y = lua_tointegerx(lua, -1, &isnum);
            lua_pop(lua, 1);
        }
        
        if (!isnum) {
            luaL_error(lua, "Argument #%d is not a valid map position", index);
        }
        
        return MapCoord(x, y);
    }
}

// Read a map direction from the given lua index
// raise a lua error if it is not a valid direction string
MapDirection GetMapDirection(lua_State *lua, int index)
{
    const char * x = lua_tostring(lua, index);
    if (x) {
        if (EqualNoCase(x, "south") || EqualNoCase(x, "down")) return D_SOUTH;
        else if (EqualNoCase(x, "west") || EqualNoCase(x, "left")) return D_WEST;
        else if (EqualNoCase(x, "east") || EqualNoCase(x, "right")) return D_EAST;
        else if (EqualNoCase(x, "north") || EqualNoCase(x, "up")) return D_NORTH;
    }
    if (x==0) x="<non-string value>";
    luaL_error(lua, "'%s' is not a valid map direction", x);
    return D_NORTH;  // avoid compiler warning
}


MapDirection Opposite(MapDirection m)
{
    return MapDirection((int(m) + 2) % 4);
}

MapDirection Clockwise(MapDirection m)
{
    return MapDirection((int(m) + 1) % 4);
}

MapDirection Anticlockwise(MapDirection m)
{
    return MapDirection((int(m) + 3) % 4);
}

MapCoord DisplaceCoord(const MapCoord &base, MapDirection dir, int amt)
{
    switch (dir) {
    case D_EAST:
        return MapCoord(base.getX() + amt, base.getY());
    case D_NORTH:
        return MapCoord(base.getX(), base.getY() - amt);
    case D_SOUTH:
        return MapCoord(base.getX(), base.getY() + amt);
    case D_WEST:
        return MapCoord(base.getX() - amt, base.getY());
    default:
        return base;
    }
}

/*
 * create_tile.cpp
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

#include "create_tile.hpp"
#include "my_ctype.hpp"
#include "special_tiles.hpp"

#include "lua.hpp"

#include <string>

boost::shared_ptr<Tile> CreateTile(lua_State *lua)
{
    // [t]
    lua_getfield(lua, -1, "type");  // [t type]
    std::string s;
    const char *p = lua_tostring(lua, -1);
    if (p) s = p;

    lua_pop(lua, 1);  // [t]

    for (std::string::iterator it = s.begin(); it != s.end(); ++it) {
        *it = ToLower(*it);
    }

    boost::shared_ptr<Tile> tile;
    
    if (s == "") {
        tile.reset(new Tile(lua));
    } else if (s == "home") {
        tile.reset(new Home(lua));
    } else if (s == "door") {
        tile.reset(new Door(lua));
    } else if (s == "chest") {
        tile.reset(new Chest(lua));
    } else if (s == "barrel") {
        tile.reset(new Barrel(lua));
    } else {
        luaL_error(lua, "Unknown tile type '%s'", s.c_str());
    }

    return tile;
}

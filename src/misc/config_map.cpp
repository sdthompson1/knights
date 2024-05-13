/*
 * config_map.cpp
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

#include "misc.hpp"

#include "config_map.hpp"

#include "lua.hpp"

void PopConfigMap(lua_State *lua, ConfigMap &cmap)
{
    if (!lua_istable(lua, -1)) {
        throw LuaError("MISC_CONFIG table not found");
    }

    lua_pushnil(lua);  // table is now at index -2, key at -1
    while (lua_next(lua, -2) != 0) {
        // new key is now at index -2, value at index -1
        if (lua_type(lua, -2) == LUA_TSTRING) {
            const std::string key = lua_tostring(lua, -2);
            if (lua_type(lua, -1) == LUA_TNUMBER) {
                const int as_int = lua_tointeger(lua, -1);
                const double as_double = lua_tonumber(lua, -1);
                if (double(as_int) == as_double) {
                    // conversion to int & back to double didn't change it, so store as int
                    cmap.setInt(key, as_int);
                } else {
                    // store as float
                    cmap.setFloat(key, float(as_double));
                }
            } else if (lua_type(lua, -1) == LUA_TSTRING) {
                cmap.setString(key, lua_tostring(lua, -1));
            } else {
                throw LuaError(std::string("error in MISC_CONFIG: field '")
                    + key = "' has incorrect type");
            }
        } else {
            throw LuaError("error in MISC_CONFIG: non-string key detected");
        }
        lua_pop(lua, 1);  // pop value (leave key)
    }
    lua_pop(lua, 1);   // pop table
}

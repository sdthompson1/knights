/*
 * lua_module.cpp
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

#include "misc.hpp"

#include "lua_load_from_rstream.hpp"
#include "lua_module.hpp"
#include "rstream.hpp"

#include "lua.hpp"

namespace {

    int DoFile(lua_State *lua)
    {
        // This is essentially a Lua wrapper around LuaExecFromRStream.

        // Expect 1 string arg (file name to load)
        // Return a variable number of results (the chunk return values).

        const char * filename = luaL_checkstring(lua, 1);

        // empty the stack
        lua_settop(lua, 0);

        // execute the file (convert the lua string directly to a path.)
        LuaExecRStream(lua, filename, LUA_MULTRET);

        // return the actual no of results.
        return lua_gettop(lua);
    }

}

void AddModuleFuncs(lua_State *lua)
{
    // Global "dofile" function
    lua_pushcfunction(lua, &DoFile);
    lua_setglobal(lua, "dofile");
}

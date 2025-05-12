/*
 * lua_check.cpp
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

#include "lua_check.hpp"

#include "include_lua.hpp"

namespace {
    bool HasMetamethod(lua_State *lua, int idx, const char *method_name)
    {
        if (!lua_getmetatable(lua, idx)) {
            // [...]
            return false;
        }

        // [... meta]
        lua_pushstring(lua, method_name);  // [... meta __call]
        lua_gettable(lua, -2);             // [... meta callobj]

        // We assume that if they have put something in the
        // metamethod, then they know what they are doing, i.e. don't
        // bother checking that the metamethod is valid in any way.
        const bool result = !lua_isnil(lua, -1);

        lua_pop(lua, 2);  // [...]
        return result;
    }
}

bool LuaIsCallable(lua_State *lua, int idx)
{
    return lua_isfunction(lua, idx) || HasMetamethod(lua, idx, "__call");
}

void LuaCheckCallable(lua_State *lua, int arg, const char *msg)
{
    if (!LuaIsCallable(lua, arg)) {
        luaL_error(lua, "%s: Argument #%d is not a function or callable object", msg, arg);
    }
}

void LuaCheckIndexable(lua_State *lua, int arg, const char *msg)
{
    if (!lua_istable(lua, arg) && !HasMetamethod(lua, arg, "__index")) {
        luaL_error(lua, "%s: Argument #%d is not a table or indexable object", msg, arg);
    }
}

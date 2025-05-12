/*
 * random_int.cpp
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

#include "lua_check.hpp"
#include "lua_exec.hpp"
#include "my_exceptions.hpp"
#include "random_int.hpp"

#include "include_lua.hpp"

RandomInt::RandomInt(lua_State *lua)
{
    ASSERT(lua);
    if (LuaIsCallable(lua, -1)) {
        function.reset(lua); // pops lua stack
        value = 0;
    } else if (lua_isnumber(lua, -1)) {
        value = lua_tointeger(lua, -1);
        lua_pop(lua, 1);
    } else {
        throw LuaError("Value is not a random int");
    }
}

int RandomInt::get() const
{
    if (function.hasValue()) {
        lua_State *lua = function.getLuaState();
        function.push(lua);
        LuaExec(lua, 0, 1);
        const int result = lua_tointeger(lua, -1);
        lua_pop(lua, 1);
        return result;
    } else {
        return value;
    }
}

/*
 * item_generator.cpp
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

#include "item_generator.hpp"
#include "lua_exec.hpp"
#include "lua_check.hpp"
#include "lua_userdata.hpp"

#include "lua.hpp"

ItemGenerator::ItemGenerator(lua_State *lua)
{
    if (!LuaIsCallable(lua, -1)) {
        luaL_error(lua, "Invalid item generator -- must be a function or callable object");
    }
    item_gen_func.reset(lua);
}

std::pair<ItemType *, int> ItemGenerator::get() const
{
    // Call the function -- pass no args, get upto 2 results.
    lua_State *lua = item_gen_func.getLuaState();
    ASSERT(lua);  // should have been set in ctor

    item_gen_func.push(lua);   // [... func]
    LuaExec(lua, 0, 2);    // [... r1 r2]

    ItemType * itype = ReadLuaPtr<ItemType>(lua, -2);
    int num = lua_tointeger(lua, -1);
    if (num < 1) num = 1;

    lua_pop(lua, 2);  // [...]

    return std::make_pair(itype, num);
}

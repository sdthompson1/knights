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

ItemGenerator::ItemGenerator(lua_State *lua_)
    : lua(lua_)
{
    LuaCheckCallable(lua, 1, "ItemGenerator");
    lua_pushvalue(lua, 1);
    lua_rawsetp(lua, LUA_REGISTRYINDEX, this);
}

ItemGenerator::~ItemGenerator()
{
    lua_pushnil(lua);
    lua_rawsetp(lua, LUA_REGISTRYINDEX, this);
}

std::pair<const ItemType *, int> ItemGenerator::get() const
{
    // Call the function -- pass no args, get upto 2 results.
    lua_rawgetp(lua, LUA_REGISTRYINDEX, this);
    LuaExec(lua, 0, 2);

    const ItemType * itype = ReadLuaPtr<const ItemType>(lua, -2);

    int num = lua_tointeger(lua, -1);
    if (num < 1) num = 1;

    return std::make_pair(itype, num);
}

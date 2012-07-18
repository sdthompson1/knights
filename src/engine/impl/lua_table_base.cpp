/*
 * lua_table_base.hpp
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

#include "lua_table_base.hpp"

#include "lua.hpp"

LuaTableBase::LuaTableBase(lua_State *lua_, int idx)
    : lua(lua_)
{
    if (lua) {

        ////
        lua_len(lua, idx);
        const int test = lua_tointeger(lua, -1);
        lua_pop(lua, 1);
        bool test2 = lua_istable(lua, -1);
        ////

        lua_pushvalue(lua, idx);
        table_ref = luaL_ref(lua, LUA_REGISTRYINDEX);
    }
}

LuaTableBase::LuaTableBase(const LuaTableBase &other)
{
    if (other.lua) {
        lua = other.lua;
        lua_rawgeti(lua, LUA_REGISTRYINDEX, other.table_ref);
        table_ref = luaL_ref(lua, LUA_REGISTRYINDEX);
    } else {
        lua = 0;
        // may as well leave table_ref unset.
    }
}

LuaTableBase::~LuaTableBase()
{
    if (lua) {
        luaL_unref(lua, LUA_REGISTRYINDEX, table_ref); // doesn't raise errors
    }
}

void LuaTableBase::pushTable(lua_State *L) const
{
    if (lua) {
        // NOTE: use L, rather than stored 'lua', because we might be running on a 
        // different Lua thread.
        lua_rawgeti(L, LUA_REGISTRYINDEX, table_ref);
    } else {
        // It doesn't have a stored table; we just push an empty table in this case
        lua_newtable(L);
    }
}

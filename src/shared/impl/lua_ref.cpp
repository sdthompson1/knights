/*
 * lua_ref.cpp
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

#include "lua_ref.hpp"

#include "lua.hpp"

LuaRef::LuaRef()
{
    lua = 0;
    ref = LUA_NOREF;
}

LuaRef::LuaRef(lua_State *lua_)
{
    lua = lua_;
    ref = luaL_ref(lua, LUA_REGISTRYINDEX);
}

LuaRef::LuaRef(const LuaRef &other)
{
    lua = other.lua;
    if (lua) {
        lua_rawgeti(lua, LUA_REGISTRYINDEX, other.ref);  // push the object
        ref = luaL_ref(lua, LUA_REGISTRYINDEX);          // pop the object & create a separate ref to it.
    } else {
        ref = LUA_NOREF;
    }
}

LuaRef & LuaRef::operator=(const LuaRef &other)
{
    if (&other == this) return *this;

    // release current reference
    luaL_unref(lua, LUA_REGISTRYINDEX, ref);

    // add a new reference
    lua = other.lua;
    if (lua) {
        lua_rawgeti(lua, LUA_REGISTRYINDEX, other.ref);
        ref = luaL_ref(lua, LUA_REGISTRYINDEX);
    } else {
        ref = LUA_NOREF;
    }

    return *this;
}

LuaRef::~LuaRef()
{
    // luaL_unref is documented not to throw errors. Therefore, safe
    // to call in a dtor.
    if (lua) {
        luaL_unref(lua, LUA_REGISTRYINDEX, ref);
    }
}

void LuaRef::push(lua_State *L) const
{
    // Note: L is different to lua as we might want to push in a
    // different Lua thread to the original

    // Caller is responsible for passing a non-null lua state.
    // (However, the reference itself can have a null state; that is ok; Lua will just push nil.)
    ASSERT(L);  

    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
}

void LuaRef::reset(lua_State *L)
{
    if (lua) {
        luaL_unref(lua, LUA_REGISTRYINDEX, ref);
    }
    lua = L;
    if (lua) {
        ref = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        ref = LUA_NOREF;
    }
}

bool LuaRef::hasValue() const
{
    return lua != 0 && ref != LUA_REFNIL;
}

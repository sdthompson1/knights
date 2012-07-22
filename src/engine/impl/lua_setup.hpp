/*
 * lua_setup.hpp
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

#ifndef LUA_SETUP_HPP
#define LUA_SETUP_HPP

#include "item_type.hpp"    // for ItemSize enum
#include "lua_userdata.hpp"
#include "map_support.hpp"  // for MapDirection enum

#include "kconfig_fwd.hpp"

#include "lua.hpp"

class Action;
class KnightsConfigImpl;
class Tile;

// Sets up Lua functions for creating ItemTypes, etc, and adding them
// to the KnightsConfigImpl.
void AddLuaConfigFunctions(lua_State *, KnightsConfigImpl *);


// These functions are used by constructors (e.g. ItemType ctor) to
// get values from the Lua state.

// Read values from a lua table (at tbl_idx)
bool LuaGetBool(lua_State *lua, int tbl_idx, const char *key, bool dflt = false);
int LuaGetInt(lua_State *lua, int tbl_idx, const char *key, int dflt = 0);
float LuaGetFloat(lua_State *lua, int tbl_idx, const char *key, float dflt = 0.0f);
float LuaGetProbability(lua_State *lua, int tbl_idx, const char *key, float dflt = 0.0f);
std::string LuaGetString(lua_State *lua, int tbl_idx, const char *key, const char *dflt = "");
ItemSize LuaGetItemSize(lua_State *lua, int tbl_idx, const char *key, ItemSize dflt = IS_NOPICKUP);
MapDirection LuaGetMapDirection(lua_State *lua, int tbl_idx, const char *key, MapDirection dflt = D_NORTH);
const KConfig::RandomInt * LuaGetRandomInt(lua_State *lua, int tbl_idx, const char *key, KnightsConfigImpl *kc);

template<class T> T * LuaGetPtr(lua_State *lua, int tbl_idx, const char *key)
{
    lua_getfield(lua, tbl_idx, key);
    T * result = ReadLuaPtr<T>(lua, -1);
    lua_pop(lua, 1);
    return result;
}

template<class T> boost::shared_ptr<T> LuaGetSharedPtr(lua_State *lua, int tbl_idx, const char *key)
{
    lua_getfield(lua, tbl_idx, key);
    boost::shared_ptr<T> result = ReadLuaSharedPtr<T>(lua, -1);
    lua_pop(lua, 1);
    return result;
}

// nil is treated as an empty list
void LuaGetTileList(lua_State *lua, int tbl_idx, const char *key, std::vector<boost::shared_ptr<Tile> > &tiles);


// Get the KnightsConfigImpl* from the lua state.
// If not available, raises lua error "Cannot create new " + msg + " during the game".
KnightsConfigImpl * GetKC(lua_State *lua, const char *msg);

#endif

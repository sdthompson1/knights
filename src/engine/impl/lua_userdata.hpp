/*
 * lua_userdata.hpp
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

#ifndef LUA_USERDATA_HPP
#define LUA_USERDATA_HPP

#include "lua_traits.hpp"
#include "map_support.hpp"

#include "lua.hpp"

typedef unsigned short int LuaTag;

// Implementation functions
void NewLuaPtr_Impl(lua_State *lua, void *ptr, LuaTag tag);
void NewLuaSharedPtr_Impl(lua_State *lua, boost::shared_ptr<void> ptr, LuaTag tag);
void NewLuaWeakPtr_Impl(lua_State *lua, boost::weak_ptr<void> ptr, LuaTag tag);
void * ReadLuaPtr_Impl(lua_State *lua, int index, LuaTag expected_tag);
boost::shared_ptr<void> ReadLuaSharedPtr_Impl(lua_State *lua, int index, LuaTag expected_tag);
boost::weak_ptr<void> ReadLuaWeakPtr_Impl(lua_State *lua, int index, LuaTag expected_tag);

// Create a new Lua userdata object and push it onto the stack
template<class T> inline void NewLuaPtr(lua_State *lua, T *ptr) { NewLuaPtr_Impl(lua, ptr, LuaTraits<T>::tag); }
template<class T> inline void NewLuaSharedPtr(lua_State *lua, boost::shared_ptr<T> ptr) { NewLuaSharedPtr_Impl(lua, ptr, LuaTraits<T>::tag); }
template<class T> inline void NewLuaWeakPtr(lua_State *lua, boost::weak_ptr<T> ptr) { NewLuaWeakPtr_Impl(lua, ptr, LuaTraits<T>::tag); }

// Read a Lua userdata object from a given stack index
// Conversions:
// ReadLuaPtr can be used on a raw or shared ptr (but NOT a weak ptr).
// ReadLuaSharedPtr can be used on a shared or weak ptr.
// ReadLuaWeakPtr can only be used on a weak ptr.
template<class T> inline T * ReadLuaPtr(lua_State *lua, int index) {
    return static_cast<T*>(ReadLuaPtr_Impl(lua, index, LuaTraits<T>::tag));
}
template<class T> inline boost::shared_ptr<T> ReadLuaSharedPtr(lua_State *lua, int index) {
    return boost::static_pointer_cast<T>(ReadLuaSharedPtr_Impl(lua, index, LuaTraits<T>::tag));
}
template<class T> inline boost::weak_ptr<T> ReadLuaWeakPtr(lua_State *lua, int index) {
    return boost::static_pointer_cast<T>(ReadLuaWeakPtr_Impl(lua, index, LuaTraits<T>::tag));
}

// Function to push a MapCoord onto the stack (as a table with "x" and "y" values,
// or "nil" if the MapCoord is null.)
void PushMapCoord(lua_State *lua, const MapCoord &mc);

// Push MapDirection onto the stack, as string ("north", "east", "south" or "west")
void PushMapDirection(lua_State *lua, MapDirection dir);

#endif

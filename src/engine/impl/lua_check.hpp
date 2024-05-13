/*
 * lua_check.hpp
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

#ifndef LUA_CHECK_HPP
#define LUA_CHECK_HPP

struct lua_State;

// Various lua checking functions

// Determine if object at given index is callable
bool LuaIsCallable(lua_State *lua, int index);

// Raises lua error if arg #n is not callable
// The error message is of form "msg: Argument #n is not a function or callable object"
void LuaCheckCallable(lua_State *lua, int arg, const char *msg);

// Ditto but must be indexable (table, or __index metamethod)
void LuaCheckIndexable(lua_State *lua, int arg, const char *msg);

#endif

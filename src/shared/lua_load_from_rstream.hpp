/*
 * lua_load_from_rstream.hpp
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

#ifndef LUA_LOAD_FROM_RSTREAM_HPP
#define LUA_LOAD_FROM_RSTREAM_HPP

#include <string>

struct lua_State;

// Read a chunk from an Rstream and push it on to the lua stack
// (as a lua function).
// If there is an error, throws LuaError and leaves the stack unchanged.
void LuaLoadFromRStream(lua_State *lua, const std::string &filename);

// Ditto but reads from a string instead
void LuaLoadFromString(lua_State *lua, const char *str);

// Execute the lua function on top of the stack, passing no arguments
// and returning no results.
// Pops the function off of the stack.
// If there is an error, throws LuaError (but still pops the function
// off the stack).
void LuaExec(lua_State *lua);

#endif

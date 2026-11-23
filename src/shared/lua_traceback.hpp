/*
 * lua_traceback.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2026.
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

#ifndef LUA_TRACEBACK_HPP
#define LUA_TRACEBACK_HPP

#include <string>

struct lua_State;

// Appends a stack traceback of the given lua state to msg
// (Does not include C functions, and stops at the first C function after a Lua function.)
// If msg already contains a traceback (e.g. from a nested dofile), the new lines are
// added to the existing traceback rather than starting a new one.
void LuaAppendTraceback(lua_State *lua, std::string &msg);

#endif

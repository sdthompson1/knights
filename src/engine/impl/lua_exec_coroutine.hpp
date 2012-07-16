/*
 * lua_exec_coroutine.hpp
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

#ifndef LUA_EXEC_COROUTINE_HPP
#define LUA_EXEC_COROUTINE_HPP

struct lua_State;

// Pops a cxt table, a function and n arguments from the stack, and 
// launches the function as a coroutine.

// If the function yields a number, then it will delay that many ms
// and re-execute after that time. (A Task is added to take care of
// this.) The yield will not return any values back to Lua.

// If the function returns, then the Task terminates (return value(s)
// from the function are ignored).

// If the function yields anything else, or errors, then a message
// will be displayed to all players, and the task will be terminated.

// "cxt" will be saved and restored across yields.

void LuaExecCoroutine(lua_State *lua, int nargs);

#endif



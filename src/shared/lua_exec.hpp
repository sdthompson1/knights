/*
 * lua_exec.hpp
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

#ifndef LUA_EXEC_HPP
#define LUA_EXEC_HPP

struct lua_State;

// Execute the lua function (and args) on top of the stack. The stack on entry is
//
//   [<stuff> func arg1 ... argn]
//
// The stack on exit will be
//
//   [<stuff> result1 ... resultn]
//
// except if there was an error, in which case:
//
//  a) If an enclosing Lua function is currently executing, the error will be
//     reported as a normal Lua error message.
//
//  b) If this is a "top level" Lua call, then the stack will be reset to
//     [<stuff>] and a LuaError C++ exception will be thrown.
//
// (In either case the message will contain a Lua stack traceback.)
//
// Note 1: <stuff> denotes zero or more items, which are ignored by LuaExec.
// Note 2: nresults may be any non-negative integer, or LUA_MULTRET.

void LuaExec(lua_State *lua, int nargs, int nresults);

#endif

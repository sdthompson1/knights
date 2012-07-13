/*
 * lua_func_wrapper.hpp
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

#ifndef LUA_FUNC_WRAPPER_HPP
#define LUA_FUNC_WRAPPER_HPP

#include "lua.hpp"

// The following functions push a "wrapped" C function onto the Lua
// stack.
// 
// The wrapper catches any C++ exceptions and re-throws them as Lua
// errors.
// 
// IMPORTANT: The wrapper adds one extra upvalue, as upvalue 1.
// Therefore your code will need to refer to upvalues from index 2
// onwards, instead of 1, if it has been wrapped.


// Note: Explanation of how exception safety works between C++ and
// Lua.
//
// -- Generally I write my C++ code such that it is safe for Lua
//    exceptions to propagate through it:
//
//   -- I compile Lua as C++, so it uses exceptions rather than
//      longjmp to propagate Lua errors.
//    
//   -- I write my C++ code to be exception safe, so if a Lua
//      exception passes up through it, it does no harm (all dtors run
//      properly).
//    
//   -- I cannot catch Lua exceptions directly, but I generally use
//      lua_pcall rather than lua_call to execute Lua code (see
//      LuaExec), so can catch lua errors that way. (I think LuaExec
//      converts them back to C++ exceptions, in fact...)
//
// -- Throwing C++ exceptions through Lua itself is probably a bad
//    idea, because Lua was not written with exception safety in mind
//    (it catches its own exceptions, but not external ones).
//
//   -- Therefore, I use the PushCFunction wrapper, instead of
//      lua_pushcfunction, to ensure that C++ exceptions never
//      propagate into Lua (instead they are converted to calls to
//      lua_error, which Lua is able to process w/o problems.)
//   

void PushCClosure(lua_State *lua, lua_CFunction func, int nupvalues);

inline void PushCFunction(lua_State *lua, lua_CFunction func)
{
    PushCClosure(lua, func, 0);
}

#endif

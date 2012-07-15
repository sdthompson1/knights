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

// PushCClosure "wraps" a try/catch handler around a lua_CFunction.
// The handler catches any C++ exceptions and converts them into a lua error.
//
// The purpose of this is to prevent C++ exceptions from propagating up into the Lua 
// codebase (which was not written with C++ exceptions in mind).
// 
// IMPORTANT: The wrapper adds one extra upvalue, as upvalue 1.
// Therefore your code will need to refer to upvalues from index 2
// onwards, instead of 1, if it has been wrapped.


// NOTE: I wrote a long description of exception safety in Lua, but in the end it boils 
// down to three simple rules:
//
// * If calling the Lua API from a C++ destructor:
//  -- Make sure you leave the stack as you found it
//  -- NEVER raise a Lua error from inside a dtor (and remember that many Lua API calls can 
//      raise errors).
// 
// * When pushing lua_CFunctions, ALWAYS use PushCClosure to "wrap" the function in an
//    exception handler.
//
// * Ensure that there is ALWAYS a lua_pcall somewhere up the call chain -- NEVER allow
//    Lua errors to escape up to top level, because this would abort the program.


void PushCClosure(lua_State *lua, lua_CFunction func, int nupvalues);

inline void PushCFunction(lua_State *lua, lua_CFunction func)
{
    PushCClosure(lua, func, 0);
}

#endif

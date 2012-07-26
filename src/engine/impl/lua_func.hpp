/*
 * lua_func.hpp
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

#ifndef LUA_FUNC_HPP
#define LUA_FUNC_HPP

#include "lua_ref.hpp"

#include <string>

class ActionData;

struct lua_State;

//
// Class representing a reference to a lua function.
//
// Basically this is the same as LuaRef but it indicates the intent
// that the ref should contain a function. It also contains some handy
// member functions to call the stored function in various ways.
//

class LuaFunc {
public:
    LuaFunc() { }
    explicit LuaFunc(lua_State *lua);   // pops from lua stack
    LuaFunc(lua_State *lua, int idx, const char *field) { reset(lua, idx, field); }
    void reset(lua_State *lua, int idx, const char *field);

    // hasValue(): returns true if lua != 0 && stored value != nil
    bool hasValue() const { return function_ref.hasValue(); }

    // This runs the stored function, as a coroutine, with no
    // arguments, and with the global variable "cxt" set to the
    // contents of the ActionData. ("cxt" will be saved and restored
    // across the call.)
    //
    // If stored func is nil, execute() does nothing (and returns false).
    //
    // If stored func yields, execute() returns false, otherwise it 
    // returns the first lua return value from the function, converted
    // to bool.
    //
    bool execute(const ActionData &) const;

    // Run with int parameter and return string result. Stack is not changed.
    // (Caller MUST check that hasValue() is true before calling this)
    std::string runIntToString(int param) const;

    // Pop lua value from top of stack and return string result.
    // (Caller MUST check that hasValue() is true before calling this)
    std::string runOneArgToString() const;
    
    // Run with a simple LuaExec. Reads N args from top of stack, but
    // does not modify the stack. Returns nothing. (If the ref is none
    // or nil, this function does nothing.)
    void runNArgsNoPop(lua_State *lua, int n) const;
    
private:
    LuaRef function_ref;
};

#endif

/*
 * lua_func.cpp
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

#include "misc.hpp"

#include "action_data.hpp"
#include "lua_check.hpp"
#include "lua_exec.hpp"
#include "lua_exec_coroutine.hpp"
#include "lua_func.hpp"
#include "my_exceptions.hpp"

#include "lua.hpp"

LuaFunc::LuaFunc(lua_State *lua)
: function_ref(lua)
{ }

void LuaFunc::reset(lua_State *lua, int idx, const char *field)
{
    lua_getfield(lua, idx, field);  // pushes function to the stack
    if (!lua_isnil(lua, -1) && !LuaIsCallable(lua, -1)) {
        throw LuaError(std::string("\'") + field + "' is not a Lua function");
    }
    function_ref.reset(lua);  // Pops function from the stack.
}

bool LuaFunc::execute(const ActionData &ad) const
{
    lua_State *lua = function_ref.getLuaState();
    if (lua && function_ref.hasValue()) {
        
        // Set up "cxt" table
        ad.pushCxtTable(lua);  // [cxt]

        // Get the function from the registry
        function_ref.push(lua); // [cxt func]

        // Call it (with 0 arguments)
        return LuaExecCoroutine(lua, 0);  // []
    } else {
        return false;
    }
}

std::string LuaFunc::runIntToString(int param) const
{
    lua_State *lua = function_ref.getLuaState();
    ASSERT(lua);
    
    function_ref.push(lua);  // [func]
    lua_pushinteger(lua, param);  // [func param]
    LuaExec(lua, 1, 1);   // [result]

    const char * p = lua_tostring(lua, -1);
    std::string result;
    if (p) result = p;
    else result = "<Not a string>";

    lua_pop(lua, 1);  // []
    return result;
}

std::string LuaFunc::runOneArgToString() const
{
    // we could factor out some common code between this and
    // runIntToString, but doesn't seem worth it at the moment

    // [arg]
    
    lua_State *lua = function_ref.getLuaState();
    ASSERT(lua);
    
    function_ref.push(lua);  // [arg func]
    lua_insert(lua, -2);     // [func arg]
    LuaExec(lua, 1, 1);   // [result]

    const char * p = lua_tostring(lua, -1);
    std::string result;
    if (p) result = p;
    else result = "<Not a string>";

    lua_pop(lua, 1);  // []
    return result;
}    

void LuaFunc::runNArgsNoPop(lua_State *lua, int n) const
{
    // [a1 .. an]
    function_ref.push(lua);  // [a1 .. an f]
    if (!lua_isnil(lua, -1)) {
        for (int i = 0; i < n; ++i) lua_pushvalue(lua, -n-1);  // [a1 .. an f a1 .. an]
        LuaExec(lua, n, 0);         // [a1 .. an]
    } else {
        lua_pop(lua, 1);  // [a1 .. an]
    }
}

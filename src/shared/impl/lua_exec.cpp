/*
 * lua_exec.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2013.
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

#include "lua_exec.hpp"
#include "lua_func_wrapper.hpp"
#include "lua_traceback.hpp"
#include "my_exceptions.hpp"

#include "lua.hpp"

namespace {

    // The Lua error handler function (for pcall).
    // This is responsible for adding the stack traceback.

    int LuaExecErrFunc(lua_State *lua)
    {
        // Called with one error message: [msg]
        // Should return one error message.

        std::string result = lua_isstring(lua, 1) ? lua_tostring(lua, 1) : "<No err msg>";
        result += LuaTraceback(lua);
        lua_pop(lua, 1);
        lua_pushstring(lua, result.c_str());
        return 1;
    }
}

void LuaExec(lua_State *lua, int nargs, int nresults)
{
    // This function is like lua_pcall except that it throws LuaError on errors.
    // A stack traceback is added to the error message.
    
    // input stack: [<stuff> func arg1 ... argn]

    // output stack: [<stuff> result1 ... resultn]
    // or [] if exception thrown


    // Implementation:

    // We need to insert our error handling function so that the stack looks like this:
    // [<stuff> errfunc func arg1 ... argn]

    PushCFunction(lua, &LuaExecErrFunc);
    lua_insert(lua, -(2 + nargs));
    
    // now we can do the call

    const int old_top = lua_gettop(lua);
    const int result = lua_pcall(lua, nargs, nresults, -(2 + nargs));

    if (result != 0) {
        // stack is now: [<stuff> errfunc msg]
        const std::string err_msg = lua_isstring(lua, -1)
            ? lua_tostring(lua, -1) : "<No err msg>";
        lua_pop(lua, 2);
        throw LuaError(err_msg);
    }

    // stack is now: [<stuff> errfunc result1 .. resultn]
    // where n = nargs + 1 + new_top - old_top

    const int new_top = lua_gettop(lua);
    const int actual_nresults = nargs + 1 + new_top - old_top;

    // the following will always be true at this point:
    ASSERT(nresults == LUA_MULTRET || actual_nresults == nresults);    
    
    // We now need to remove that errfunc from the stack.
    lua_remove(lua, -(1 + actual_nresults));
    
    // Stack is now [<stuff> result1 ... resultn], as required.
}

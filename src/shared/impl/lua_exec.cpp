/*
 * lua_exec.cpp
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

#include "lua_exec.hpp"
#include "my_exceptions.hpp"

#include "lua.hpp"

namespace {
    int LuaExecErrFunc(lua_State *lua)
    {
        // Called with one error message: [msg]
        // Should return one error message.

        int ct = 1;
        
        lua_pushstring(lua, "\nTraceback:"); ++ct;

        // note: level 0 is this error handler function, so start from level 1.
        const int MAXLEVEL = 15;
        for (int level = 1; level <= MAXLEVEL; ++level) {
            lua_Debug ar;
            const int result = lua_getstack(lua, level, &ar);
            if (result == 0) {
                // Have reached top of stack
                break;
            }

            lua_getinfo(lua, "Sl", &ar);
            const char * fmtstr = ",   %s:%d";
            if (level == 1) ++fmtstr;  // don't show the comma for first entry
            lua_pushfstring(lua, fmtstr, ar.short_src, ar.currentline); ++ct;
        }
        
        lua_concat(lua, ct);
        return 1;
    }
}

void LuaExec(lua_State *lua, int nargs, int nresults)
{
    // This function is like lua_pcall except that it throws LuaError on errors.
    // A stack traceback is added to the error message.
    
    // input stack: [func arg1 ... argn]

    // output stack: [result1 ... resultn]
    // or [] if exception thrown


    // Implementation:
    
    // We need to insert our error handling function so that the stack looks like this:
    // [errfunc func arg1 ... argn]

    lua_pushcfunction(lua, &LuaExecErrFunc);
    lua_insert(lua, -(2 + nargs));
    
    // now we can do the call
    
    const int result = lua_pcall(lua, nargs, nresults, -(2 + nargs));
    if (result != 0) {

        // stack is now: [errfunc msg]
        
        // get the error msg & throw exception
        const std::string err_msg = lua_tostring(lua, -1);
        lua_pop(lua, 2);
        throw LuaError(err_msg);

    }

    // stack is now: [errfunc result1 .. resultn]
    
    // We need to remove that errfunc from the stack.
    lua_remove(lua, -(1 + nresults));
    
    // stack is now [result1 ... resultn] as required.
}

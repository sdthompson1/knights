/*
 * lua_func_wrapper.cpp
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

#include "lua_func_wrapper.hpp"

#include "boost/thread/mutex.hpp"
#include "boost/thread/locks.hpp"

// definition copied from ldo.c
struct lua_longjmp {
  struct lua_longjmp *previous;
  int b;
  volatile int status;  /* error code */
};

namespace {

    boost::mutex g_wrapper_mutex;
    std::map<int, lua_CFunction> g_func_map;
    int g_next_index;
    
    int LuaWrapperFunc(lua_State *lua)
    {
        int i = lua_tointeger(lua, lua_upvalueindex(1));
        
        lua_CFunction func;
        {
            boost::unique_lock<boost::mutex> lock(g_wrapper_mutex);
            func = g_func_map[i];
        }

        try {
            return func(lua);

        } catch (lua_longjmp *) {
            // This is a Lua error. Lua knows how to deal with this so
            // we can re-throw it w/o problems.
            throw;  
            
        } catch (const std::exception &e) {
            // Convert this to a Lua error.
            return luaL_error(lua, e.what());

        } catch (...) {
            // Convert this to a Lua error.
            return luaL_error(lua, "Unknown C++ exception");
        }
    }
}

void PushCClosure(lua_State *lua, lua_CFunction func, int nupvalues)
{
    int i;  // initialized below
    {
        boost::unique_lock<boost::mutex> lock(g_wrapper_mutex);
        i = g_next_index++;
        g_func_map[i] = func;
    }
    
    lua_pushinteger(lua, i);
    lua_insert(lua, -nupvalues - 1);

    lua_pushcclosure(lua, &LuaWrapperFunc, nupvalues + 1);
}

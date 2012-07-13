/*
 * lua_func_wrapper.cpp
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

#include "lua_func_wrapper.hpp"

#include "boost/thread/mutex.hpp"
#include "boost/thread/locks.hpp"

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

        } catch (const std::exception &e) {
            return luaL_error(lua, e.what());

        } catch (...) {
            return luaL_error(lua, "Unknown C++ exception");
        }
    }
}

void PushCClosure(lua_State *lua, lua_CFunction func, int nupvalues)
{
    int i = g_next_index++;
    
    lua_pushinteger(lua, i);
    lua_insert(lua, -nupvalues - 1);

    {
        boost::unique_lock<boost::mutex> lock(g_wrapper_mutex);
        g_func_map[i] = func;
    }

    lua_pushcclosure(lua, &LuaWrapperFunc, nupvalues + 1);
}

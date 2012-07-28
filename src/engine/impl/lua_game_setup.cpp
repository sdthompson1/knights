/*
 * lua_game_setup.cpp
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

#include "lua.hpp"


namespace {
    //
    // Lua Functions.
    //

    int MyFunc(lua_State *lua)
    {
        TODO;
    }
}

// TODO: Func to get mediator instance?


// Setup function.
void AddLuaConfigFunctions(lua_State *lua)
{
    // all functions go in "kts" table.
    lua_rawgeti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);   // [env]
    luaL_getsubtable(lua, -1, "kts");                        // [env kts]


    // Dungeon generation

    PushCFunction(lua, &AddItem);
    lua_setfield(lua, -2, "AddItem");

    PushCFunction(lua, &AddMonsters);
    lua_setfield(lua, -2, "AddMonsters");
    
    PushCFunction(lua, &AddStuff);
    lua_setfield(lua, -2, "AddStuff");

    PushCFunction(lua, &ConnectivityCheck);
    lua_setfield(lua, -2, "ConnectivityCheck");
    
    PushCFunction(lua, &GenerateLocksAndTraps);
    lua_setfield(lua, -2, "GenerateLocksAndTraps");
    
    PushCFunction(lua, &LayoutDungeon);
    lua_setfield(lua, -2, "LayoutDungeon");
    
    PushCFunction(lua, &WipeDungeon);
    lua_setfield(lua, -2, "WipeDungeon");
}

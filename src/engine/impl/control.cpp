/*
 * control.cpp
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

#include "control.hpp"
#include "lua_setup.hpp"

#include "lua.hpp"

Control::Control(lua_State *lua, int idx)
    : UserControl(lua, idx)
{
    ASSERT(lua);

    execute = LuaFunc(lua, idx, "action");
    possible = LuaFunc(lua, idx, "possible");
    can_execute_while_moving = LuaGetBool(lua, idx, "can_do_while_moving", false);
    can_execute_while_stunned = LuaGetBool(lua, idx, "can_do_while_stunned", false);
    
    lua_pushvalue(lua, idx);   // [t]
    table_ref.reset(lua);      // []            
}

void Control::newIndex(lua_State *lua)
{
    // [ud k v]
    if (!lua_isstring(lua, 2)) return;
    const std::string k = lua_tostring(lua, 2);

    if (k == "action") {
        lua_pushvalue(lua, 3);
        execute = LuaFunc(lua);
    } else if (k == "possible") {
        lua_pushvalue(lua, 3);
        possible = LuaFunc(lua);
    } else if (k == "can_do_while_moving") {
        can_execute_while_moving = lua_toboolean(lua, 3) != 0;
    } else if (k == "can_do_while_stunned") {
        can_execute_while_stunned = lua_toboolean(lua, 3) != 0;
    } else {
        UserControl::newIndex(lua);
    }
}

bool Control::checkPossible(const ActionData &ad) const
{
    // NOTE: Don't really want to run 'possible' as a coroutine
    // (which is what execute() does), but this is more convenient, 
    // because we don't have a function to run it normally!
    return !possible.hasValue() || possible.execute(ad);
}

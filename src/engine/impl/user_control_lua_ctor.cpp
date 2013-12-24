/*
 * user_control_lua_ctor.cpp
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

#include "lua_setup.hpp"
#include "lua_userdata.hpp"
#include "map_support.hpp"
#include "user_control.hpp"

#include "lua.hpp"

UserControl::UserControl(lua_State *lua, int idx)
    : id(-1)
{
    action_bar_slot = LuaGetInt(lua, 1, "action_bar_slot", -1);
    tap_priority = LuaGetInt(lua, 1, "tap_priority", 0);
    action_bar_priority = LuaGetInt(lua, 1, "action_bar_priority", tap_priority);
    continuous = LuaGetBool(lua, 1, "continuous", false);
    menu_direction = LuaGetMapDirection(lua, 1, "menu_direction", D_NORTH);
    menu_graphic = LuaGetPtr<Graphic>(lua, 1, "menu_icon");
    menu_special = static_cast<unsigned int>(LuaGetInt(lua, 1, "menu_special", 0));
    name = LuaGetString(lua, 1, "name");
    suicide_key = LuaGetBool(lua, 1, "suicide_key", false);
}

void UserControl::newIndex(lua_State *lua)
{
    // [ud key val]
    if (!lua_isstring(lua, 2)) return;
    const std::string k = lua_tostring(lua, 2);

    if (k == "action_bar_slot") {
        action_bar_slot = lua_tointeger(lua, 3);
    } else if (k == "tap_priority") {
        tap_priority = lua_tointeger(lua, 3);
    } else if (k == "action_bar_priority") {
        action_bar_priority = lua_tointeger(lua, 3);
    } else if (k == "continuous") {
        continuous = lua_toboolean(lua, 3) != 0;
    } else if (k == "menu_direction") {
        menu_direction = GetMapDirection(lua, 3);
    } else if (k == "menu_icon") {
        menu_graphic = ReadLuaPtr<Graphic>(lua, 3);
    } else if (k == "menu_special") {
        menu_special = lua_tointeger(lua, 3);
    } else if (k == "name") {
        name = lua_tostring(lua, 3);
    } else if (k == "suicide_key") {
        suicide_key = lua_toboolean(lua, 3) != 0;
    }
}

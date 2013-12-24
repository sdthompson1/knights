/*
 * overlay_lua_ctor.cpp
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

#include "lua_userdata.hpp"
#include "overlay.hpp"

#include "lua.hpp"

Overlay::Overlay(lua_State *lua, int idx)
    : id(-1)
{
    ASSERT(lua);
    
    lua_pushvalue(lua, idx); // push
    table_ref.reset(lua);    // pop

    for (int i = 0; i < N_OVERLAY_FRAME*4; ++i) {
        offset_data[i].ofsx = offset_data[i].ofsy = 0;
        offset_data[i].dir = D_NORTH;
    }

    for (int i = 0; i < 4; ++i) {
        lua_pushinteger(lua, i+1);  // [i+1]
        lua_gettable(lua, idx);     // [gfx]
        raw_graphic[i] = ReadLuaPtr<Graphic>(lua, -1);
        lua_pop(lua, 1);            // []
    }
}
    
void Overlay::newIndex(lua_State *lua)
{
    // [ud k v]
    const int k = lua_tointeger(lua, 2);
    if (k >= 1 && k <= 4) {
        raw_graphic[k-1] = ReadLuaPtr<Graphic>(lua, -1);
    }
}

    

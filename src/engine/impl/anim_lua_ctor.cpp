/*
 * anim_lua_ctor.cpp
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

#include "anim.hpp"
#include "lua_setup.hpp"
#include "lua_userdata.hpp"

#include "lua.hpp"

Anim::Anim(int id_, lua_State *lua, int idx)
    : LuaTableBase(lua, idx), id(id_)
{
    const int idxm1 = idx < 0 ? idx-1 : idx;

    lua_pushstring(lua, "bat");   // [... "bat"]
    lua_gettable(lua, idxm1);      // [... bat]
    vbat_mode = lua_toboolean(lua, -1) != 0;
    lua_pop(lua, 1);   // [...]
    
    lua_len(lua, idx);   // [... len]
    const int sz = lua_tointeger(lua, -1);
    lua_pop(lua, 1);     // [...]

    if (sz != 4 && sz != 12 && sz != 16 && sz != 32) {
        luaL_error(lua, "Anim table must have 4, 12, 16 or 32 elements");
    }

    for (int f = 0; f < 8; ++f) {
        if (sz >= (f+1)*4) {
            for (int d = 0; d < 4; ++d) {
                lua_pushinteger(lua, f*4 + d + 1);   // [... idx]
                lua_gettable(lua, idxm1);            // [... graphic]
                g[d][f] = ReadLuaPtr<Graphic>(lua, -1);
                lua_pop(lua, 1);                     // [...]
            }
        } else {
            for (int d = 0; d < 4; ++d) {
                g[d][f] = 0;
            }
        }
    }
}
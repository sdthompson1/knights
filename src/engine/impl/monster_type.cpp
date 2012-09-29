/*
 * monster_type.cpp
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

#include "lua_setup.hpp"
#include "monster_type.hpp"

#include "lua.hpp"

MonsterType::MonsterType(lua_State *lua)
{
    lua_pushvalue(lua, -1);
    table_ref.reset(lua);

    sound_action.reset(lua, -1, "sound");
}

void MonsterType::newIndex(lua_State *lua)
{
    // [ud key val]
    if (!lua_isstring(lua, 2)) return;

    const std::string k = lua_tostring(lua, 2);

    if (k == "sound") {
        lua_pushvalue(lua, 3);
        sound_action = LuaFunc(lua); // pops
    }
}

bool MonsterType::okToCreateAt(const DungeonMap &dmap, const MapCoord &pos) const
{
    return true;
}

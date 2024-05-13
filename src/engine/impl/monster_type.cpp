/*
 * monster_type.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2024.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

    on_attack.reset(lua, -1, "on_attack");
    on_damage.reset(lua, -1, "on_damage");
    on_death.reset(lua, -1, "on_death");
    on_move.reset(lua, -1, "on_move");
}

void MonsterType::newIndex(lua_State *lua)
{
    // [ud key val]
    if (!lua_isstring(lua, 2)) return;

    const std::string k = lua_tostring(lua, 2);

    if (k == "on_attack") {
        lua_pushvalue(lua, 3);
        on_attack = LuaFunc(lua);  // pops

    } else if (k == "on_damage") {
        lua_pushvalue(lua, 3);
        on_damage = LuaFunc(lua);

    } else if (k == "on_death") {
        lua_pushvalue(lua, 3);
        on_death = LuaFunc(lua);

    } else if (k == "on_move") {
        lua_pushvalue(lua, 3);
        on_move = LuaFunc(lua);
    }
}

bool MonsterType::okToCreateAt(const DungeonMap &dmap, const MapCoord &pos) const
{
    return true;
}

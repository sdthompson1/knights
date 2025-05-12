/*
 * create_monster_type.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2025.
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

#include "misc.hpp"

#include "create_monster_type.hpp"
#include "knights_config_impl.hpp"
#include "lua_setup.hpp"
#include "monster_definitions.hpp"
#include "my_ctype.hpp"

#include "include_lua.hpp"

#include <string>

using std::unique_ptr;

MonsterType * CreateMonsterType(lua_State *lua, KnightsConfigImpl *kc)
{
    // [... t]
    lua_getfield(lua, -1, "type");  // [... t type]
    std::string s;
    const char *p = lua_tostring(lua, -1);
    if (p) s = p;

    lua_pop(lua, 1);  // [... t]

    for (std::string::iterator it = s.begin(); it != s.end(); ++it) {
        *it = ToLower(*it);
    }

    unique_ptr<MonsterType> result;

    if (s == "walking") {
        result.reset(new WalkingMonsterType(lua));
    } else if (s == "flying") {
        result.reset(new FlyingMonsterType(lua));
    } else {
        luaL_error(lua, "Unknown monster type '%s'", s.c_str());
    }

    std::vector<boost::shared_ptr<Tile> > corpse_tiles;
    LuaGetTileList(lua, -1, "corpse_tiles", corpse_tiles);
        
    return kc->addLuaMonsterType(std::move(result), corpse_tiles);
}

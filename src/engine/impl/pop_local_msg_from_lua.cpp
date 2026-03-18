/*
 * pop_local_msg_from_lua.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2026.
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

#include "creature.hpp"
#include "lua_userdata.hpp"
#include "player.hpp"
#include "pop_local_msg_from_lua.hpp"

#include "include_lua.hpp"

LocalMsg PopLocalMsgFromLua(lua_State *lua)
{
    LocalMsg msg;

    if (lua_isstring(lua, -1)) {
        // Found a plain string key
        msg.key = LocalKey(lua_tostring(lua, -1));
        lua_pop(lua, 1);
        return msg;
    }

    if (!lua_istable(lua, -1)) {
        lua_pushstring(lua, "incorrect message format: should be string or table");
        lua_error(lua);
    }

    // Read the key
    lua_getfield(lua, -1, "key");
    if (!lua_isstring(lua, -1)) {
        lua_pushstring(lua, "message table must have a string 'key' field");
        lua_error(lua);
    }
    msg.key = LocalKey(lua_tostring(lua, -1));
    lua_pop(lua, 1);

    // Read the params
    lua_getfield(lua, -1, "params");
    if (!lua_isnil(lua, -1)) {
        if (!lua_istable(lua, -1)) {
            lua_pushstring(lua, "message 'params' field must be a table");
            lua_error(lua);
        }

        lua_len(lua, -1);
        size_t len = lua_tointeger(lua, -1);
        lua_pop(lua, 1);

        for (size_t i = 1; i <= len; ++i) {
            lua_pushinteger(lua, i);
            lua_gettable(lua, -2);

            if (lua_isnumber(lua, -1)) {
                msg.params.push_back(LocalParam(static_cast<int>(lua_tointeger(lua, -1))));
            } else if (lua_isstring(lua, -1)) {
                msg.params.push_back(LocalParam(LocalKey(lua_tostring(lua, -1))));
            } else {
                Player *player = nullptr;
                if (IsLuaPtr<Player>(lua, -1)) {
                    player = ReadLuaPtr<Player>(lua, -1);
                } else if (IsLuaPtr<Creature>(lua, -1)) {
                    shared_ptr<Creature> c = ReadLuaSharedPtr<Creature>(lua, -1);
                    if (c) player = c->getPlayer();
                }
                if (player) {
                    msg.params.push_back(LocalParam(player->getPlayerID()));
                } else {
                    lua_pushstring(lua, "message params must be strings, integers, or players");
                    lua_error(lua);
                }
            }

            lua_pop(lua, 1);
        }
    }
    lua_pop(lua, 1);

    // Read the plural value, if present
    lua_getfield(lua, -1, "plural");
    if (!lua_isnil(lua, -1)) {
        if (!lua_isnumber(lua, -1)) {
            lua_pushstring(lua, "message 'plural' field must be a number");
            lua_error(lua);
        }
        msg.count = static_cast<int>(lua_tointeger(lua, -1));
    }
    lua_pop(lua, 1);

    // Pop the table itself
    lua_pop(lua, 1);

    return msg;
}

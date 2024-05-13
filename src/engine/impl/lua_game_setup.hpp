/*
 * lua_game_setup.hpp
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

#ifndef LUA_GAME_SETUP_HPP
#define LUA_GAME_SETUP_HPP

class KnightsEngine;

struct lua_State;

#include <string>


// Adds Lua functions for game setup (dungeon generation, monster
// activity setup, stuff like that). Functions are added to the "kts"
// table.
void AddLuaGameSetupFunctions(lua_State *);


// this (temporarily) stores a ptr to the KnightsEngine into the lua
// registry, for use by the game startup functions
class LuaStartupSentinel {
public:
    explicit LuaStartupSentinel(lua_State *lua_, KnightsEngine &ke);
    ~LuaStartupSentinel();
private:
    lua_State *lua;
};


// this is a workaround to allow msgs to be printed during game startup
void GameStartupMsg(lua_State *lua, const std::string &msg);

#endif

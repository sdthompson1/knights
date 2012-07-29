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

#include "dungeon_generation_failed.hpp"
#include "dungeon_generator.hpp"
#include "dungeon_layout.hpp"
#include "knights_engine.hpp"
#include "lua_func_wrapper.hpp"
#include "lua_game_setup.hpp"
#include "lua_userdata.hpp"
#include "mediator.hpp"
#include "my_exceptions.hpp"

#include "lua.hpp"

#include <vector>

namespace {

    //
    // utility functions.
    //

    int g_dummy_reg_key;
    
    void SetKnightsEngine(lua_State *lua, KnightsEngine *ke)
    {
        lua_pushlightuserdata(lua, ke);
        lua_rawsetp(lua, LUA_REGISTRYINDEX, &g_dummy_reg_key);
    }
    
    KnightsEngine & GetKnightsEngine(lua_State *lua)
    {
        lua_rawgetp(lua, LUA_REGISTRYINDEX, &g_dummy_reg_key);
        void * ud = lua_touserdata(lua, -1);
        lua_pop(lua, 1);
        if (ud) {
            return *static_cast<KnightsEngine*>(ud);
        } else {
            throw LuaError("This function can only be called during game startup");
        }
    }        
    
    void PopTileList(lua_State *lua, std::vector<boost::shared_ptr<Tile> > &tiles)
    {
        // [t]
        if (IsLuaPtr<Tile>(lua, -1)) {
            tiles.push_back(ReadLuaSharedPtr<Tile>(lua, -1));
        } else {
            lua_len(lua, -1);  // [t l]
            int sz = lua_tointeger(lua, -1);
            lua_pop(lua, 1);   // [t]
            for (int i = 1; i <= sz; ++i) {
                lua_pushinteger(lua, i);  // [t i]
                lua_gettable(lua, -2);    // [t tile]
                tiles.push_back(ReadLuaSharedPtr<Tile>(lua, -1));
                lua_pop(lua, 1);  // [t]
            }
        }
        lua_pop(lua, 1); // []
    }

    void PopSegments(lua_State *lua, std::vector<const Segment *> &segs)
    {
        // [s]
        lua_len(lua, -1); // [s l]
        int sz = lua_tointeger(lua, -1);
        lua_pop(lua, 1); // [s]
        for (int i = 1; i <= sz; ++i) {
            lua_pushinteger(lua, i);  // [s i]
            lua_gettable(lua, -2);    // [s s]
            segs.push_back(ReadLuaPtr<Segment>(lua, -1));
            lua_pop(lua, 1); // [s]
        }
        lua_pop(lua, 1); // []
    }
    
    
    //
    // Lua Functions: Dungeon Generation
    //

    int AddItem(lua_State *lua)
    {
        return 0;  // TODO
    }

    int AddMonsters(lua_State *lua)
    {
        return 0;  // TODO
    }

    int AddStuff(lua_State *lua)
    {
        return 0;  // TODO
    }

    int ConnectivityCheck(lua_State *lua)
    {
        return 0;  // TODO
    }

    int GenerateLocksAndTraps_Lua(lua_State *lua)
    {
        const int nkeys = luaL_checkinteger(lua, 1);
        const bool pretrapped = lua_toboolean(lua, 2) != 0;

        GenerateLocksAndTraps(*Mediator::instance().getMap(), nkeys, pretrapped);
        
        return 0;
    }

    int LayoutDungeon(lua_State *lua)
    {
        // Input argument: a table containing:

        // * layout               -- a DungeonLayout, as a table.
        // * wall, horiz_door, vert_door  -- either tiletype, or list of tiletype.
        // * segments             -- list of segments, a random subset will be included
        // * special_segments     -- list of segments, all must be included
        // * entry_type           -- one of "none", "close", "away", "random" (corresponding to HomeType enum)

        DungeonSettings settings;
        
        lua_getfield(lua, 1, "layout");   // [layout]
        settings.layout.reset(new DungeonLayout(lua));   // []

        lua_getfield(lua, 1, "wall");
        PopTileList(lua, settings.wall_tiles);
        lua_getfield(lua, 1, "horiz_door");
        PopTileList(lua, settings.hdoor_tiles);
        lua_getfield(lua, 1, "vert_door");
        PopTileList(lua, settings.vdoor_tiles);

        lua_getfield(lua, 1, "segments");
        PopSegments(lua, settings.normal_segments);
        lua_getfield(lua, 1, "special_segments");
        PopSegments(lua, settings.required_segments);

        lua_getfield(lua, 1, "entry_type");   // [entry]
        const char *p = lua_tostring(lua, -1);
        bool home_set = false;
        if (p) {
            home_set = true;
            if (std::strcmp(p, "none") == 0) {
                settings.home_type = H_NONE;
            } else if (std::strcmp(p, "close") == 0) {
                settings.home_type = H_CLOSE;
            } else if (std::strcmp(p, "away") == 0) {
                settings.home_type = H_AWAY;
            } else if (std::strcmp(p, "random") == 0) {
                settings.home_type = H_RANDOM;
            } else {
                home_set = false;
            }
        }
        if (!home_set) {
            luaL_error(lua, "'home_type' is invalid, must be 'none', 'close', 'away' or 'random'");
        }

        Mediator &m = Mediator::instance();

        try {
            DungeonGenerator(*m.getMap(),
                             *m.getCoordTransform(),
                             m.getHomeManager(),
                             m.getMonsterManager(),
                             m.getPlayers(),
                             settings);
            
        } catch (DungeonGenerationFailed &f) {
            lua_getglobal(lua, "kts");  // [kts]
            lua_pushstring(lua, f.what());   // [kts msg]
            lua_setfield(lua, -2, "DUNGEON_ERROR"); // [kts]
            lua_pop(lua, 1);
        }

        return 0;
    }
    
    int WipeDungeon(lua_State *lua)
    {
        // clear the dungeon map
        KnightsEngine &ke = GetKnightsEngine(lua);
        ke.resetMap();

        // clear DUNGEON_ERROR
        lua_getglobal(lua, "kts");
        lua_pushnil(lua);
        lua_setfield(lua, -2, "DUNGEON_ERROR");
        lua_pop(lua, 1);
        
        return 0;
    }


    //
    // Quests
    //

    int AddQuest(lua_State *lua)
    {
        return 0;
    }

    int QuestFail(lua_State *lua)
    {
        return 0;
    }

    int SetDeathmatchMode(lua_State *lua)
    {
        return 0;
    }

    int AddHint(lua_State *lua)
    {
        return 0;
    }


    //
    // General Settings
    //
    
    int AddStartingGear(lua_State *lua)
    {
        const ItemType *itype = ReadLuaPtr<const ItemType>(lua, 1);

        std::vector<int> num;
        for (int i = 2; i <= lua_gettop(lua); ++i) {
            num.push_back(lua_tointeger(lua, i));
        }

        GetKnightsEngine(lua).addStartingGear(itype, num);
        
        return 0;
    }

    int SetStuffRespawning(lua_State *lua)
    {
        return 0;
    }

    int SetLockpickSpawn(lua_State *lua)
    {
        return 0;
    }

    int AddMonsterGenerator(lua_State *lua)
    {
        return 0;
    }

    int LimitMonster(lua_State *lua)
    {
        return 0;
    }

    int LimitTotalMonsters(lua_State *lua)
    {
        return 0;
    }

    int SetZombieActivity(lua_State *lua)
    {
        return 0;
    }

    int SetPremapped(lua_State *lua)
    {
        KnightsEngine &ke = GetKnightsEngine(lua);
        ke.setPremapped(true);
        return 0;
    }
}

// Setup function.
void AddLuaGameSetupFunctions(lua_State *lua)
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
    
    PushCFunction(lua, &GenerateLocksAndTraps_Lua);
    lua_setfield(lua, -2, "GenerateLocksAndTraps");
    
    PushCFunction(lua, &LayoutDungeon);
    lua_setfield(lua, -2, "LayoutDungeon");
    
    PushCFunction(lua, &WipeDungeon);
    lua_setfield(lua, -2, "WipeDungeon");


    // Quests

    PushCFunction(lua, &AddQuest);
    lua_setfield(lua, -2, "AddQuest");

    PushCFunction(lua, &QuestFail);
    lua_setfield(lua, -2, "QuestFail");

    PushCFunction(lua, &SetDeathmatchMode);
    lua_setfield(lua, -2, "SetDeathmatchMode");

    PushCFunction(lua, &AddHint);
    lua_setfield(lua, -2, "AddHint");


    // General Settings

    PushCFunction(lua, &AddStartingGear);
    lua_setfield(lua, -2, "AddStartingGear");

    PushCFunction(lua, &SetStuffRespawning);
    lua_setfield(lua, -2, "SetStuffRespawning");

    PushCFunction(lua, &SetLockpickSpawn);
    lua_setfield(lua, -2, "SetLockpickSpawn");

    PushCFunction(lua, &AddMonsterGenerator);
    lua_setfield(lua, -2, "AddMonsterGenerator");

    PushCFunction(lua, &LimitMonster);
    lua_setfield(lua, -2, "LimitMonster");

    PushCFunction(lua, &LimitTotalMonsters);
    lua_setfield(lua, -2, "LimitTotalMonsters");

    PushCFunction(lua, &SetZombieActivity);
    lua_setfield(lua, -2, "SetZombieActivity");

    PushCFunction(lua, &SetPremapped);
    lua_setfield(lua, -2, "SetPremapped");
}


LuaStartupSentinel::LuaStartupSentinel(lua_State *lua_, KnightsEngine &ke)
    : lua(lua_)
{
    SetKnightsEngine(lua, &ke);
}

LuaStartupSentinel::~LuaStartupSentinel()
{
    SetKnightsEngine(lua, 0);
}


void GameStartupMsg(lua_State *lua, const std::string &msg)
{
    KnightsEngine &ke = GetKnightsEngine(lua);
    ke.gameStartupMsg(msg);
}

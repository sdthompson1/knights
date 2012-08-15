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
#include "monster_manager.hpp"
#include "my_exceptions.hpp"
#include "player.hpp"

#include "lua.hpp"

#include <cstring>
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
        ItemType *itype = ReadLuaPtr<ItemType>(lua, 1);
        if (!itype) luaL_error(lua, "AddItem: no item specified");

        const int qty = luaL_checkinteger(lua, 2);

        std::vector<std::pair<int,int> > weights;
        lua_len(lua, 3);
        int sz = lua_tointeger(lua, -1);
        lua_pop(lua, 1);

        if ((sz & 1) != 0) luaL_error(lua, "AddItem: weights table must have even number of entries");

        int total_weight = 0;
        
        for (int i = 1; i < sz; i += 2) {
            lua_pushinteger(lua, i);
            lua_gettable(lua, 3);
            const int cat = GetTileCategory(lua, -1);
            lua_pop(lua, 1);

            lua_pushinteger(lua, i+1);
            lua_gettable(lua, 3);
            const int weight = lua_tointeger(lua, -1);
            if (weight < 0) luaL_error(lua, "Item weight must be non-negative");
            lua_pop(lua, 1);

            total_weight += weight;
            weights.push_back(std::make_pair(cat, weight));
        }

        if (total_weight == 0) luaL_error(lua, "Must have at least one non-zero weight");

        for (int i = 0; i < qty; ++i) {
            GenerateItem(*Mediator::instance().getMap(), *itype, weights, total_weight);
        }
        
        return 0;
    }

    int AddMonsters(lua_State *lua)
    {
        // arg 1 = monstertype
        // arg 2 = number to place

        // See also "AddMonster" in lua_ingame.cpp

        const MonsterType *montype = ReadLuaPtr<MonsterType>(lua, 1);
        if (!montype) {
            luaL_error(lua, "Invalid monster type passed to AddMonsters");
        }

        const int num = luaL_checkinteger(lua, 2);

        Mediator &m = Mediator::instance();
        GenerateMonsters(*m.getMap(), m.getMonsterManager(), *montype, num);
        
        return 0;
    }

    int AddStuff(lua_State *lua)
    {
        // param 1 = list of triples: { tile_cat_string, probability, generator_func }

        std::map<int, StuffInfo> stuff;
        
        lua_len(lua, 1);
        const int sz = lua_tointeger(lua, -1);
        lua_pop(lua, 1);
        
        for (int i = 1; i <= sz; ++i) {
            lua_pushinteger(lua, i);
            lua_gettable(lua, 1);              // [triple]

            lua_pushinteger(lua, 1);
            lua_gettable(lua, -2);
            const int cat = GetTileCategory(lua, -1);
            lua_pop(lua, 1);

            lua_pushinteger(lua, 2);
            lua_gettable(lua, -2);
            StuffInfo s;
            s.chance = float(lua_tonumber(lua, -1));
            lua_pop(lua, 1);

            lua_pushinteger(lua, 3);
            lua_gettable(lua, -2);             // [triple func]
            s.gen = ItemGenerator(lua);        // [triple]
            lua_pop(lua, 1);                   // []

            stuff[cat] = s;
        }

        GenerateStuff(*Mediator::instance().getMap(), stuff);
        
        return 0;
    }

    int ConnectivityCheck_Lua(lua_State *lua)
    {
        const int num_keys = luaL_checkinteger(lua, 1);
        ItemType *lockpicks = ReadLuaPtr<ItemType>(lua, 2);
        if (!lockpicks) {
            luaL_error(lua, "Invalid Lockpicks itemtype passed to ConnectivityCheck");
        }
        ConnectivityCheck(Mediator::instance().getPlayers(), num_keys, *lockpicks);
        return 0;
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

    int SetDeathmatchMode(lua_State *lua)
    {
        const bool dm = lua_toboolean(lua, 1) != 0;
        Mediator::instance().setDeathmatchMode(dm);
        return 0;
    }


    //
    // General Settings
    //
    
    int AddStartingGear(lua_State *lua)
    {
        ItemType *itype = ReadLuaPtr<ItemType>(lua, 1);
        if (!itype) luaL_error(lua, "AddStartingGear: no item specified");

        std::vector<int> num;
        for (int i = 2; i <= lua_gettop(lua); ++i) {
            num.push_back(lua_tointeger(lua, i));
        }

        GetKnightsEngine(lua).addStartingGear(itype, num);
        
        return 0;
    }

    int SetStuffRespawning(lua_State *lua)
    {
        lua_len(lua, 1);
        const int sz = lua_tointeger(lua, -1);
        lua_pop(lua, 1);

        std::vector<ItemType *> itemtypes;
        
        for (int i = 1; i <= sz; ++i) {
            lua_pushinteger(lua, i);
            lua_gettable(lua, 1);
            ItemType *itype = ReadLuaPtr<ItemType>(lua, -1);
            if (!itype) {
                luaL_error(lua, "Table entry %d is not a valid itemtype", i);
            }
            lua_pop(lua, 1);

            itemtypes.push_back(itype);
        }

        const int respawn_delay_in_ms = luaL_checkinteger(lua, 2);

        GetKnightsEngine(lua).setItemRespawn(itemtypes, respawn_delay_in_ms);
        
        return 0;
    }

    int SetLockpickSpawn(lua_State *lua)
    {
        ItemType *itype = ReadLuaPtr<ItemType>(lua, 1);
        if (!itype) luaL_error(lua, "Argument #1 is not a valid itemtype");

        // both times in ms
        const int init_time = luaL_checkinteger(lua, 2);
        const int interval_time = luaL_checkinteger(lua, 3);

        GetKnightsEngine(lua).setLockpickSpawn(itype, init_time, interval_time);
        
        return 0;
    }

    int AddMonsterGenerator(lua_State *lua)
    {
        // arg 1 = montype
        // arg 2 = list-of-tiletypes
        // arg 3 = probability

        const MonsterType *montype = ReadLuaPtr<MonsterType>(lua, 1);
        if (!montype) luaL_error(lua, "Argument #1 is not a valid monster type");

        std::vector<boost::shared_ptr<Tile> > tiles;
        
        lua_len(lua, 2);
        const int sz = lua_tointeger(lua, -1);
        lua_pop(lua, 1);

        for (int i = 1; i <= sz; ++i) {
            lua_pushinteger(lua, i);
            lua_gettable(lua, 2);
            boost::shared_ptr<Tile> t = ReadLuaSharedPtr<Tile>(lua, -1);
            if (!t) luaL_error(lua, "Invalid tile type");
            tiles.push_back(t);
        }

        float prob = float(lua_tonumber(lua, 3));

        for (std::vector<boost::shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
            Mediator::instance().getMonsterManager().addMonsterGenerator(*it, montype, prob);
        }
        
        return 0;
    }

    int LimitMonster(lua_State *lua)
    {
        // arg 1 = mon type
        // arg 2 = number

        MonsterType *montype = ReadLuaPtr<MonsterType>(lua, 1);
        if (!montype) luaL_error(lua, "Argument #1 is not a valid monster type");

        Mediator::instance().getMonsterManager().limitMonster(montype, luaL_checkinteger(lua, 2));
        
        return 0;
    }

    int LimitTotalMonsters(lua_State *lua)
    {
        // arg 1 = total number

        Mediator::instance().getMonsterManager().limitTotalMonsters(luaL_checkinteger(lua, 1));
        
        return 0;
    }

    int SetZombieActivity(lua_State *lua)
    {
        // arg 1 = probability
        // arg 2 = zombie activity table

        MonsterManager &monster_manager = Mediator::instance().getMonsterManager();

        const float prob = float(luaL_checknumber(lua, 1));
        monster_manager.setZombieChance(prob);

        lua_pushvalue(lua, 2);  // [zomtable]
        lua_len(lua, -1);  // [zt len]
        const int sz = lua_tointeger(lua, -1);
        lua_pop(lua, 1);  // [zt]

        for (int i = 0; i < sz; ++i) {
            lua_pushinteger(lua, i+1);  // [zt i]
            lua_gettable(lua, -2);      // [zt entry]

            // entry is a table of two things: tilefrom, and tileto/monsterto.
            
            lua_pushinteger(lua, 1);  // [zt entry 1]
            lua_gettable(lua, -2);    // [zt entry from]
            boost::shared_ptr<Tile> from = ReadLuaSharedPtr<Tile>(lua, -1);
            lua_pop(lua, 1);          // [zt entry]
            
            lua_pushinteger(lua, 2);  // [zt entry 2]
            lua_gettable(lua, -2);    // [zt entry to]
            
            if (IsLuaPtr<MonsterType>(lua, -1)) {
                monster_manager.addZombieReanimate(from, ReadLuaPtr<MonsterType>(lua, -1));
            } else {
                monster_manager.addZombieDecay(from, ReadLuaSharedPtr<Tile>(lua, -1));
            }
        
            lua_pop(lua, 2);  // [zt]
        }

        lua_pop(lua, 1);  // []
        
        return 0;
    }

    int SetPremapped(lua_State *lua)
    {
        KnightsEngine &ke = GetKnightsEngine(lua);
        ke.setPremapped(true);
        return 0;
    }

    int SetRespawnType(lua_State *lua)
    {
        const char *p = luaL_checkstring(lua, 1);

        Player::RespawnType rt;
        
        if (std::strcmp(p, "normal") == 0) {
            rt = Player::R_NORMAL;
        } else if (std::strcmp(p, "anywhere") == 0) {
            rt = Player::R_RANDOM_SQUARE;
        } else if (std::strcmp(p, "different") == 0) {
            rt = Player::R_DIFFERENT_EVERY_TIME;
        } else {
            luaL_error(lua, "'%s' is not a valid respawn type, must be 'normal', 'different' or 'anywhere'", p);
        }

        const std::vector<Player*> & players = Mediator::instance().getPlayers();
        for (std::vector<Player*>::const_iterator it = players.begin(); it != players.end(); ++it) {
            (*it)->setRespawnType(rt);
        }

        return 0;
    }

    int SetRespawnFunction(lua_State *lua)
    {
        // [func]
        LuaFunc func;
        if (!lua_isnil(lua, 1)) {
            lua_pushvalue(lua, 1);
            func = LuaFunc(lua);  // pops
        }

        Mediator & m = Mediator::instance();
        for (std::vector<Player*>::const_iterator it = m.getPlayers().begin(); it != m.getPlayers().end(); ++it) {
            (*it)->setRespawnFunc(func);
        }

        return 0;
    }

    int SetTimeLimit(lua_State *lua)
    {
        int seconds = luaL_checkinteger(lua, 1);
        GetKnightsEngine(lua).setTimeLimit(seconds * 1000);
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

    PushCFunction(lua, &ConnectivityCheck_Lua);
    lua_setfield(lua, -2, "ConnectivityCheck");
    
    PushCFunction(lua, &GenerateLocksAndTraps_Lua);
    lua_setfield(lua, -2, "GenerateLocksAndTraps");
    
    PushCFunction(lua, &LayoutDungeon);
    lua_setfield(lua, -2, "LayoutDungeon");
    
    PushCFunction(lua, &WipeDungeon);
    lua_setfield(lua, -2, "WipeDungeon");


    // Quests

    PushCFunction(lua, &SetDeathmatchMode);
    lua_setfield(lua, -2, "SetDeathmatchMode");


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

    PushCFunction(lua, &SetRespawnType);
    lua_setfield(lua, -2, "SetRespawnType");

    PushCFunction(lua, &SetRespawnFunction);
    lua_setfield(lua, -2, "SetRespawnFunction");

    PushCFunction(lua, &SetTimeLimit);
    lua_setfield(lua, -2, "SetTimeLimit");

    lua_pop(lua, 2);
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

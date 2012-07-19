/*
 * lua_ingame.cpp
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

#include "action.hpp"
#include "coord_transform.hpp"
#include "creature.hpp"
#include "dungeon_map.hpp"
#include "knights_callbacks.hpp"
#include "lockable.hpp"
#include "lua_func_wrapper.hpp"
#include "lua_ingame.hpp"
#include "lua_userdata.hpp"
#include "map_support.hpp"
#include "mediator.hpp"
#include "missile.hpp"
#include "my_exceptions.hpp"
#include "rng.hpp"
#include "teleport.hpp"

#include "lua.hpp"

#include "boost/scoped_ptr.hpp"

#include <cstring>

class Player;

namespace {


    // This is an implementation of ActionPars that reads its parameters from the Lua stack.
    // NOTE: 'index' parameter is zero-based, so have to add one when passing to Lua API.
    class LuaActionPars : public ActionPars {
    public:
        explicit LuaActionPars(lua_State *lua_) : lua(lua_) { }

        void require(int n1, int n2 = -1);

        int getSize();

        int getInt(int index);
        const ItemType * getItemType(int index);
        MapDirection getMapDirection(int index);
        float getProbability(int index);
        const Sound * getSound(int index);
        std::string getString(int index);
        shared_ptr<Tile> getTile(int index);
        const MonsterType * getMonsterType(int index);

        void error();
        
    private:
        lua_State *lua;
    };

    void LuaActionPars::require(int n1, int n2)
    {
        const int sz = getSize();
        if (sz != n1 && (n2 < 0 || sz != n2)) {
            // push error message as a series of strings/numbers which we concatenate.
            // (beware, number of elements set in lua_concat call below.)
            lua_pushstring(lua, "Wrong number of arguments to action function: expected ");
            lua_pushinteger(lua, n1);
            if (n2 >= 0) {
                lua_pushstring(lua, " or ");
                lua_pushinteger(lua, n2);
            }
            lua_pushstring(lua, ", got ");
            lua_pushinteger(lua, sz);
            lua_concat(lua, n2 >= 0 ? 6 : 4);
            lua_error(lua);
        }
    }

    int LuaActionPars::getSize()
    {
        return lua_gettop(lua);
    }

    int LuaActionPars::getInt(int c_index)
    {
        return luaL_checkinteger(lua, c_index+1);
    }

    void ErrMsg(lua_State *lua, int lua_index, const char *expected)
    {
        luaL_error(lua, "bad type for argument #%d: expected %s", lua_index, expected);
    }
    
    template<class T> T* CheckLuaPtr(lua_State *lua, int lua_index, const char *expected)
    {
        T* result = ReadLuaPtr<T>(lua, lua_index);
        if (!result) {
            ErrMsg(lua, lua_index, expected);
        }
        return result;
    }

    template<class T> boost::shared_ptr<T> CheckLuaSharedPtr(lua_State *lua, int lua_index, const char *expected)
    {
        boost::shared_ptr<T> result = ReadLuaSharedPtr<T>(lua, lua_index);
        if (!result) {
            ErrMsg(lua, lua_index, expected);
        }
        return result;
    }
    
    const ItemType * LuaActionPars::getItemType(int c_index)
    {
        return CheckLuaPtr<const ItemType>(lua, c_index+1, "item type");
    }
    
    MapDirection LuaActionPars::getMapDirection(int c_index)
    {
        return GetMapDirection(lua, c_index+1);
    }
    
    float LuaActionPars::getProbability(int c_index)
    {
        return float(luaL_checknumber(lua, c_index+1));
    }

    const Sound * LuaActionPars::getSound(int c_index)
    {
        return CheckLuaPtr<Sound>(lua, c_index+1, "sound");
    }

    std::string LuaActionPars::getString(int c_index)
    {
        const char * s = luaL_checkstring(lua, c_index+1);
        return s;
    }

    boost::shared_ptr<Tile> LuaActionPars::getTile(int c_index)
    {
        return CheckLuaSharedPtr<Tile>(lua, c_index+1, "tile");
    }

    const MonsterType * LuaActionPars::getMonsterType(int c_index)
    {
        throw LuaError("LuaActionPars::getMonsterType: not implemented");
        //return CheckLuaPtr<MonsterType>(lua, c_index+1);
    }

    void LuaActionPars::error()
    {
        luaL_error(lua, "Failed to create LuaAction");
    }
    

    typedef void (Lockable::* LockableFnPtr)(DungeonMap &, const MapCoord &, const Originator &);

    // Reads a position from the top of the lua stack, and a Player from "cxt".
    // Does not modify the lua stack.
    void DoOpenOrClose(lua_State *lua, LockableFnPtr function_ptr)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return;

        MapCoord mc = GetMapCoord(lua, 1);
        
        std::vector<shared_ptr<Tile> > tiles;
        dmap->getTiles(mc, tiles);

        for (std::vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end(); ++it) {
            Tile *tile = it->get();
            Lockable *lockable = dynamic_cast<Lockable*>(tile);
            if (lockable) {
                (lockable->*function_ptr)(*dmap, mc, GetOriginatorFromCxt(lua));
            }
        }
    }

    // Input: position
    // Cxt: originator
    // Output: none
    int Open(lua_State *lua)
    {
        DoOpenOrClose(lua, &Lockable::open);
        return 0;
    }

    // Input: position
    // Cxt: originator
    // Output: none
    int Close(lua_State *lua)
    {
        DoOpenOrClose(lua, &Lockable::close);
        return 0;
    }

    // Input: position
    // Cxt: originator
    // Output: none
    int OpenOrClose(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;

        MapCoord mc = GetMapCoord(lua, 1);
        Originator orig = GetOriginatorFromCxt(lua);

        std::vector<shared_ptr<Tile> > tiles;
        dmap->getTiles(mc, tiles);

        for (std::vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end(); ++it) {
            Tile *tile = it->get();
            Lockable *lockable = dynamic_cast<Lockable*>(tile);
            if (lockable) {
                if (lockable->isClosed()) {
                    lockable->open(*dmap, mc, orig);
                } else {
                    lockable->close(*dmap, mc, orig);
                }
            }
        }

        return 0;
    }

    // Input: position
    // Cxt: none
    // Output: true, false or nil
    int IsOpen(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;

        MapCoord mc = GetMapCoord(lua, 1);

        std::vector<shared_ptr<Tile> > tiles;
        dmap->getTiles(mc, tiles);

        for (std::vector<shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
            Tile *tile = it->get();
            Lockable *lockable = dynamic_cast<Lockable*>(tile);
            if (lockable) {
                lua_pushboolean(lua, ! lockable->isClosed());
                return 1;
            }
        }

        return 0;
    }
    
    // Input: position, direction, item type, boolean (drop_after flag)
    // Cxt: originator
    // Output: none
    int AddMissile(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;

        const MapCoord mc = GetMapCoord(lua, 1);
        const MapDirection dir = GetMapDirection(lua, 2);
        const ItemType * itype = ReadLuaPtr<const ItemType>(lua, 3);
        const bool drop_after = lua_toboolean(lua, 4) != 0;

        if (itype) {
            CreateMissile(*dmap, mc, dir, *itype, drop_after, false, GetOriginatorFromCxt(lua), true);
        }

        return 0;
    }

    // Input: creature, new position.
    // Cxt: none
    // Output: none
    int Teleport(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;

        shared_ptr<Creature> cr = ReadLuaSharedPtr<Creature>(lua, 1);
        const MapCoord dest = GetMapCoord(lua, 2);

        TeleportToSquare(cr, *dmap, dest);
        return 0;
    }

    // Input: position
    // Cxt: none
    // Output: list of tiles
    int GetTiles(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;

        const MapCoord &mc = GetMapCoord(lua, 1);
        
        std::vector<shared_ptr<Tile> > tiles;
        dmap->getTiles(mc, tiles);
        const int num_tiles = int(tiles.size());
        
        lua_createtable(lua, num_tiles, 0);
        
        for (int idx = 1; idx <= num_tiles; ++idx) {
            lua_pushinteger(lua, idx);
            NewLuaSharedPtr(lua, tiles[idx-1]);
            lua_settable(lua, -3);
        }

        return 1; // return the table
    }

    // Input: position, tile
    // Cxt: originator
    // Output: none
    int RemoveTile(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;

        const MapCoord &mc = GetMapCoord(lua, 1);
        shared_ptr<Tile> tile = ReadLuaSharedPtr<Tile>(lua, 2);

        dmap->rmTile(mc, tile, GetOriginatorFromCxt(lua));
        return 0;
    }

    // Input: position, tile
    // Cxt: originator
    // Output: none
    int AddTile(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;

        const MapCoord mc = GetMapCoord(lua, 1);
        shared_ptr<Tile> tile = ReadLuaSharedPtr<Tile>(lua, 2);

        if (tile) dmap->addTile(mc, tile->clone(false), GetOriginatorFromCxt(lua));
        return 0;
    }

    // Input: position (can be null), sound, frequency
    // Cxt: none
    // Output: none
    int PlaySound(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;

        const bool all = lua_isnil(lua, 1);
        const MapCoord mc = all ? MapCoord() : GetMapCoord(lua, 1);

        const Sound *sound = ReadLuaPtr<Sound>(lua, 2);
        if (!sound) return 0;

        const int frequency = lua_tointeger(lua, 3);

        med.playSound(*dmap, mc, *sound, frequency, all);
        return 0;
    }

    // Input: creature
    // Cxt: none
    // Output: map coord, or nil if creature is not valid / has been killed.
    int GetPos(lua_State *lua)
    {
        shared_ptr<Creature> cr = ReadLuaSharedPtr<Creature>(lua, 1);
        if (cr) {
            PushMapCoord(lua, cr->getPos());
        } else {
            lua_pushnil(lua);
        }
        return 1;
    }

    // Input: (optional) player, followed by any number of args.
    // Cxt: none
    // Output: none
    int Print(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        KnightsCallbacks& cb = med.getCallbacks();

        // find out if there is a 'player' argument, if so, convert it to a player number
        int player_num = -1;
        int start = 1;
        if (!lua_isnil(lua, 1) && IsLuaPtr<Player>(lua, 1)) {
            const Player * player = ReadLuaPtr<Player>(lua, 1);
            ++start;
            if (player) {
                for (int i = 0; i < med.getPlayers().size(); ++i) {
                    if (med.getPlayers()[i] == player) {
                        player_num = i;
                        break;
                    }
                }
                if (player_num < 0) return 0;  // that player doesn't seem to exist (shouldn't happen)
            }
        }

        // build the message
        std::string msg;
        const int top = lua_gettop(lua);
        for (int i = start; i <= top; ++i) {
            const char *x = lua_tostring(lua, i);
            if (!x) return luaL_error(lua, "'tostring' must return a string to 'print'");
            if (i > start) msg += " ";
            msg += x;
        }

        // print the message
        cb.gameMsg(player_num, msg);
        return 0;
    }

    // Input: map position + two integers
    // Cxt: none
    // Output: map position
    int RotateAddPos(lua_State *lua)
    {
        // Get the CoordTransform
        Mediator &med = Mediator::instance();
        const CoordTransform * ct = med.getCoordTransform().get();
        if (!ct) return 0;

        // Read the inputs
        const MapCoord base = GetMapCoord(lua, 1);
        int x = lua_tointeger(lua, 2);
        int y = lua_tointeger(lua, 3);
        
        // Do the transform
        ct->transformOffset(base, x, y);

        // Return results
        PushMapCoord(lua, MapCoord(base.getX() + x, base.getY() + y));
        return 1;
    }

    // Input: map position + direction (string)
    // Cxt: none
    // Output: new direction
    int RotateDirection(lua_State *lua)
    {
        // Get the CoordTransform
        Mediator &med = Mediator::instance();
        const CoordTransform * ct = med.getCoordTransform().get();
        if (!ct) return 0;

        // Read the inputs
        const MapCoord base = GetMapCoord(lua, 1);
        MapDirection dir = GetMapDirection(lua, 2);

        // Do the transform
        ct->transformDirection(base, dir);

        // Return result
        PushMapDirection(lua, dir);
        return 1;
    }

    // Input: time in ms
    // Cxt: actor
    // Output: yields; does not return anything.
    int Delay(lua_State *lua)
    {
        const int time = luaL_checkinteger(lua, 1);

        if (time <= 0) return 0;   // Nothing to do
        
        // Read cxt table.
        lua_getglobal(lua, "cxt");  // [arg cxt]
        lua_pushstring(lua, "actor");  // [arg cxt "actor"]
        lua_gettable(lua, -2);         // [arg cxt cr]
        boost::shared_ptr<Creature> actor = ReadLuaSharedPtr<Creature>(lua, -1);

        if (actor) {
            const int gvt = Mediator::instance().getGVT();
            actor->stunUntil(gvt + time);
        }

        lua_pushinteger(lua, time);
        return lua_yield(lua, 1);
    }

    // Input: none
    // Cxt: none
    // Output: time in ms
    int GameTime(lua_State *lua)
    {
        lua_pushinteger(lua, Mediator::instance().getGVT());
        return 1;
    }
    
    // Input: various
    // Cxt: all ActionData fields
    // Output: none
    // Upvalue: ActionMaker pointer.
    int LuaActionFunc(lua_State *lua)
    {
        // Make a LuaActionPars
        LuaActionPars p(lua);

        // Get the upvalue which is the ActionMaker
        const ActionMaker *maker = static_cast<const ActionMaker *>(lua_touserdata(lua, lua_upvalueindex(2)));

        // Call the maker to produce an Action.
        // Store in a scoped_ptr
        boost::scoped_ptr<Action> action( maker->make(p) );

        // Create the ActionData from cxt
        ActionData ad( lua );

        // Call the action
        action->execute(ad);

        // Done.
        return 0;
    }

    // Input: two integers (low and high of range; inclusive)
    // Cxt: none
    // Output: one integer
    int RandomRange(lua_State *lua)
    {
        int low = luaL_checkint(lua, 1);
        int high = luaL_checkint(lua, 2);

        int result = g_rng.getInt(low, high + 1);  // g_rng uses exclusive upper bound; we use inclusive; so add one.

        lua_pushinteger(lua, result);
        return 1;
    }

    int RandomChance(lua_State *lua)
    {
        double chance = luaL_checknumber(lua, 1);
        if (chance < 0) chance = 0;
        if (chance > 1) chance = 1;

        lua_pushboolean(lua, g_rng.getBool(float(chance)) ? 1 : 0);
        return 1;
    }
}

void AddLuaIngameFunctions(lua_State *lua)
{
    // all functions go in "kts" table.
    lua_rawgeti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);   // [env]
    luaL_getsubtable(lua, -1, "kts");                         // [env kts]
    
    PushCFunction(lua, &Open);
    lua_setfield(lua, -2, "open_door");

    PushCFunction(lua, &Close);
    lua_setfield(lua, -2, "close_door");

    PushCFunction(lua, &OpenOrClose);
    lua_setfield(lua, -2, "open_or_close_door");

    PushCFunction(lua, &IsOpen);
    lua_setfield(lua, -2, "is_door_open");
    
    PushCFunction(lua, &AddMissile);
    lua_setfield(lua, -2, "add_missile");

    PushCFunction(lua, &Teleport);
    lua_setfield(lua, -2, "teleport");

    PushCFunction(lua, &GetTiles);
    lua_setfield(lua, -2, "get_tiles");

    PushCFunction(lua, &RemoveTile);
    lua_setfield(lua, -2, "remove_tile");

    PushCFunction(lua, &AddTile);
    lua_setfield(lua, -2, "add_tile");

    PushCFunction(lua, &PlaySound);
    lua_setfield(lua, -2, "play_sound");

    PushCFunction(lua, &GetPos);
    lua_setfield(lua, -2, "get_pos");

    // "print" is set both in kts, and globally.
    PushCFunction(lua, &Print);       // [env kts Print]
    lua_pushvalue(lua, -1);           // [env kts Print Print]
    lua_setfield(lua, -3, "print");   // [env kts Print]
    lua_setfield(lua, -3, "print");   // [env kts]

    PushCFunction(lua, &RotateAddPos);
    lua_setfield(lua, -2, "rotate_add_pos");

    PushCFunction(lua, &RotateDirection);
    lua_setfield(lua, -2, "rotate_direction");

    // Now we want to add all the in-game Actions as Lua functions.
    {
        boost::unique_lock<boost::mutex> lock(g_makers_mutex);
        const std::map<std::string, const ActionMaker *> & makers_map = MakersMap();
        for (std::map<std::string, const ActionMaker *>::const_iterator it = makers_map.begin();
        it != makers_map.end(); ++it) {
            // const_cast is ok: we promise to cast it back to const again when 
            // we get it back from Lua...
            lua_pushlightuserdata(lua, const_cast<ActionMaker*>(it->second));
            PushCClosure(lua, &LuaActionFunc, 1);
            lua_setfield(lua, -2, it->first.c_str());
        }
    }

    // Function to stun a knight and also yield for that length of time.
    PushCFunction(lua, &Delay);
    lua_setfield(lua, -2, "Delay");

    // Function to return total elapsed time of this game, in ms.
    PushCFunction(lua, &GameTime);
    lua_setfield(lua, -2, "GameTime");
    
    // Random number generation functions.
    PushCFunction(lua, &RandomRange);
    lua_setfield(lua, -2, "RandomRange");

    PushCFunction(lua, &RandomChance);
    lua_setfield(lua, -2, "RandomChance");
    
    // pop the "kts" and environment tables.
    lua_pop(lua, 2);
}

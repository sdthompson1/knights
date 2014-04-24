/*
 * lua_ingame.cpp
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

#include "action_data.hpp"
#include "colour_change.hpp"
#include "concrete_traps.hpp"
#include "coord_transform.hpp"
#include "creature.hpp"
#include "dungeon_map.hpp"
#include "home_manager.hpp"
#include "item.hpp"
#include "item_type.hpp"
#include "knight.hpp"
#include "knights_callbacks.hpp"
#include "legacy_action.hpp"
#include "lockable.hpp"
#include "lua_exec_coroutine.hpp"
#include "lua_func_wrapper.hpp"
#include "lua_ingame.hpp"
#include "lua_userdata.hpp"
#include "map_support.hpp"
#include "mediator.hpp"
#include "missile.hpp"
#include "monster.hpp"
#include "monster_manager.hpp"
#include "monster_type.hpp"
#include "my_exceptions.hpp"
#include "player.hpp"
#include "rng.hpp"
#include "teleport.hpp"
#include "tutorial_manager.hpp"
#include "tutorial_window.hpp"

#include "lua.hpp"

#include "boost/scoped_ptr.hpp"

#include <cstring>
#include <iostream>

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
        ItemType * getItemType(int index);
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
    
    template<class T> T& CheckLuaPtr(lua_State *lua, int lua_index, const char *expected)
    {
        T* result = ReadLuaPtr<T>(lua, lua_index);
        if (!result) {
            ErrMsg(lua, lua_index, expected);
        }
        return *result;
    }

    template<class T> shared_ptr<T> CheckLuaSharedPtr(lua_State *lua, int lua_index, const char *expected)
    {
        shared_ptr<T> result = ReadLuaSharedPtr<T>(lua, lua_index);
        if (!result) {
            ErrMsg(lua, lua_index, expected);
        }
        return result;
    }
    
    Player & GetPlayerOrKnight(lua_State *lua, int lua_index)
    {
        Player *result = 0;
        
        if (IsLuaPtr<Player>(lua, lua_index)) {
            result = ReadLuaPtr<Player>(lua, lua_index);
        } else if (IsLuaPtr<Creature>(lua, lua_index)) {
            shared_ptr<Creature> c = ReadLuaSharedPtr<Creature>(lua, lua_index);
            if (c) result = c->getPlayer();
        }

        if (!result) {
            ErrMsg(lua, lua_index, "player or knight");
        }

        return *result;
    }

    ItemType * LuaActionPars::getItemType(int c_index)
    {
        return &CheckLuaPtr<ItemType>(lua, c_index+1, "item type");
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
        return &CheckLuaPtr<Sound>(lua, c_index+1, "sound");
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
        return &CheckLuaPtr<MonsterType>(lua, c_index+1, "monster");
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

    // Input: position, lock type (number or string)
    // Cxt: none
    // Output: none
    int LockDoor(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;
        
        MapCoord mc = GetMapCoord(lua, 1);
        std::vector<shared_ptr<Tile> > tiles;
        dmap->getTiles(mc, tiles);

        int lock;
        bool pick_only = false;
        bool special = false;
        if (lua_isnumber(lua, 2)) {
            lock = lua_tointeger(lua, 2);
        } else if (lua_isstring(lua, 2)) {
            if (std::strcmp(lua_tostring(lua, 2), "special") == 0) {
                special = true;
            } else if (std::strcmp(lua_tostring(lua, 2), "pick_only") == 0) {
                pick_only = true;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
        
        for (std::vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end(); ++it) {
            Tile *tile = it->get();
            Lockable *lockable = dynamic_cast<Lockable*>(tile);
            if (lockable) {
                if (pick_only) lockable->setLockPickOnly();
                else if (special) lockable->setLockSpecial();
                else lockable->setLock(lock);
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
        ItemType * itype = ReadLuaPtr<ItemType>(lua, 3);
        const bool drop_after = lua_toboolean(lua, 4) != 0;

        if (itype) {
            CreateMissile(*dmap, mc, dir, *itype, drop_after, false, GetOriginatorFromCxt(lua), true);
        }

        return 0;
    }

    int AddMonster(lua_State *lua)
    {
        // arg 1 = position
        // arg 2 = monstertype
        // result = creature (or nil if placement failed)
        
        // See also "AddMonsters" in lua_game_setup.cpp

        const MapCoord mc = GetMapCoord(lua, 1);
        const MonsterType *montype = ReadLuaPtr<MonsterType>(lua, 2);
        if (!montype) {
            luaL_error(lua, "Invalid monster type passed to AddMonster");
        }

        boost::shared_ptr<Creature> cr;
        Mediator &m = Mediator::instance();
        boost::shared_ptr<DungeonMap> dmap = m.getMap();
        
        if (dmap 
        && dmap->getAccess(mc, montype->getHeight()) == A_CLEAR
        && montype->okToCreateAt(*dmap, mc)) {  // stops zombies being placed on pits
            cr = m.getMonsterManager().placeMonster(*montype, *dmap, mc, D_NORTH);
        }

        // Return the creature to lua
        NewLuaWeakPtr<Creature>(lua, cr);
        return 1;
    }

    // Input: creature, new position.
    // Cxt: none
    // Output: none
    int TeleportTo(lua_State *lua)
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

        const MapCoord mc = GetMapCoord(lua, 1);

        const Sound *sound = ReadLuaPtr<Sound>(lua, 2);
        if (!sound) return 0;

        const int frequency = lua_tointeger(lua, 3);

        med.playSound(*dmap, mc, *sound, frequency, mc.isNull());
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

    int GetFacing(lua_State *lua)
    {
        shared_ptr<Creature> cr = ReadLuaSharedPtr<Creature>(lua, 1);
        if (cr) {
            PushMapDirection(lua, cr->getFacing());
        } else {
            lua_pushnil(lua);
        }
        return 1;
    }

    int GetNumHeld(lua_State *lua)
    {
        int num = 0;
        shared_ptr<Creature> cr = ReadLuaSharedPtr<Creature>(lua, 1);
        const ItemType * itype = ReadLuaPtr<ItemType>(lua, 2);

        if (cr && itype) {
            if (cr->getItemInHand() == itype) ++num;

            Knight * kt = dynamic_cast<Knight*>(cr.get());
            if (kt) {
                num += kt->getNumCarried(*itype);
            }
        }

        lua_pushinteger(lua, num);
        return 1;
    }

    int GetItemInHand(lua_State *lua)
    {
        shared_ptr<Creature> cr = ReadLuaSharedPtr<Creature>(lua, 1);
        if (cr) {
            NewLuaPtr<ItemType>(lua, cr->getItemInHand());
        } else {
            lua_pushnil(lua);
        }
        return 1;
    }

    int GiveItem(lua_State *lua)
    {
        shared_ptr<Creature> cr = ReadLuaSharedPtr<Creature>(lua, 1);
        shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(cr);
        ItemType *itype = ReadLuaPtr<ItemType>(lua, 2);
        int num = 1;
        if (lua_isnumber(lua, 3)) {
            num = lua_tointeger(lua, 3);
        }

        if (kt && itype && !itype->isBig() && num > 0) {
            kt->addToBackpack(*itype, num);
        }

        return 0;
    }

    // Input: creature
    // Cxt: none
    // Output: boolean
    int IsKnight(lua_State *lua)
    {
        if (lua_gettop(lua) == 0) {
            luaL_error(lua, "IsKnight: requires 1 parameter");
        }
        shared_ptr<Creature> cr = ReadLuaSharedPtr<Creature>(lua, 1);
        lua_pushboolean(lua, cr && dynamic_pointer_cast<Knight>(cr));
        return 1;
    }

    int IsAlive(lua_State *lua)
    {
        if (lua_gettop(lua) == 0) {
            luaL_error(lua, "IsAlive: requires 1 parameter");
        }
        shared_ptr<Creature> cr = ReadLuaSharedPtr<Creature>(lua, 1);
        lua_pushboolean(lua, !!cr);
        return 1;
    }
    
    // Input: none
    // Cxt: actor
    // Output: boolean
    int ActorIsKnight(lua_State *lua)
    {
        lua_getglobal(lua, "cxt");        // [arg cxt]
        lua_getfield(lua, -1, "actor");   // [arg cxt cr]
        boost::shared_ptr<Creature> actor = ReadLuaSharedPtr<Creature>(lua, -1);
        lua_pushboolean(lua, actor && dynamic_pointer_cast<Knight>(actor));
        return 1;
    }

    // called by Print
    std::string BuildMsg(lua_State *lua, int start)
    {
        std::string msg;
        const int top = lua_gettop(lua);
        for (int i = start; i <= top; ++i) {
            const char *x = luaL_tolstring(lua, i, 0);
            if (!x) luaL_error(lua, "'tostring' must return a string to 'print'");
            if (i > start) msg += " ";
            msg += x;
        }
        return msg;
    }

    // Input: (optional) player, followed by any number of args.
    // Cxt: none
    // Output: none
    int Print(lua_State *lua)
    {
        try {
            Mediator &med = Mediator::instance();

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
            
            // print the message
            med.gameMsg(player_num, BuildMsg(lua, start));

        } catch (MediatorUnavailable&) {
            // print it to stdout instead
            // (Assumes no 'player' arg)
            std::string msg = BuildMsg(lua, 1);
            std::cout << msg << std::endl;
        }
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

    // Input: creature; (optional) death type
    // Cxt: originator (optionally).
    // Output: none
    int DestroyCreature(lua_State *lua)
    {
        shared_ptr<Creature> cr = ReadLuaSharedPtr<Creature>(lua, 1);

        Creature::DeathMode dm = Creature::NORMAL_MODE;
        if (lua_isstring(lua, 2)) {
            const char *s = lua_tostring(lua, 2);
            if (std::strcmp(s, "pit") == 0) {
                dm = Creature::PIT_MODE;
            } else if (std::strcmp(s, "zombie") == 0) {
                dm = Creature::ZOMBIE_MODE;
            } else if (std::strcmp(s, "poison") == 0) {
                dm = Creature::POISON_MODE;
            } else if (std::strcmp(s, "normal") == 0) {
                dm = Creature::NORMAL_MODE;
            } else {
                luaL_error(lua, "Death type '%s' is invalid, should be one of 'pit', 'zombie', 'poison', 'normal'");
            }
        }
        
        Originator orig = GetOriginatorFromCxt(lua);

        if (cr && cr->getMap()) {
            cr->onDeath(dm, orig);
            cr->rmFromMap();
        }

        return 0;
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
    int LuaLegacyActionFunc(lua_State *lua)
    {
        // Make a LuaActionPars
        LuaActionPars p(lua);

        // Get the upvalue which is the ActionMaker
        const ActionMaker *maker = static_cast<const ActionMaker *>(lua_touserdata(lua, lua_upvalueindex(2)));

        // Call the maker to produce a LegacyAction.
        // Store in a scoped_ptr
        boost::scoped_ptr<LegacyAction> action( maker->make(p) );

        // Create the ActionData from cxt
        ActionData ad( lua );

        // Call the action
        bool result = action->executeWithResult(ad);

        // Done.
        lua_pushboolean(lua, result);
        return 1;
    }

    int LuaLegacyPossibleFunc(lua_State *lua)
    {
        LuaActionPars p(lua);
        const ActionMaker *maker = static_cast<const ActionMaker *>(lua_touserdata(lua, lua_upvalueindex(2)));
        boost::scoped_ptr<LegacyAction> action( maker->make(p) );
        ActionData ad(lua);
        bool result = action->possible(ad);
        lua_pushboolean(lua, result);
        return 1;
    }
    

    // Input: two integers (low and high of range; inclusive)
    // Cxt: none
    // Output: one integer
    int RandomRange(lua_State *lua)
    {
        if (!g_rng.isInitialized()) {
            luaL_error(lua, "Can't generate random numbers during init phase");
        }

        int low = luaL_checkint(lua, 1);
        int high = luaL_checkint(lua, 2);

        int result = g_rng.getInt(low, high + 1);  // g_rng uses exclusive upper bound; we use inclusive; so add one.

        lua_pushinteger(lua, result);
        return 1;
    }

    int RandomChance(lua_State *lua)
    {
        if (!g_rng.isInitialized()) {
            luaL_error(lua, "Can't generate random numbers during init phase");
        }

        double chance = luaL_checknumber(lua, 1);
        if (chance < 0) chance = 0;
        if (chance > 1) chance = 1;

        lua_pushboolean(lua, g_rng.getBool(float(chance)) ? 1 : 0);
        return 1;
    }

    int GetRandomPos(lua_State *lua)
    {
        if (!g_rng.isInitialized()) {
            luaL_error(lua, "Can't generate random numbers during init phase");
        }

        boost::shared_ptr<DungeonMap> dmap = Mediator::instance().getMap();
        if (!dmap) {
            luaL_error(lua, "Problem in GetRandomPos: Map not created yet");
        }

        const int x = g_rng.getInt(0, dmap->getWidth());
        const int y = g_rng.getInt(0, dmap->getHeight());

        lua_createtable(lua, 0, 2);
        lua_pushinteger(lua, x);
        lua_setfield(lua, -2, "x");
        lua_pushinteger(lua, y);
        lua_setfield(lua, -2, "y");
        return 1;
    }


    //
    // Homes and players
    //
    
    int GetAllHomes(lua_State *lua)
    {
        Mediator::instance().getHomeManager().pushAllHomes(lua);
        return 1;
    }

    int GetHomeFor(lua_State *lua)
    {
        Player & plyr = GetPlayerOrKnight(lua, 1);
        Mediator::instance().getHomeManager().pushHomeFor(lua, plyr);
        return 1;
    }

    int SetHomeFor(lua_State *lua)
    {
        Player & plyr = GetPlayerOrKnight(lua, 1);
        MapCoord pos = GetMapCoord(lua, 2);
        MapDirection facing = GetMapDirection(lua, 3);
        plyr.resetHome(Mediator::instance().getMap().get(), pos, facing);
        return 0;
    }

    int GetPlayer(lua_State *lua)
    {
        shared_ptr<Creature> cr = CheckLuaSharedPtr<Creature>(lua, 1, "Player");
        NewLuaPtr<Player>(lua, cr->getPlayer());
        return 1;
    }

    int GetAllPlayers(lua_State *lua)
    {
        const std::vector<Player*> & plyrs = Mediator::instance().getPlayers();
        lua_createtable(lua, plyrs.size(), 0);
        int n = 1;
        for (std::vector<Player*>::const_iterator it = plyrs.begin(); it != plyrs.end(); ++it) {
            NewLuaPtr<Player>(lua, *it);
            lua_rawseti(lua, -2, n++);
        }
        return 1;
    }

    int GetPlayerName(lua_State *lua)
    {
        Player & plyr = GetPlayerOrKnight(lua, 1);
        lua_pushstring(lua, plyr.getName().asLatin1().c_str());
        return 1;
    }

    int IsEliminated(lua_State *lua)
    {
        Player & pl = CheckLuaPtr<Player>(lua, 1, "Player");
        lua_pushboolean(lua, pl.getElimFlag());
        return 1;
    }

    int EliminatePlayer(lua_State *lua)
    {
        Player & pl = GetPlayerOrKnight(lua, 1);
        Mediator::instance().eliminatePlayer(pl);
        return 0;
    }

    int WinGame(lua_State *lua)
    {
        Player & plyr = GetPlayerOrKnight(lua, 1);
        Mediator::instance().winGame(plyr);
        return 0;
    }

    int GetMonsterLimit(lua_State *lua)
    {
        const MonsterType *montype = ReadLuaPtr<MonsterType>(lua, 1);
        if (!montype) luaL_error(lua, "GetMonsterLimit: parameter 1 must be a monster type");

        const int lim = Mediator::instance().getMonsterManager().getMonsterLimit(*montype);
        if (lim < 0) {
            lua_pushnil(lua);
        } else {
            lua_pushinteger(lua, lim);
        }
        return 1;
    }

    int GetTotalMonsterLimit(lua_State *lua)
    {
        const int lim = Mediator::instance().getMonsterManager().getTotalMonsterLimit();
        if (lim < 0) {
            lua_pushnil(lua);
        } else {
            lua_pushinteger(lua, lim);
        }
        return 1;
    }

    int GetMonsterCount(lua_State *lua)
    {
        const MonsterType *montype = ReadLuaPtr<MonsterType>(lua, 1);
        if (!montype) luaL_error(lua, "GetMonsterCount: parameter 1 must be a monster type");
        
        const int ct = Mediator::instance().getMonsterManager().getMonsterCount(*montype);
        lua_pushinteger(lua, ct);
        return 1;
    }

    int GetTotalMonsterCount(lua_State *lua)
    {
        const int ct = Mediator::instance().getMonsterManager().getTotalMonsterCount();
        lua_pushinteger(lua, ct);
        return 1;
    }

    int AddTask(lua_State *lua)
    {
        lua_createtable(lua, 0, 0); // empty cxt table
        lua_pushvalue(lua, 1);      // the function (1st and only argument of AddTask)
        LuaExecCoroutine(lua, 0);
        return 0;
    }


    //
    // Quests
    //

    int AddHint(lua_State *lua)
    {
        const std::string msg = luaL_checkstring(lua, 1);
        const double order = luaL_checknumber(lua, 2);
        const double group = luaL_checknumber(lua, 3);
        Mediator::instance().addQuestHint(msg, order, group);
        return 0;
    }

    int ClearHints(lua_State *lua)
    {
        Mediator::instance().clearQuestHints();
        return 0;
    }

    int ResendHints(lua_State *lua)
    {
        Mediator::instance().sendQuestHints();
        return 0;
    }

    //
    // Items
    //

    int PlaceItem(lua_State *lua)
    {
        const MapCoord mc = GetMapCoord(lua, 1);
        ItemType *itype = ReadLuaPtr<ItemType>(lua, 2);
        DungeonMap *dmap = Mediator::instance().getMap().get();
        
        if (itype && dmap) {

            int num = 1;
            if (lua_isnumber(lua, 3)) num = lua_tointeger(lua, 3);
            boost::shared_ptr<Item> item(new Item(*itype, num));
            
            // see if we can "place" the item
            std::vector<shared_ptr<Tile> > tiles;
            dmap->getTiles(mc, tiles);
            for (std::vector<shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
                if ((*it)->canPlaceItem()) {
                    if ((*it)->itemPlacedAlready()) {
                        // failure
                        lua_pushboolean(lua, false);
                        return 1;
                    } else {
                        (*it)->placeItem(item);
                        lua_pushboolean(lua, true);
                        return 1;
                    }
                }
            }

            // put item directly in dungeon map
            lua_pushboolean(lua, dmap->addItem(mc, item));
            return 1;
        }

        lua_pushboolean(lua, false);
        return 1;
    }
                
                
                
    //
    // Tutorial
    //

    ColourChange PopColourChange(lua_State *lua)
    {
        // [cc]
        // where cc is a list of alternating from-colour, to-colour, from-colour, to-colour etc.

        ColourChange result;
        
        if (!lua_isnil(lua, -1)) {

            lua_len(lua, -1);  // [cc len]
            const int sz = lua_tointeger(lua, -1);
            lua_pop(lua, 1);   // [cc]

            if (sz % 2 != 0) {
                luaL_error(lua, "Colour-change list must have even number of elements");
            }

            for (int i = 0; i < sz/2; ++i) {
                lua_pushinteger(lua, i*2 + 1); // [cc fromidx]
                lua_gettable(lua, -2);         // [cc from]
                const int from = lua_tointeger(lua, -1);
                lua_pop(lua, 1);  // [cc]

                lua_pushinteger(lua, i*2 + 2);
                lua_gettable(lua, -2);
                const int to = lua_tointeger(lua, -1);
                lua_pop(lua, 1); // [cc]

                result.add(Colour((from >> 16) & 255, (from >> 8) & 255, from & 255),
                           Colour((to   >> 16) & 255, (to   >> 8) & 255, to   & 255));
            }
        }

        lua_pop(lua, 1);
        return result;
    }
                
    int PopUpWindow(lua_State *lua)
    {
        // Read out the table
        // Note: we interpret the lua-strings as Latin1 for now.
        TutorialWindow win;
        
        lua_getfield(lua, 1, "title");  // [title]
        if (lua_isstring(lua, -1)) win.title_latin1 = lua_tostring(lua, -1);
        lua_pop(lua, 1);  // []

        lua_getfield(lua, 1, "body");  // [body]
        if (lua_isstring(lua, -1)) win.msg_latin1 = lua_tostring(lua, -1);
        lua_pop(lua, 1); // []

        lua_getfield(lua, 1, "popup");  // [popup]
        win.popup = lua_toboolean(lua, -1) != 0;
        lua_pop(lua, 1); // []

        lua_getfield(lua, 1, "graphics");  // [graphics]
        if (!lua_isnil(lua, -1)) {
            lua_len(lua, -1);   // [gfx len]
            const int n = lua_tointeger(lua, -1);
            lua_pop(lua, 1);  // [gfx]

            for (int i = 1; i <= n; ++i) {
                lua_pushinteger(lua, i);  // [gfx i]
                lua_gettable(lua, -2);    // [gfx g]
                win.gfx.push_back(ReadLuaPtr<Graphic>(lua, -1));
                lua_pop(lua, 1);  // [gfx]
            }
        }
        lua_pop(lua, 1);  // []

        lua_getfield(lua, 1, "colour_changes");  // [cctbl]
        if (!lua_isnil(lua, -1)) {
            for (int i = 1; i <= int(win.gfx.size()); ++i) {
                lua_pushinteger(lua, i);  // [cctbl i]
                lua_gettable(lua, -2);  // [cctbl cc]
                win.cc.push_back(PopColourChange(lua)); // [cctbl]
            }
        } else {
            win.cc.resize(win.gfx.size()); // fill with empty colour changes
        }
        
        Mediator::instance().getCallbacks().popUpWindow(std::vector<TutorialWindow>(1, win));
        return 0;
    }

    int StartTutorialManager(lua_State *lua)
    {
        boost::shared_ptr<TutorialManager> tm(new TutorialManager);

        // tutorial table at stack position 1
        
        lua_len(lua, 1);  // [len]
        const int sz = lua_tointeger(lua, -1);
        lua_pop(lua, 1);  // []

        if (sz % 3 != 0) luaL_error(lua, "Tutorial list size must be a multiple of 3");

        for (int i = 0; i < sz; i += 3) {
            lua_pushinteger(lua, i+1);  // [1]
            lua_gettable(lua, 1);       // [key]
            const int t_key = lua_tointeger(lua, -1);
            lua_pop(lua, 1);  // []

            lua_pushinteger(lua, i+2);  // [2]
            lua_gettable(lua, 1);   // [title]
            const char * title_c = lua_tostring(lua, -1);
            std::string title = title_c ? title_c : "";
            lua_pop(lua, 1);  // []
        
            lua_pushinteger(lua, i+3);  // [3]
            lua_gettable(lua, 1); // [msg]
            const char * msg_c = lua_tostring(lua, -1);
            std::string msg = msg_c ? msg_c : "";
            lua_pop(lua, 1);  // []

            tm->addTutorialKey(t_key, title, msg);
        }

        Mediator::instance().setTutorialManager(tm);
        
        return 0;
    }

    //
    // Traps
    //

    void SetTrapImpl(const MapCoord &mc, const Originator &orig, boost::shared_ptr<Trap> trap)
    {
        DungeonMap *dmap = Mediator::instance().getMap().get();
        if (!dmap) return;
        
        std::vector<boost::shared_ptr<Tile> > tiles;
        Lockable *result = 0;
        dmap->getTiles(mc, tiles);
        for (std::vector<boost::shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
            result = dynamic_cast<Lockable*>(it->get());
            if (result) break;
        }

        if (result) {
            result->setTrap(*dmap, mc, boost::shared_ptr<Creature>(), orig, trap);
        }
    }
    
    int SetPoisonTrap(lua_State *lua)
    {
        MapCoord mc = GetMapCoord(lua, 1);
        ItemType *trap_item = ReadLuaPtr<ItemType>(lua, 2);
        boost::shared_ptr<Trap> trap(new PoisonTrap(trap_item));
        SetTrapImpl(mc, GetOriginatorFromCxt(lua), trap);
        return 0;
    }

    int SetBladeTrap(lua_State *lua)
    {
        MapCoord mc = GetMapCoord(lua, 1);
        ItemType *trap_item = ReadLuaPtr<ItemType>(lua, 2);
        ItemType *bolt_item = ReadLuaPtr<ItemType>(lua, 3);
        MapDirection dir = GetMapDirection(lua, 4);
        if (bolt_item) {
            boost::shared_ptr<Trap> trap(new BladeTrap(trap_item, *bolt_item, dir));
            SetTrapImpl(mc, GetOriginatorFromCxt(lua), trap);
        }
        return 0;
    }
}

void AddLuaIngameFunctions(lua_State *lua)
{
    // all functions go in "kts" table.
    lua_rawgeti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);   // [env]
    luaL_getsubtable(lua, -1, "kts");                         // [env kts]
    
    PushCFunction(lua, &Open);
    lua_setfield(lua, -2, "OpenDoor");

    PushCFunction(lua, &Close);
    lua_setfield(lua, -2, "CloseDoor");

    PushCFunction(lua, &OpenOrClose);
    lua_setfield(lua, -2, "OpenOrCloseDoor");

    PushCFunction(lua, &LockDoor);
    lua_setfield(lua, -2, "LockDoor");
    
    PushCFunction(lua, &IsOpen);
    lua_setfield(lua, -2, "IsDoorOpen");
    
    PushCFunction(lua, &AddMissile);
    lua_setfield(lua, -2, "AddMissile");

    PushCFunction(lua, &AddMonster);
    lua_setfield(lua, -2, "AddMonster");
    
    PushCFunction(lua, &GetTiles);
    lua_setfield(lua, -2, "GetTiles");

    PushCFunction(lua, &RemoveTile);
    lua_setfield(lua, -2, "RemoveTile");

    PushCFunction(lua, &AddTile);
    lua_setfield(lua, -2, "AddTile");

    PushCFunction(lua, &PlaySound);
    lua_setfield(lua, -2, "PlaySound");

    // "print" is set globally, NOT in kts table
    PushCFunction(lua, &Print);       // [env kts Print]
    lua_setfield(lua, -3, "print");   // [env kts]

    PushCFunction(lua, &RotateAddPos);
    lua_setfield(lua, -2, "RotateAddPos");

    PushCFunction(lua, &RotateDirection);
    lua_setfield(lua, -2, "RotateDirection");

    // Now we want to add all the LegacyActions as Lua functions.
    {
        boost::unique_lock<boost::mutex> lock(g_makers_mutex);
        const std::map<std::string, const ActionMaker *> & makers_map = MakersMap();
        for (std::map<std::string, const ActionMaker *>::const_iterator it = makers_map.begin();
        it != makers_map.end(); ++it) {
            // const_cast is ok: we promise to cast it back to const again when 
            // we get it back from Lua...
            lua_pushlightuserdata(lua, const_cast<ActionMaker*>(it->second));
            PushCClosure(lua, &LuaLegacyActionFunc, 1);
            lua_setfield(lua, -2, it->first.c_str());

            lua_pushlightuserdata(lua, const_cast<ActionMaker*>(it->second));
            PushCClosure(lua, &LuaLegacyPossibleFunc, 1);
            lua_setfield(lua, -2, ("Can_" + it->first).c_str());
        }
    }

    // Function to return total elapsed time of this game, in ms.
    PushCFunction(lua, &GameTime);
    lua_setfield(lua, -2, "GameTime");
    
    // Random number generation functions.
    PushCFunction(lua, &RandomRange);
    lua_setfield(lua, -2, "RandomRange");

    PushCFunction(lua, &RandomChance);
    lua_setfield(lua, -2, "RandomChance");

    PushCFunction(lua, &GetRandomPos);
    lua_setfield(lua, -2, "GetRandomPos");

    // Homes and Players
    PushCFunction(lua, &GetAllHomes);
    lua_setfield(lua, -2, "GetAllHomes");

    PushCFunction(lua, &GetHomeFor);
    lua_setfield(lua, -2, "GetHomeFor");

    PushCFunction(lua, &SetHomeFor);
    lua_setfield(lua, -2, "SetHomeFor");

    PushCFunction(lua, &GetAllPlayers);
    lua_setfield(lua, -2, "GetAllPlayers");

    PushCFunction(lua, &GetPlayerName);
    lua_setfield(lua, -2, "GetPlayerName");

    PushCFunction(lua, &IsEliminated);
    lua_setfield(lua, -2, "IsEliminated");

    PushCFunction(lua, &EliminatePlayer);
    lua_setfield(lua, -2, "EliminatePlayer");
    
    PushCFunction(lua, &WinGame);
    lua_setfield(lua, -2, "WinGame");

    PushCFunction(lua, &GetMonsterLimit);
    lua_setfield(lua, -2, "GetMonsterLimit");
    PushCFunction(lua, &GetTotalMonsterLimit);
    lua_setfield(lua, -2, "GetTotalMonsterLimit");
    PushCFunction(lua, &GetMonsterCount);
    lua_setfield(lua, -2, "GetMonsterCount");
    PushCFunction(lua, &GetTotalMonsterCount);
    lua_setfield(lua, -2, "GetTotalMonsterCount");

    PushCFunction(lua, &AddTask);
    lua_setfield(lua, -2, "AddTask");



    // Creatures

    PushCFunction(lua, &ActorIsKnight);
    lua_setfield(lua, -2, "ActorIsKnight");

    // Stun a knight and also yield for that length of time.
    PushCFunction(lua, &Delay);
    lua_setfield(lua, -2, "Delay");

    PushCFunction(lua, &DestroyCreature);
    lua_setfield(lua, -2, "DestroyCreature");

    PushCFunction(lua, &GetFacing);
    lua_setfield(lua, -2, "GetFacing");

    PushCFunction(lua, &GetItemInHand);
    lua_setfield(lua, -2, "GetItemInHand");

    PushCFunction(lua, &GetNumHeld);
    lua_setfield(lua, -2, "GetNumHeld");

    PushCFunction(lua, &GetPlayer);
    lua_setfield(lua, -2, "GetPlayer");

    PushCFunction(lua, &GetPos);
    lua_setfield(lua, -2, "GetPos");

    PushCFunction(lua, &GiveItem);
    lua_setfield(lua, -2, "GiveItem");
    
    PushCFunction(lua, &IsAlive);
    lua_setfield(lua, -2, "IsAlive");
    
    PushCFunction(lua, &IsKnight);
    lua_setfield(lua, -2, "IsKnight");
    
    PushCFunction(lua, &TeleportTo);
    lua_setfield(lua, -2, "TeleportTo");

    
    // Items

    PushCFunction(lua, &PlaceItem);
    lua_setfield(lua, -2, "PlaceItem");
    

    // Quests

    PushCFunction(lua, &AddHint);
    lua_setfield(lua, -2, "AddHint");

    PushCFunction(lua, &ClearHints);
    lua_setfield(lua, -2, "ClearHints");

    PushCFunction(lua, &ResendHints);
    lua_setfield(lua, -2, "ResendHints");


    // Tutorial

    PushCFunction(lua, &PopUpWindow);
    lua_setfield(lua, -2, "PopUpWindow");

    PushCFunction(lua, &StartTutorialManager);
    lua_setfield(lua, -2, "StartTutorialManager");


    // Traps

    PushCFunction(lua, &SetPoisonTrap);
    lua_setfield(lua, -2, "SetPoisonTrap");

    PushCFunction(lua, &SetBladeTrap);
    lua_setfield(lua, -2, "SetBladeTrap");
    
        
    // pop the "kts" and environment tables.
    lua_pop(lua, 2);
}

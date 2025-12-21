/*
 * script_actions.cpp
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

#include "action_data.hpp"
#include "creature.hpp"
#include "dungeon_map.hpp"
#include "dungeon_view.hpp"
#include "item.hpp"
#include "knight.hpp"
#include "knights_callbacks.hpp"
#include "map_support.hpp"
#include "mediator.hpp"
#include "missile.hpp"
#include "monster.hpp"
#include "monster_manager.hpp"  // for Necromancy, *ZombieActivity
#include "player.hpp"
#include "room_map.hpp"
#include "script_actions.hpp"
#include "status_display.hpp"
#include "task_manager.hpp"
#include "teleport.hpp"
#include "tile.hpp"

#include "include_lua.hpp"
#include "lua_func_wrapper.hpp"
#include "lua_userdata.hpp"

namespace {
    // Input: none
    // Cxt: actor
    // Output: none
    int CrystalStart(lua_State *lua)
    {
        lua_getglobal(lua, "cxt");        // [cxt]
        lua_getfield(lua, -1, "actor");   // [cxt cr]
        boost::shared_ptr<Creature> actor = ReadLuaSharedPtr<Creature>(lua, -1);
        boost::shared_ptr<Knight> kt = boost::dynamic_pointer_cast<Knight>(actor);
        if (kt) kt->setCrystalBall(true);
        lua_pop(lua, 2);  // pop cxt and actor
        return 0;
    }

    // Input: none
    // Cxt: actor
    // Output: none
    int CrystalStop(lua_State *lua)
    {
        lua_getglobal(lua, "cxt");        // [cxt]
        lua_getfield(lua, -1, "actor");   // [cxt cr]
        boost::shared_ptr<Creature> actor = ReadLuaSharedPtr<Creature>(lua, -1);
        boost::shared_ptr<Knight> kt = boost::dynamic_pointer_cast<Knight>(actor);
        if (kt) kt->setCrystalBall(false);
        lua_pop(lua, 2);  // pop cxt and actor
        return 0;
    }

    // Input: none
    // Cxt: actor
    // Output: none
    int RevealStart(lua_State *lua)
    {
        lua_getglobal(lua, "cxt");        // [cxt]
        lua_getfield(lua, -1, "actor");   // [cxt cr]
        boost::shared_ptr<Creature> actor = ReadLuaSharedPtr<Creature>(lua, -1);
        boost::shared_ptr<Knight> kt = boost::dynamic_pointer_cast<Knight>(actor);
        if (kt) kt->setReveal2(true);
        lua_pop(lua, 2);  // pop cxt and actor
        return 0;
    }

    // Input: none
    // Cxt: actor
    // Output: none
    int RevealStop(lua_State *lua)
    {
        lua_getglobal(lua, "cxt");        // [cxt]
        lua_getfield(lua, -1, "actor");   // [cxt cr]
        boost::shared_ptr<Creature> actor = ReadLuaSharedPtr<Creature>(lua, -1);
        boost::shared_ptr<Knight> kt = boost::dynamic_pointer_cast<Knight>(actor);
        if (kt) kt->setReveal2(false);
        lua_pop(lua, 2);  // pop cxt and actor
        return 0;
    }

    // Input: none
    // Cxt: none
    // Output: none
    int FullZombieActivity(lua_State *lua)
    {
        Mediator::instance().getMonsterManager().fullZombieActivity();
        return 0;
    }

    // Input: none
    // Cxt: none
    // Output: none
    int NormalZombieActivity(lua_State *lua)
    {
        Mediator::instance().getMonsterManager().normalZombieActivity();
        return 0;
    }

    // This directly kills the actor, without blood/gore effects, but only if the
    // actor is at height H_WALKING. Used for pits.
    // Input: none
    // Cxt: actor, originator
    // Output: none
    int PitKill(lua_State *lua)
    {
        lua_getglobal(lua, "cxt");        // [cxt]
        lua_getfield(lua, -1, "actor");   // [cxt cr]
        shared_ptr<Creature> cr = ReadLuaSharedPtr<Creature>(lua, -1);
        lua_pop(lua, 2);  // pop cxt and actor

        if (!cr || !cr->getMap()) return 0;
        if (cr->getHeight() != H_WALKING) return 0;

        cr->onDeath(Creature::PIT_MODE, GetOriginatorFromCxt(lua));
        cr->rmFromMap();
        return 0;
    }

    // Input: optional int delay (arg 1, default 0)
    // Cxt: actor
    // Output: none
    int FlashScreen(lua_State *lua)
    {
        int delay = 0;
        if (lua_gettop(lua) >= 1) {
            delay = luaL_checkinteger(lua, 1);
        }

        lua_getglobal(lua, "cxt");        // [cxt]
        lua_getfield(lua, -1, "actor");   // [cxt cr]
        shared_ptr<Creature> actor = ReadLuaSharedPtr<Creature>(lua, -1);
        lua_pop(lua, 2);  // pop cxt and actor

        // Only flash for Knights (not zombies - otherwise the screen would
        // flash every time a zombie walked over a pentagram, which would
        // be distracting)
        if (dynamic_cast<Knight*>(actor.get())) {
            Mediator::instance().flashScreen(actor, delay);
        }
        return 0;
    }

    // Input: localization msg (arg 1), optional int num_times (arg 2, default 4)
    // Cxt: actor
    // Output: none
    int FlashMessage(lua_State *lua)
    {
        lua_pushvalue(lua, 1);
        LocalMsg msg = PopLocalMsgFromLua(lua);

        int num_times = 4;
        if (lua_gettop(lua) >= 2) {
            num_times = luaL_checkinteger(lua, 2);
        }

        lua_getglobal(lua, "cxt");        // [cxt]
        lua_getfield(lua, -1, "actor");   // [cxt cr]
        shared_ptr<Creature> actor = ReadLuaSharedPtr<Creature>(lua, -1);
        lua_pop(lua, 2);  // pop cxt and actor

        if (actor && actor->getPlayer()) {
            actor->getPlayer()->getDungeonView().flashMessage(msg, num_times);
        }
        return 0;
    }

    // This damages the *actor* (e.g. used for bear traps)
    // Input: int amount (arg 1), int stun_time (arg 2), optional bool inhibit_squelch (arg 3)
    // Cxt: actor, originator
    // Output: none
    int Damage(lua_State *lua)
    {
        int amount = luaL_checkinteger(lua, 1);
        int stun_time = luaL_checkinteger(lua, 2);
        bool inhibit_squelch = false;
        if (lua_gettop(lua) >= 3) {
            inhibit_squelch = lua_toboolean(lua, 3) != 0;
        }

        lua_getglobal(lua, "cxt");        // [cxt]
        lua_getfield(lua, -1, "actor");   // [cxt cr]
        shared_ptr<Creature> cr = ReadLuaSharedPtr<Creature>(lua, -1);
        lua_pop(lua, 2);  // pop cxt and actor

        if (cr) {
            const int stun = stun_time > 0 ? stun_time : 0;
            cr->damage(amount, GetOriginatorFromCxt(lua),
                      Mediator::instance().getGVT() + stun, inhibit_squelch);
        }
        return 0;
    }

    // Input: ItemType* item_type (arg 1)
    // Cxt: pos (generic position)
    // Output: none
    int ChangeItem(lua_State *lua)
    {
        ItemType *item_type = ReadLuaPtr<ItemType>(lua, 1);
        if (!item_type) return 0;

        lua_getglobal(lua, "cxt");        // [cxt]
        lua_getfield(lua, -1, "pos");     // [cxt pos]
        MapCoord mc = GetMapCoord(lua, -1);
        lua_pop(lua, 2);  // pop cxt and pos

        DungeonMap *dmap = Mediator::instance().getMap().get();
        if (!dmap || mc.isNull()) return 0;

        dmap->rmItem(mc);
        shared_ptr<Item> new_item(new Item(*item_type));
        dmap->addItem(mc, new_item);
        return 0;
    }

    // Input: shared_ptr<Tile> new_tile (arg 1)
    // Cxt: tile (old tile), pos, originator
    // Output: none
    int ChangeTile(lua_State *lua)
    {
        shared_ptr<Tile> new_tile = ReadLuaSharedPtr<Tile>(lua, 1);
        if (!new_tile) return 0;

        lua_getglobal(lua, "cxt");        // [cxt]
        lua_getfield(lua, -1, "tile");    // [cxt tile]
        shared_ptr<Tile> old_tile = ReadLuaSharedPtr<Tile>(lua, -1);
        lua_getfield(lua, -2, "pos");     // [cxt tile pos]
        MapCoord mc = GetMapCoord(lua, -1);
        lua_pop(lua, 3);  // pop cxt, tile, and pos

        DungeonMap *dmap = Mediator::instance().getMap().get();
        if (!dmap || !old_tile) return 0;

        Originator orig = GetOriginatorFromCxt(lua);
        dmap->rmTile(mc, old_tile, orig);
        dmap->addTile(mc, new_tile->clone(false), orig);
        return 0;
    }

    // Input: int nzoms (arg 1), int range (arg 2)
    // Cxt: pos (generic position)
    // Output: boolean (true if successful, false if already done or no map)
    int Necromancy(lua_State *lua)
    {
        int nzoms = luaL_checkinteger(lua, 1);
        int range = luaL_checkinteger(lua, 2);

        // Check if possible (necromancy not already done)
        if (Mediator::instance().getMonsterManager().hasNecromancyBeenDone()) {
            lua_pushboolean(lua, false);
            return 1;
        }

        lua_getglobal(lua, "cxt");        // [cxt]
        lua_getfield(lua, -1, "pos");     // [cxt pos]
        MapCoord pos = GetMapCoord(lua, -1);
        lua_pop(lua, 2);  // pop cxt and pos

        DungeonMap *dmap = Mediator::instance().getMap().get();
        if (dmap) {
            Mediator::instance().getMonsterManager().doNecromancy(nzoms, *dmap,
                    pos.getX() - range,     pos.getY() - range,
                    pos.getX() + range + 1, pos.getY() + range + 1);
            lua_pushboolean(lua, true);
            return 1;
        }

        lua_pushboolean(lua, false);
        return 1;
    }

    // Kills the *victim* if it has a given monster type. Used as melee_action
    // for the wand of undeath.
    // Input: MonsterType* zom_type (arg 1)
    // Cxt: victim, originator
    // Output: boolean (true if killed, false otherwise)
    int ZombieKill(lua_State *lua)
    {
        const MonsterType *zom_type = ReadLuaPtr<MonsterType>(lua, 1);
        if (!zom_type) {
            lua_pushboolean(lua, false);
            return 1;
        }

        lua_getglobal(lua, "cxt");        // [cxt]
        lua_getfield(lua, -1, "victim");  // [cxt victim]
        shared_ptr<Creature> victim = ReadLuaSharedPtr<Creature>(lua, -1);
        lua_pop(lua, 2);  // pop cxt and victim

        shared_ptr<Monster> cr = dynamic_pointer_cast<Monster>(victim);
        if (cr && cr->getMap() && &cr->getMonsterType() == zom_type) {
            cr->onDeath(Creature::NORMAL_MODE, GetOriginatorFromCxt(lua));
            cr->rmFromMap();
            lua_pushboolean(lua, true);
            return 1;
        }

        lua_pushboolean(lua, false);
        return 1;
    }
}

void AddLuaScriptFunctions(lua_State *lua)
{
    // all functions go in "kts" table.
    lua_rawgeti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);   // [env]
    luaL_getsubtable(lua, -1, "kts");                         // [env kts]

    PushCFunction(lua, &ChangeItem);
    lua_setfield(lua, -2, "ChangeItem");

    PushCFunction(lua, &ChangeTile);
    lua_setfield(lua, -2, "ChangeTile");

    PushCFunction(lua, &CrystalStart);
    lua_setfield(lua, -2, "CrystalStart");

    PushCFunction(lua, &CrystalStop);
    lua_setfield(lua, -2, "CrystalStop");

    PushCFunction(lua, &Damage);
    lua_setfield(lua, -2, "Damage");

    PushCFunction(lua, &FlashMessage);
    lua_setfield(lua, -2, "FlashMessage");

    PushCFunction(lua, &FlashScreen);
    lua_setfield(lua, -2, "FlashScreen");

    PushCFunction(lua, &FullZombieActivity);
    lua_setfield(lua, -2, "FullZombieActivity");

    PushCFunction(lua, &Necromancy);
    lua_setfield(lua, -2, "Necromancy");

    PushCFunction(lua, &NormalZombieActivity);
    lua_setfield(lua, -2, "NormalZombieActivity");

    PushCFunction(lua, &PitKill);
    lua_setfield(lua, -2, "PitKill");

    PushCFunction(lua, &RevealStart);
    lua_setfield(lua, -2, "RevealStart");

    PushCFunction(lua, &RevealStop);
    lua_setfield(lua, -2, "RevealStop");

    PushCFunction(lua, &ZombieKill);
    lua_setfield(lua, -2, "ZombieKill");

    // pop the "kts" and environment tables.
    lua_pop(lua, 2);
}

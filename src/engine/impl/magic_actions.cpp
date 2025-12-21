/*
 * magic_actions.cpp
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
#include "dispel_magic.hpp"
#include "dungeon_view.hpp"
#include "knight.hpp"
#include "lockable.hpp"
#include "magic_actions.hpp"
#include "magic_map.hpp"
#include "map_support.hpp"
#include "mediator.hpp"
#include "monster_manager.hpp"
#include "my_exceptions.hpp"
#include "player.hpp"
#include "potion_magic.hpp"
#include "task_manager.hpp"
#include "teleport.hpp"
#include "tile.hpp"

#include "include_lua.hpp"
#include "lua_func_wrapper.hpp"
#include "lua_userdata.hpp"

// Helper functions
namespace {
    void SetPotion(int gvt, shared_ptr<Knight> kt, PotionMagic pm, int dur)
    {
        if (!kt) return;
        int stop_time = gvt;
        if (dur > 0) stop_time += dur;
        kt->setPotionMagic(pm, stop_time);
    }

    shared_ptr<Creature> GetActorAsCreature(lua_State *lua)
    {
        lua_getglobal(lua, "cxt");
        lua_getfield(lua, -1, "actor");
        shared_ptr<Creature> actor = ReadLuaSharedPtr<Creature>(lua, -1);
        lua_pop(lua, 2);
        return actor;
    }

    shared_ptr<Knight> GetActorAsKnight(lua_State *lua)
    {
        return dynamic_pointer_cast<Knight>(GetActorAsCreature(lua));
    }

    void FlashMessage(shared_ptr<Knight> kt, const std::string &msg)
    {
        if (!kt) return;
        kt->getPlayer()->getDungeonView().flashMessage(msg, 4);
    }

    void FlashMessage(lua_State *lua, const std::string &msg)
    {
        FlashMessage(GetActorAsKnight(lua), msg);
    }

    bool ZombifyCreature(shared_ptr<Creature> cr, const MonsterType &zom_type, const Originator &originator)
    {
        // Only Knights can be zombified. This prevents zombification
        // of vampire bats or other weird things like that. Of course,
        // if new monster types are added (and they are to be
        // zombifiable) then this rule might need to change ... (maybe
        // only zombify things which are not already of type "zom_type"
        // and are at height H_WALKING?)
        shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(cr);
        if (kt && kt->getMap()) {
            DungeonMap *dmap = kt->getMap();
            MapCoord mc = kt->getPos();
            MapDirection facing = kt->getFacing();
            kt->onDeath(Creature::ZOMBIE_MODE, originator);       // this drops items, etc.
            kt->rmFromMap();
            MonsterManager &mm = Mediator::instance().getMonsterManager();
            mm.placeMonster(zom_type, *dmap, mc, facing);
            return true;
        } else {
            return false;
        }
    }
}

// Lua function implementations
namespace {
    // Input: none
    // Cxt: actor
    // Output: none
    int MagicMapping(lua_State *lua)
    {
        shared_ptr<Knight> kt = GetActorAsKnight(lua);
        if (kt) ::MagicMapping(kt);
        return 0;
    }

    // Input: none
    // Cxt: actor
    // Output: none
    int WipeMap(lua_State *lua)
    {
        shared_ptr<Knight> kt = GetActorAsKnight(lua);
        if (kt) ::WipeMap(kt);
        return 0;
    }

    // Input: none
    // Cxt: actor
    // Output: none
    int Attractor(lua_State *lua)
    {
        shared_ptr<Creature> cr = GetActorAsCreature(lua);
        if (cr && cr->getMap()) {
            shared_ptr<Knight> who_to_teleport = FindRandomOtherKnight(boost::dynamic_pointer_cast<Knight>(cr));
            if (who_to_teleport) {
                TeleportToRoom(who_to_teleport, cr);
            } else {
                // If no other knights exist then teleport myself randomly instead.
                TeleportToRandomSquare(cr);
            }
        }
        return 0;
    }

    // Input: none
    // Cxt: actor
    // Output: none
    int TeleportRandom(lua_State *lua)
    {
        // Only Knights can teleport, because we don't want zombies to be teleported about
        // by pentagrams....
        shared_ptr<Knight> kt = GetActorAsKnight(lua);
        if (kt && kt->getMap()) {
            shared_ptr<Knight> where_to_teleport_to = FindRandomOtherKnight(kt);
            if (where_to_teleport_to) {
                TeleportToRoom(kt, where_to_teleport_to);
            } else {
                // If no other knights exist then just teleport myself randomly
                TeleportToRandomSquare(kt);
            }
        }
        return 0;
    }

    // Input: none
    // Cxt: actor, tile, tile_pos
    // Output: none
    int OpenWays(lua_State *lua)
    {
        lua_getglobal(lua, "cxt");
        lua_getfield(lua, -1, "actor");
        shared_ptr<Creature> actor = ReadLuaSharedPtr<Creature>(lua, -1);
        lua_getfield(lua, -2, "tile");
        shared_ptr<Tile> t = ReadLuaSharedPtr<Tile>(lua, -1);
        lua_getfield(lua, -3, "tile_pos");
        MapCoord mc = GetMapCoord(lua, -1);
        lua_pop(lua, 4);

        DungeonMap *dm = Mediator::instance().getMap().get();
        if (!dm || !t) return 0;

        // If the tile is a "lockable" (i.e. a door or chest) then open it.
        shared_ptr<Lockable> lock = dynamic_pointer_cast<Lockable>(t);
        if (lock) {
            Originator orig = GetOriginatorFromCxt(lua);
            lock->open(*dm, mc, orig);
        } else if (actor && actor->hasStrength()) {
            // We turn off allow_strength for wand of open ways, so the normal melee code
            // will not attempt to damage the tile if the knight has strength. This is to
            // prevent the melee code from destroying the tile before the above code has a
            // chance to open it.
            // However for other types of tile we DO want the normal smashing to take
            // place. So we do it here:
            t->damage(*dm, mc, 9999, actor);
        }
        return 0;
    }

    // Input: int dur (arg 1)
    // Cxt: actor
    // Output: none
    int Paralyzation(lua_State *lua)
    {
        int dur = luaL_checkinteger(lua, 1);
        shared_ptr<Knight> kt = GetActorAsKnight(lua);
        SetPotion(Mediator::instance().getGVT(), kt, PARALYZATION, dur);
        return 0;
    }

    // Input: int dur (arg 1)
    // Cxt: actor
    // Output: none
    int RevealLocation(lua_State *lua)
    {
        int dur = luaL_checkinteger(lua, 1);
        shared_ptr<Knight> kt = GetActorAsKnight(lua);
        if (kt) {
            const int stop_time = Mediator::instance().getGVT() + (dur > 0 ? dur : 0);
            kt->setRevealLocation(true, stop_time);
        }
        return 0;
    }

    // Input: int dur (arg 1)
    // Cxt: actor
    // Output: none
    int SenseItems(lua_State *lua)
    {
        int dur = luaL_checkinteger(lua, 1);
        shared_ptr<Knight> kt = GetActorAsKnight(lua);
        if (kt) {
            const int stop_time = Mediator::instance().getGVT() + (dur > 0 ? dur : 0);
            ::SenseItems(kt, stop_time);
        }
        return 0;
    }

    // Input: int dur (arg 1)
    // Cxt: actor
    // Output: none
    int SenseKnight(lua_State *lua)
    {
        int dur = luaL_checkinteger(lua, 1);
        shared_ptr<Knight> kt = GetActorAsKnight(lua);
        if (kt) {
            const int stop_time = Mediator::instance().getGVT() + (dur > 0 ? dur : 0);
            kt->setSenseKnight(true, stop_time);
        }
        return 0;
    }

    // Input: string msg (arg 1)
    // Cxt: actor
    // Output: none
    int DispelMagic(lua_State *lua)
    {
        const std::string msg = luaL_checkstring(lua, 1);

        FlashMessage(lua, msg);
        ::DispelMagic(Mediator::instance().getPlayers());
        return 0;
    }

    // Input: string msg (arg 1)
    // Cxt: actor
    // Output: none
    int Healing(lua_State *lua)
    {
        const std::string msg = luaL_checkstring(lua, 1);
        shared_ptr<Creature> actor = GetActorAsCreature(lua);
        if (actor && actor->getHealth() < actor->getMaxHealth()) {
            // reset his health to maximum
            FlashMessage(lua, msg);
            actor->addToHealth(actor->getMaxHealth() - actor->getHealth());
        }
        return 0;
    }

    // Input: string msg (arg 1)
    // Cxt: actor, originator
    // Output: none
    int Poison(lua_State *lua)
    {
        const std::string msg = luaL_checkstring(lua, 1);
        shared_ptr<Creature> actor = GetActorAsCreature(lua);
        if (actor) {
            FlashMessage(lua, msg);
            actor->poison(GetOriginatorFromCxt(lua));
        }
        return 0;
    }

    // Input: int dur (arg 1), string msg (arg 2)
    // Cxt: actor
    // Output: none
    int Invisibility(lua_State *lua)
    {
        int dur = luaL_checkinteger(lua, 1);
        const std::string msg = luaL_checkstring(lua, 2);
        shared_ptr<Knight> kt = GetActorAsKnight(lua);
        FlashMessage(kt, msg);
        SetPotion(Mediator::instance().getGVT(), kt, INVISIBILITY, dur);
        return 0;
    }

    // Input: int dur (arg 1), string msg (arg 2)
    // Cxt: actor
    // Output: none
    int Invulnerability(lua_State *lua)
    {
        int dur = luaL_checkinteger(lua, 1);
        const std::string msg = luaL_checkstring(lua, 2);
        shared_ptr<Knight> kt = GetActorAsKnight(lua);
        if (kt) {
            FlashMessage(kt, msg);
            int stop_time = Mediator::instance().getGVT();
            if (dur > 0) stop_time += dur;
            kt->setInvulnerability(true, stop_time);
        }
        return 0;
    }

    // Input: int dur (arg 1), string msg (arg 2)
    // Cxt: actor
    // Output: none
    int PoisonImmunity(lua_State *lua)
    {
        int dur = luaL_checkinteger(lua, 1);
        const std::string msg = luaL_checkstring(lua, 2);
        shared_ptr<Knight> kt = GetActorAsKnight(lua);
        if (kt) {
            FlashMessage(kt, msg);
            const int stop_time = Mediator::instance().getGVT() + (dur > 0 ? dur : 0);
            kt->setPoisonImmunity(true, stop_time);
        }
        return 0;
    }

    // Input: int dur (arg 1), string msg (arg 2)
    // Cxt: actor
    // Output: none
    int Quickness(lua_State *lua)
    {
        int dur = luaL_checkinteger(lua, 1);
        const std::string msg = luaL_checkstring(lua, 2);
        shared_ptr<Knight> kt = GetActorAsKnight(lua);
        FlashMessage(kt, msg);
        SetPotion(Mediator::instance().getGVT(), kt, QUICKNESS, dur);
        return 0;
    }

    // Input: int dur (arg 1), string msg (arg 2)
    // Cxt: actor
    // Output: none
    int Strength(lua_State *lua)
    {
        int dur = luaL_checkinteger(lua, 1);
        const std::string msg = luaL_checkstring(lua, 2);
        shared_ptr<Knight> kt = GetActorAsKnight(lua);
        FlashMessage(kt, msg);
        SetPotion(Mediator::instance().getGVT(), kt, STRENGTH, dur);
        return 0;
    }

    // Input: int dur (arg 1), string msg (arg 2)
    // Cxt: actor
    // Output: none
    int Super(lua_State *lua)
    {
        int dur = luaL_checkinteger(lua, 1);
        const std::string msg = luaL_checkstring(lua, 2);
        shared_ptr<Knight> kt = GetActorAsKnight(lua);
        FlashMessage(kt, msg);
        SetPotion(Mediator::instance().getGVT(), kt, SUPER, dur);
        return 0;
    }

    // Input: int dur (arg 1), string msg (arg 2), string regen_type (arg 3)
    // Cxt: actor
    // Output: none
    int Regeneration(lua_State *lua)
    {
        int dur = luaL_checkinteger(lua, 1);
        const std::string msg = luaL_checkstring(lua, 2);
        const std::string regen_type = luaL_checkstring(lua, 3);

        PotionMagic pm = NO_POTION;
        if (regen_type == "fast") {
            pm = FAST_REGENERATION;
        } else if (regen_type == "slow") {
            pm = SLOW_REGENERATION;
        } else {
            throw LuaError("invalid regeneration type, must be \"fast\" or \"slow\"");
        }

        shared_ptr<Knight> kt = GetActorAsKnight(lua);
        FlashMessage(kt, msg);
        SetPotion(Mediator::instance().getGVT(), kt, pm, dur);
        return 0;
    }

    // Input: MonsterType* zom_type (arg 1)
    // Cxt: actor, originator
    // Output: none
    int ZombifyActor(lua_State *lua)
    {
        const MonsterType *zom_type = ReadLuaPtr<MonsterType>(lua, 1);
        if (!zom_type) return 0;

        shared_ptr<Creature> actor = GetActorAsCreature(lua);
        ZombifyCreature(actor, *zom_type, GetOriginatorFromCxt(lua));
        return 0;
    }

    // Input: MonsterType* zom_type (arg 1)
    // Cxt: victim, originator
    // Output: boolean (true if zombified, false otherwise)
    int ZombifyTarget(lua_State *lua)
    {
        const MonsterType *zom_type = ReadLuaPtr<MonsterType>(lua, 1);
        if (!zom_type) {
            lua_pushboolean(lua, false);
            return 1;
        }

        lua_getglobal(lua, "cxt");
        lua_getfield(lua, -1, "victim");
        shared_ptr<Creature> victim = ReadLuaSharedPtr<Creature>(lua, -1);
        lua_pop(lua, 2);

        bool result = ZombifyCreature(victim, *zom_type, GetOriginatorFromCxt(lua));
        lua_pushboolean(lua, result);
        return 1;
    }
}

void AddLuaMagicFunctions(lua_State *lua)
{
    // all functions go in "kts" table (same as script actions)
    lua_rawgeti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    luaL_getsubtable(lua, -1, "kts");

    PushCFunction(lua, &Attractor);
    lua_setfield(lua, -2, "Attractor");

    PushCFunction(lua, &DispelMagic);
    lua_setfield(lua, -2, "DispelMagic");

    PushCFunction(lua, &Healing);
    lua_setfield(lua, -2, "Healing");

    PushCFunction(lua, &Invisibility);
    lua_setfield(lua, -2, "Invisibility");

    PushCFunction(lua, &Invulnerability);
    lua_setfield(lua, -2, "Invulnerability");

    PushCFunction(lua, &MagicMapping);
    lua_setfield(lua, -2, "MagicMapping");

    PushCFunction(lua, &OpenWays);
    lua_setfield(lua, -2, "OpenWays");

    PushCFunction(lua, &Paralyzation);
    lua_setfield(lua, -2, "Paralyzation");

    PushCFunction(lua, &Poison);
    lua_setfield(lua, -2, "Poison");

    PushCFunction(lua, &PoisonImmunity);
    lua_setfield(lua, -2, "PoisonImmunity");

    PushCFunction(lua, &Quickness);
    lua_setfield(lua, -2, "Quickness");

    PushCFunction(lua, &Regeneration);
    lua_setfield(lua, -2, "Regeneration");

    PushCFunction(lua, &RevealLocation);
    lua_setfield(lua, -2, "RevealLocation");

    PushCFunction(lua, &SenseItems);
    lua_setfield(lua, -2, "SenseItems");

    PushCFunction(lua, &SenseKnight);
    lua_setfield(lua, -2, "SenseKnight");

    PushCFunction(lua, &Strength);
    lua_setfield(lua, -2, "Strength");

    PushCFunction(lua, &Super);
    lua_setfield(lua, -2, "Super");

    PushCFunction(lua, &TeleportRandom);
    lua_setfield(lua, -2, "TeleportRandom");

    PushCFunction(lua, &WipeMap);
    lua_setfield(lua, -2, "WipeMap");

    PushCFunction(lua, &ZombifyActor);
    lua_setfield(lua, -2, "ZombifyActor");

    PushCFunction(lua, &ZombifyTarget);
    lua_setfield(lua, -2, "ZombifyTarget");

    // pop the "kts" and environment tables
    lua_pop(lua, 2);
}

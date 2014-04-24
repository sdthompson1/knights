/*
 * item_type.cpp
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
#include "creature.hpp"
#include "dungeon_map.hpp"
#include "item.hpp"
#include "item_type.hpp"
#include "lua_setup.hpp"
#include "rng.hpp"
#include "task_manager.hpp"
#include "tile.hpp"

using std::vector;

//
// ctors
//

ItemType::ItemType(lua_State *lua, int idx)
{
    ASSERT(lua);

    allow_strength = LuaGetBool(lua, idx, "allow_strength", true);
    ammo = LuaGetPtr<ItemType>(lua, idx, "ammo");
    backpack_graphic = LuaGetPtr<Graphic>(lua, idx, "backpack_graphic");
    backpack_overdraw = LuaGetPtr<Graphic>(lua, idx, "backpack_overdraw");
    backpack_slot = LuaGetInt(lua, idx, "backpack_slot");
    can_throw = LuaGetBool(lua, idx, "can_throw");
    control = LuaGetPtr<Control>(lua, idx, "control");
    fragile = LuaGetBool(lua, idx, "fragile");
    graphic = LuaGetPtr<Graphic>(lua, idx, "graphic");
    key = LuaGetInt(lua, idx, "key");
    max_stack = LuaGetInt(lua, idx, "max_stack", 1);
    melee_action.reset(lua, idx, "melee_action");
    melee_backswing_time = LuaGetInt(lua, idx, "melee_backswing_time");
    melee_damage = LuaGetRandomInt(lua, idx, "melee_damage");
    melee_downswing_time = LuaGetInt(lua, idx, "melee_downswing_time");
    melee_stun_time = LuaGetRandomInt(lua, idx, "melee_stun_time");
    melee_tile_damage = LuaGetRandomInt(lua, idx, "melee_tile_damage");
    missile_access_chance = LuaGetFloat(lua, idx, "missile_access_chance");
    missile_anim = LuaGetPtr<Anim>(lua, idx, "missile_anim");
    missile_backswing_time = LuaGetInt(lua, idx, "missile_backswing_time");
    missile_damage = LuaGetRandomInt(lua, idx, "missile_damage");
    missile_downswing_time = LuaGetInt(lua, idx, "missile_downswing_time");
    missile_hit_multiplier = LuaGetInt(lua, idx, "missile_hit_multiplier", 1);
    missile_range = LuaGetInt(lua, idx, "missile_range");
    missile_speed = LuaGetInt(lua, idx, "missile_speed");
    missile_stun_time = LuaGetRandomInt(lua, idx, "missile_stun_time");
    on_drop.reset(lua, idx, "on_drop");
    on_hit.reset(lua, idx, "on_hit");
    on_pick_up.reset(lua, idx, "on_pick_up");
    on_walk_over.reset(lua, idx, "on_walk_over");
    open_traps = LuaGetBool(lua, idx, "open_traps");
    overlay = LuaGetPtr<Overlay>(lua, idx, "overlay");
    parry_chance = LuaGetFloat(lua, idx, "parry_chance");
    reload_action.reset(lua, idx, "reload_action");
    reload_action_time = LuaGetInt(lua, idx, "reload_action_time", 250);
    reload_time = LuaGetInt(lua, idx, "reload_time");
    stack_graphic = LuaGetPtr<Graphic>(lua, idx, "stack_graphic");
    if (!stack_graphic) stack_graphic = graphic;
    tutorial_key = LuaGetInt(lua, idx, "tutorial");
    is = LuaGetItemSize(lua, idx, "type", IS_NOPICKUP);

    lua_getfield(lua, idx, "critical");  // [crit]
    if (lua_toboolean(lua, -1) != 0) {
        const char *p = lua_tostring(lua, -1);
        if (p) critical_msg = p;
        is_critical = true;
    } else {
        is_critical = false;
    }
    lua_pop(lua, 1); // []

    loaded = 0;  // set separately via 'setLoaded', see also KnightsConfigImpl::addLuaItemType

    lua_pushvalue(lua, idx);
    table_ref.reset(lua);
}

ItemType::ItemType(const Graphic *gfx, ItemSize item_size,
                   const LuaFunc &pickup_action, const LuaFunc &drop_action)
    : on_pick_up(pickup_action),
      on_drop(drop_action)
{
    allow_strength = false;
    ammo = 0;
    backpack_graphic = 0;
    backpack_overdraw = 0;
    backpack_slot = 0;
    can_throw = false;
    control = 0;
    fragile = false;
    graphic = gfx;
    is_critical = false;
    key = 0;
    max_stack = 1;
    melee_backswing_time = 0;
    melee_downswing_time = 0;
    missile_access_chance = 0;
    missile_anim = 0;
    missile_backswing_time = 0;
    missile_downswing_time = 0;
    missile_hit_multiplier = 0;
    missile_range = 0;
    missile_speed = 0;
    open_traps = false;
    overlay = 0;
    parry_chance = 0;
    reload_action_time = 0;
    reload_time = 0;
    stack_graphic = graphic;
    tutorial_key = 0;
    is = item_size;
    loaded = 0;
}

void ItemType::newIndex(lua_State *lua)
{
    // [ud key val]
    if (!lua_isstring(lua, 2)) return;
    const std::string k = lua_tostring(lua, 2);

    // slow one-by-one string comparisons, seems easiest way though, and
    // this code would only be run during setup.
    if (k == "allow_strength") {
        allow_strength = lua_toboolean(lua, 3) != 0;

    } else if (k == "ammo") {
        ammo = ReadLuaPtr<ItemType>(lua, 3);

    } else if (k == "backpack_graphic") {
        backpack_graphic = ReadLuaPtr<Graphic>(lua, 3);

    } else if (k == "backpack_overdraw") {
        backpack_overdraw = ReadLuaPtr<Graphic>(lua, 3);

    } else if (k == "backpack_slot") {
        backpack_slot = lua_tointeger(lua, 3);

    } else if (k == "can_throw") {
        can_throw = lua_toboolean(lua, 3) != 0;

    } else if (k == "control") {
        control = ReadLuaPtr<Control>(lua, 3);

    } else if (k == "fragile") {
        fragile = lua_toboolean(lua, 3) != 0;

    } else if (k == "graphic") {
        graphic = ReadLuaPtr<Graphic>(lua, 3);
        
    } else if (k == "key") {
        key = lua_tointeger(lua, 3);

    } else if (k == "max_stack") {
        max_stack = lua_tointeger(lua, 3);

    } else if (k == "melee_action") {
        lua_pushvalue(lua, 3);
        melee_action = LuaFunc(lua); // pops

    } else if (k == "melee_backswing_time") {
        melee_backswing_time = lua_tointeger(lua, 3);

    } else if (k == "melee_damage") {
        lua_pushvalue(lua, 3);
        melee_damage = LuaPopRandomInt(lua, "melee_damage");

    } else if (k == "melee_downswing_time") {
        melee_downswing_time = lua_tointeger(lua, 3);

    } else if (k == "melee_stun_time") {
        lua_pushvalue(lua, 3);
        melee_stun_time = LuaPopRandomInt(lua, "melee_stun_time");

    } else if (k == "melee_tile_damage") {
        lua_pushvalue(lua, 3);
        melee_tile_damage = LuaPopRandomInt(lua, "melee_tile_damage");

    } else if (k == "missile_access_chance") {
        missile_access_chance = float(lua_tonumber(lua, 3));

    } else if (k == "missile_anim") {
        missile_anim = ReadLuaPtr<Anim>(lua, 3);

    } else if (k == "missile_backswing_time") {
        missile_backswing_time = lua_tointeger(lua, 3);

    } else if (k == "missile_damage") {
        lua_pushvalue(lua, 3);
        missile_damage = LuaPopRandomInt(lua, "missile_damage");

    } else if (k == "missile_downswing_time") {
        missile_downswing_time = lua_tointeger(lua, 3);

    } else if (k == "missile_hit_multiplier") {
        missile_hit_multiplier = lua_tointeger(lua, 3);

    } else if (k == "missile_range") {
        missile_range = lua_tointeger(lua, 3);

    } else if (k == "missile_speed") {
        missile_speed = lua_tointeger(lua, 3);

    } else if (k == "missile_stun_time") {
        lua_pushvalue(lua, 3);
        missile_stun_time = LuaPopRandomInt(lua, "missile_stun_time");

    } else if (k == "on_drop") {
        lua_pushvalue(lua, 3);
        on_drop = LuaFunc(lua); // pops

    } else if (k == "on_hit") {
        lua_pushvalue(lua, 3);
        on_hit = LuaFunc(lua);

    } else if (k == "on_pick_up") {
        lua_pushvalue(lua, 3);
        on_pick_up = LuaFunc(lua);

    } else if (k == "on_walk_over") {
        lua_pushvalue(lua, 3);
        on_walk_over = LuaFunc(lua);

    } else if (k == "open_traps") {
        open_traps = lua_toboolean(lua, 3) != 0;

    } else if (k == "overlay") {
        overlay = ReadLuaPtr<Overlay>(lua, 3);

    } else if (k == "parry_chance") {
        parry_chance = float(lua_tonumber(lua, 3));

    } else if (k == "reload_action") {
        lua_pushvalue(lua, 3);
        reload_action = LuaFunc(lua); // pops

    } else if (k == "reload_action_time") {
        reload_action_time = lua_tointeger(lua, 3);

    } else if (k == "reload_time") {
        luaL_error(lua, "Re-setting 'reload_time' after construction is not implemented");
        
    } else if (k == "stack_graphic") {
        stack_graphic = ReadLuaPtr<Graphic>(lua, 3);
        if (!stack_graphic) stack_graphic = graphic;

    } else if (k == "tutorial") {
        tutorial_key = lua_tointeger(lua, 3);

    } else if (k == "type") {
        lua_pushvalue(lua, 3);
        is = LuaPopItemSize(lua, IS_NOPICKUP);

    } else if (k == "critical") {
        if (lua_toboolean(lua, 3) != 0) {
            const char *p = lua_tostring(lua, 3);
            if (p) critical_msg = p;
            is_critical = true;
        } else {
            is_critical = false;
        }
    }
}        


//
// combat stuff
//

void ItemType::doCreatureImpact(int gvt, shared_ptr<Creature> attacker,
                                shared_ptr<Creature> target,
                                bool with_strength)
{
    if (!target) return;

    runMeleeAction(attacker, target);
    
    int stun_until = melee_stun_time.get() + gvt;
    int damage = melee_damage.get();
    if (with_strength) damage += g_rng.getInt(1,3); // add d2 dmg if you have strength
    target->damage(damage, attacker->getOriginator(), stun_until);
}

void ItemType::doTileImpact(shared_ptr<Creature> attacker, DungeonMap &dmap,
                            const MapCoord &mc, bool with_strength)
{
    // Impact against tiles. (This is only done against targettable tiles.)
    vector<shared_ptr<Tile> > tiles;
    dmap.getTiles(mc, tiles);
    for (vector<shared_ptr<Tile> >::iterator t = tiles.begin(); t != tiles.end(); ++t) {
        if ((*t)->targettable()) {
            // NOTE: It is important to do the melee action BEFORE the onHit action. This is
            // relied upon by Wands of Open Ways, to ensure that the traps are disarmed BEFORE
            // onHit runs.
            // If you need to switch the order then consider adding two separate melee actions
            // (eg pre_melee_action and post_melee_action).
            runMeleeAction(attacker, dmap, mc, *t);
            (*t)->onHit(dmap, mc, attacker, attacker->getOriginator());
            (*t)->damage(dmap, mc,
                         (with_strength && allow_strength) ? 9999 :
                         melee_tile_damage.get(),
                         attacker);
        }
    }

    // Impact against items.
    // (Note there is no melee_action corresponding to this case...)
    shared_ptr<Item> item = dmap.getItem(mc);
    if (item) {
        item->getType().onHit(dmap, mc, attacker);
    }
}


//
// actions
//

void ItemType::onPickUp(DungeonMap &dmap, const MapCoord &mc,
                        shared_ptr<Creature> actor)
{
    if (on_pick_up.hasValue()) {
        ActionData ad;
        ad.setActor(actor);
        ad.setItem(&dmap, mc, this);
        ad.setGenericPos(&dmap, mc);
        ad.setOriginator(actor->getOriginator());
        on_pick_up.execute(ad);
    }
}

void ItemType::onDrop(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor)
{
    if (on_drop.hasValue()) {
        ActionData ad;
        ad.setActor(actor);
        ad.setItem(&dmap, mc, this);
        ad.setGenericPos(&dmap, mc);
        ad.setOriginator(actor->getOriginator());
        on_drop.execute(ad);
    }
        
}

void ItemType::onWalkOver(DungeonMap &dmap, const MapCoord &mc,
                          shared_ptr<Creature> actor,
                          const Originator &item_owner)
{
    if (on_walk_over.hasValue() && actor && actor->getHeight() == H_WALKING) {
        ActionData ad;
        ad.setActor(actor);
        
        // the originator of an item-walk-over event (e.g. stepping on a beartrap)
        // is considered to be the person who put the beartrap there, NOT the player stepping on it.
        ad.setOriginator(item_owner);
        ad.setItem(&dmap, mc, this);
        ad.setGenericPos(&dmap, mc);
        on_walk_over.execute(ad);
    }
}

void ItemType::onHit(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor)
{
    if (on_hit.hasValue()) {
        ActionData ad;
        ad.setActor(actor);
        ad.setItem(&dmap, mc, this);
        ad.setGenericPos(&dmap, mc);
        ad.setOriginator(actor->getOriginator());
        on_hit.execute(ad);
    }
}

void ItemType::runMeleeAction(shared_ptr<Creature> actor, shared_ptr<Creature> victim)
{
    if (melee_action.hasValue()) {
        ActionData ad;
        ad.setActor(actor);
        ad.setOriginator(actor ? actor->getOriginator() : Originator(OT_None()));
        ad.setVictim(victim);
        ad.setItem(0, MapCoord(), this);
        // for generic_pos, use position of the actor rather than null.
        ad.setGenericPos(actor ? actor->getMap() : 0, 
                         actor ? actor->getPos() : MapCoord());
        melee_action.execute(ad);
    }
}

void ItemType::runMeleeAction(shared_ptr<Creature> actor,
                              DungeonMap &dmap, const MapCoord &mc,
                              shared_ptr<Tile> tile)
{
    if (melee_action.hasValue()) {
        ActionData ad;
        ad.setActor(actor);
        ad.setOriginator(actor ? actor->getOriginator() : Originator(OT_None()));
        ad.setTile(&dmap, mc, tile);
        ad.setItem(0, MapCoord(), this);
        ad.setGenericPos(actor ? actor->getMap() : 0, 
                         actor ? actor->getPos() : MapCoord());
        melee_action.execute(ad);
    }
}

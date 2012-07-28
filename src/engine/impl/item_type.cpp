/*
 * item_type.cpp
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

#include "action_data.hpp"
#include "creature.hpp"
#include "dungeon_map.hpp"
#include "item.hpp"
#include "item_type.hpp"
#include "lua_setup.hpp"
#include "rng.hpp"
#include "task_manager.hpp"
#include "tile.hpp"

//
// ctors
//

ItemType::ItemType(lua_State *lua, int idx, KnightsConfigImpl *kc)
{
    ASSERT(lua);

    allow_strength = LuaGetBool(lua, idx, "allow_strength", true);
    ammo = LuaGetPtr<const ItemType>(lua, idx, "ammo");
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
    melee_damage = LuaGetRandomInt(lua, idx, "melee_damage", kc);
    melee_downswing_time = LuaGetInt(lua, idx, "melee_downswing_time");
    melee_stun_time = LuaGetRandomInt(lua, idx, "melee_stun_time", kc);
    melee_tile_damage = LuaGetRandomInt(lua, idx, "melee_tile_damage", kc);
    missile_access_chance = LuaGetFloat(lua, idx, "missile_access_chance");
    missile_anim = LuaGetPtr<Anim>(lua, idx, "missile_anim");
    missile_backswing_time = LuaGetInt(lua, idx, "missile_backswing_time");
    missile_damage = LuaGetRandomInt(lua, idx, "missile_damage", kc);
    missile_downswing_time = LuaGetInt(lua, idx, "missile_downswing_time");
    missile_hit_multiplier = LuaGetInt(lua, idx, "missile_hit_multiplier", 1);
    missile_range = LuaGetInt(lua, idx, "missile_range");
    missile_speed = LuaGetInt(lua, idx, "missile_speed");
    missile_stun_time = LuaGetRandomInt(lua, idx, "missile_stun_time", kc);
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
    key = 0;
    max_stack = 1;
    melee_backswing_time = 0;
    melee_damage = 0;
    melee_downswing_time = 0;
    melee_stun_time = 0;
    melee_tile_damage = 0;
    missile_access_chance = 0;
    missile_anim = 0;
    missile_backswing_time = 0;
    missile_damage = 0;
    missile_downswing_time = 0;
    missile_hit_multiplier = 0;
    missile_range = 0;
    missile_speed = 0;
    missile_stun_time = 0;
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
    

//
// combat stuff
//

void ItemType::doCreatureImpact(int gvt, shared_ptr<Creature> attacker,
                                shared_ptr<Creature> target,
                                bool with_strength) const
{
    if (!target) return;

    runMeleeAction(attacker, target);
    
    int stun_until = (melee_stun_time ? melee_stun_time->get() : 0) + gvt;
    int damage = melee_damage ? melee_damage->get() : 0;
    if (with_strength) damage += g_rng.getInt(1,3); // add d2 dmg if you have strength
    target->damage(damage, attacker->getOriginator(), stun_until);
}

void ItemType::doTileImpact(shared_ptr<Creature> attacker, DungeonMap &dmap,
                            const MapCoord &mc, bool with_strength) const
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
                         (melee_tile_damage ? melee_tile_damage->get() : 0),
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
                        shared_ptr<Creature> actor) const
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

void ItemType::onDrop(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor) const
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
                          const Originator &item_owner) const
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

void ItemType::onHit(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor) const
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

void ItemType::runMeleeAction(shared_ptr<Creature> actor, shared_ptr<Creature> victim) const
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
                              shared_ptr<Tile> tile) const
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

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

#include "action.hpp"
#include "creature.hpp"
#include "dungeon_map.hpp"
#include "item.hpp"
#include "item_type.hpp"
#include "rng.hpp"
#include "task_manager.hpp"
#include "tile.hpp"

//
// ctors
//

ItemType::ItemType()
 : graphic(0), stack_graphic(0), backpack_graphic(0), backpack_overdraw(0), overlay(0), is(IS_BIG),
   max_stack(1), backpack_slot(0), fragile(false), allow_strength(true),
   melee_backswing_time(0), melee_damage(0), 
   melee_stun_time(0), melee_tile_damage(0), melee_action(0), parry_chance(0), 
   can_throw(false), missile_range(0), missile_speed(0), missile_access_chance(0),
   missile_hit_multiplier(1),
   missile_backswing_time(0), missile_downswing_time(0), missile_damage(0),
   missile_stun_time(0), missile_anim(0), 
   reload_time(0), ammo(0), loaded(0), reload_action(0),
   key(0), open_traps(false), 
   control(0), on_pick_up(0), on_drop(0), on_walk_over(0), on_hit(0), tutorial_key(0)
{ }

void ItemType::construct(const Graphic *g, const Graphic *sg,
    const Graphic *bg, const Graphic *bod,
    const Overlay *ov,
    ItemSize is_, int mxstk, int bslot, bool frag,
    int mbt, int mdt, const RandomInt *mdmg, const RandomInt *mst, const RandomInt *mtdmg,
    const Action *mac, float pch,
    bool cthrow, int mi_rng, int mi_spd, float mi_acc_ch, int mi_h_mul, int mi_bksw_time, int mi_dnsw_time,
    const RandomInt *mi_dmg, const RandomInt *mi_st,
    const Anim *mi_anm,
    int rt, const ItemType *amm, const Action *rld_ac, int rat,
    int k, bool ot, const Control *ctrl,
    const Action *puac, const Action *dac, const Action *woac, const Action *hac,
    bool astr, int t_key, const std::string &name_)
{
    graphic = g; stack_graphic = sg; backpack_graphic = bg; backpack_overdraw = bod;
    overlay = ov; is = is_; max_stack = mxstk;
    backpack_slot = bslot;
    fragile = frag; allow_strength = astr;
    melee_backswing_time = mbt; melee_downswing_time = mdt; melee_damage = mdmg;
    melee_stun_time = mst; melee_tile_damage = mtdmg; melee_action = mac; parry_chance = pch;
    can_throw = cthrow; missile_range = mi_rng; missile_speed = mi_spd;
    missile_access_chance = mi_acc_ch; missile_hit_multiplier = mi_h_mul;
    missile_backswing_time = mi_bksw_time; missile_downswing_time = mi_dnsw_time;
    missile_damage = mi_dmg;
    missile_stun_time = mi_st; missile_anim = mi_anm;
    reload_time = rt; ammo = amm; reload_action = rld_ac; reload_action_time = rat; 
    key = k; open_traps = ot; control = ctrl;
    on_pick_up = puac; on_drop = dac; on_walk_over = woac; on_hit = hac;
    tutorial_key = t_key;
    name = name_;
}

void ItemType::construct(const Graphic *g, ItemSize sz)
{
    graphic = g;
    is = sz;
    // (the rest are as set from the ctor.)
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
    if (on_pick_up) {
        ActionData ad;
        ad.setActor(actor);
        ad.setItem(&dmap, mc, this);
        ad.setGenericPos(&dmap, mc);
        ad.setOriginator(actor->getOriginator());
        on_pick_up->execute(ad);
    }
}

void ItemType::onDrop(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor) const
{
    if (on_drop) {
        ActionData ad;
        ad.setActor(actor);
        ad.setItem(&dmap, mc, this);
        ad.setGenericPos(&dmap, mc);
        ad.setOriginator(actor->getOriginator());
        on_drop->execute(ad);
    }
        
}

void ItemType::onWalkOver(DungeonMap &dmap, const MapCoord &mc,
                          shared_ptr<Creature> actor,
                          const Originator &item_owner) const
{
    if (on_walk_over && actor && actor->getHeight() == H_WALKING) {
        ActionData ad;
        ad.setActor(actor);
        
        // the originator of an item-walk-over event (e.g. stepping on a beartrap)
        // is considered to be the person who put the beartrap there, NOT the player stepping on it.
        ad.setOriginator(item_owner);
        ad.setItem(&dmap, mc, this);
        ad.setGenericPos(&dmap, mc);
        on_walk_over->execute(ad);
    }
}

void ItemType::onHit(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor) const
{
    if (on_hit) {
        ActionData ad;
        ad.setActor(actor);
        ad.setItem(&dmap, mc, this);
        ad.setGenericPos(&dmap, mc);
        ad.setOriginator(actor->getOriginator());
        on_hit->execute(ad);
    }
}

void ItemType::runMeleeAction(shared_ptr<Creature> actor, shared_ptr<Creature> victim) const
{
    if (melee_action) {
        ActionData ad;
        ad.setActor(actor);
        ad.setOriginator(actor ? actor->getOriginator() : Originator(OT_None()));
        ad.setVictim(victim);
        ad.setItem(0, MapCoord(), this);
        // for generic_pos, use position of the actor rather than null.
        ad.setGenericPos(actor ? actor->getMap() : 0, 
                         actor ? actor->getPos() : MapCoord());
        melee_action->execute(ad);
    }
}

void ItemType::runMeleeAction(shared_ptr<Creature> actor,
                              DungeonMap &dmap, const MapCoord &mc,
                              shared_ptr<Tile> tile) const
{
    if (melee_action) {
        ActionData ad;
        ad.setActor(actor);
        ad.setOriginator(actor ? actor->getOriginator() : Originator(OT_None()));
        ad.setTile(&dmap, mc, tile);
        ad.setItem(0, MapCoord(), this);
        ad.setGenericPos(actor ? actor->getMap() : 0, 
                         actor ? actor->getPos() : MapCoord());
        melee_action->execute(ad);
    }
}

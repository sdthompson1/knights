/*
 * item_type.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
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

/*
 * ItemType is a 'class' of items. Think of it as a master copy of a
 * given sort of item (a potion, scroll, key, hammer, or whatever).
 *
 * Really, this class ought to be split up into several different
 * classes representing different capabilities of an item. An item
 * could then have a list of capabilities instead of just a single
 * "type". This would avoid having loads of unrelated stuff all in the
 * one ItemType class (eg the "construct" function below is pretty
 * awful).
 * 
 */

#ifndef ITEM_TYPE_HPP
#define ITEM_TYPE_HPP

#include "random_int.hpp"
using namespace KConfig;

#include <boost/shared_ptr.hpp>
using namespace boost;

#include <string>

class Action;
class Anim;
class Control;
class Creature;
class DungeonMap;
class Graphic;
class Item;
class MapCoord;
class Originator;
class Overlay;
class Tile;

enum ItemSize {
    IS_BIG,   // "held"
    IS_SMALL, // "backpack"
    IS_MAGIC, // consumed when picked up
    IS_NOPICKUP,  // cannot be picked up
    NUM_ITEM_SIZES
};

class ItemType {
public:
    ItemType();
    void construct(const Graphic *gfx, const Graphic *stack_gfx,
                   const Graphic *backpack_gfx, const Graphic *backpack_overdraw,
                   const Overlay *ovrly, ItemSize is, int max_stack, int backpack_slot,
                   bool fragile,
                   int melee_back_time, int melee_down_time, const RandomInt *melee_dmg, 
                   const RandomInt *melee_stun_time, const RandomInt *melee_tile_damage,
                   const Action *melee_action, int parry_chance,
                   bool can_throw, int mssl_rng, int mssl_spd, int mssl_acc_chance,
                   int mssl_hit_mult,
                   int mssl_bksw_time, int mssl_dswg_time, const RandomInt *mssl_dmg,
                   const RandomInt *mssl_stun_time, const Anim * mssl_anim,
                   int reload_time, const ItemType *ammo,
                   const Action *reload_action, int reload_action_time,
                   int key, bool opens_traps, 
                   const Control *ctrl, const Action *pickup, const Action *drop, 
                   const Action *walk_over, const Action *on_hit, bool allw_str,
                   int t_key,
                   const std::string &name_);
    void construct(const Graphic *g, ItemSize is);

    void setAmmoType(ItemType *a) { ammo = a; }

    // Name
    // Currently this is either empty, or of the form "A gem" or "The book".
    const std::string & getName() const { return name; }
        
    // Graphics
    const Graphic * getSingleGraphic() const { return graphic; }
    const Graphic * getStackGraphic() const { return stack_graphic; }
    const Graphic * getBackpackGraphic() const { return backpack_graphic; }
    const Graphic * getBackpackOverdraw() const { return backpack_overdraw; }
    const Overlay * getOverlay() const { return overlay; }

    // Backpack slot. Note there are some "magic numbers" associated with this. See
    // DungeonView and LocalDungeonView. (Applies only to IS_SMALL items.)
    int getBackpackSlot() const { return backpack_slot; }
    
    // Size and stacking; fragility
    bool canPickUp() const { return (is != IS_NOPICKUP); }
    bool isBig() const { return is == IS_BIG; }
    bool isMagic() const { return is == IS_MAGIC; }
    int getMaxStack() const { return max_stack; }
    bool isFragile() const { return fragile; }

    // Melee properties
    bool canSwing() const { return melee_backswing_time > 0; }
    int getMeleeBackswingTime() const { return melee_backswing_time; }
    int getMeleeDownswingTime() const { return melee_downswing_time; }
    void doCreatureImpact(int gvt, shared_ptr<Creature> attacker,
                          shared_ptr<Creature> target, bool with_strength) const;
    void doTileImpact(shared_ptr<Creature> attacker, DungeonMap &dmap, const MapCoord &mc, 
                      bool with_strength) const;
    int getParryChance() const { return parry_chance; } // in percent
    
    // Missile properties
    bool canThrow() const { return can_throw; }
    int getMissileRange() const { return missile_range; }
    int getMissileSpeed() const { return missile_speed; }
    int getMissileAccessChance() const { return missile_access_chance; } // chance of passing through gates etc
    int getMissileHitMultiplier() const { return missile_hit_multiplier; }  // affects chance of hitting creatures
    int getMissileBackswingTime() const { return missile_backswing_time; }
    int getMissileDownswingTime() const { return missile_downswing_time; }
    const RandomInt * getMissileDamage() const { return missile_damage; }
    const RandomInt * getMissileStunTime() const { return missile_stun_time; }
    const Anim * getMissileAnim() const { return missile_anim; }
    
    // Crossbow properties
    // NB 'reload_time' is set -ve for a loaded crossbow, +ve for
    // an unloaded crossbow, and 0 for other items.
    bool canLoad() const { return reload_time > 0; } // used for crossbows
    bool canShoot() const { return reload_time < 0; } // used for crossbows
    int getReloadTime() const { return reload_time; }
    const Action * getReloadAction() const { return reload_action; }
    int getReloadActionTime() const { return reload_action_time; }
    const ItemType * getAmmo() const { return ammo; }
    const ItemType * getLoaded() const { return canLoad() ? loaded : 0; }
    const ItemType * getUnloaded() const { return canShoot() ? loaded : 0; }
    void setLoaded(const ItemType *i) { loaded = i; }
    void setUnloaded(const ItemType *i) { loaded = i; reload_time = -1; }
    
    // Keys and traps
    bool canOpenTraps() const { return open_traps; } // used for staffs

    // getKey:
    //  >0: item is a key, opening the given lock number.
    //   0: not a key.
    //  -1: item is a lockpick. (This is only used as a signal to DungeonGenerator::checkConnectivity. The actual
    //      ability to pick locks is implemented separately, as a "control".)
    int getKey() const { return key; }

    // Custom control (used for eg lockpicks, trap setting)
    const Control * getControl() const { return control; }

    
    // Actions

    // onPickUp -- runs on_pick_up
    void onPickUp(DungeonMap &, const MapCoord &, shared_ptr<Creature> actor) const;

    // onDrop -- runs on_drop
    void onDrop(DungeonMap &, const MapCoord &, shared_ptr<Creature> actor) const;

    // onWalkOver -- runs on_walk_over
    // (NB Does nothing if actor->getHeight() != H_WALKING.)
    void onWalkOver(DungeonMap &, const MapCoord &, shared_ptr<Creature> actor, const Originator &item_owner) const;

    // onHit -- runs on_hit
    void onHit(DungeonMap &, const MapCoord &, shared_ptr<Creature> actor) const;

    // runMeleeAction -- runs melee_action against either a creature or a tile
    void runMeleeAction(shared_ptr<Creature> actor, shared_ptr<Creature> victim) const;
    void runMeleeAction(shared_ptr<Creature> actor, DungeonMap &, const MapCoord &,
                        shared_ptr<Tile> target) const;

    int getTutorialKey() const { return tutorial_key; }
    
private:
    const Graphic * graphic;   // used while on the ground
    const Graphic * stack_graphic;     // on the ground in a stack of 2 or more
    const Graphic * backpack_graphic;  // used while in backpack (on UI display)
    const Graphic * backpack_overdraw;
    const Overlay * overlay;  // used while held by a creature

    ItemSize is;       // "type" in the config file
    int max_stack;
    int backpack_slot; 
    bool fragile;
    bool allow_strength;
    
    int melee_backswing_time;             // 0 if can't swing
    int melee_downswing_time;
    const RandomInt * melee_damage;
    const RandomInt * melee_stun_time;    // in ticks
    const RandomInt * melee_tile_damage;
    const Action * melee_action;
    int parry_chance;     // in percent

    bool can_throw;
    int missile_range;
    int missile_speed;
    int missile_access_chance;
    int missile_hit_multiplier;
    int missile_backswing_time;
    int missile_downswing_time;
    const RandomInt * missile_damage;
    const RandomInt * missile_stun_time;  // in ticks
    const Anim * missile_anim;
    
    int reload_time;                       // 0 if can't fire, -ve if loaded
    const ItemType * ammo;
    const ItemType * loaded;
    const Action * reload_action;
    int reload_action_time;
    
    int key;
    bool open_traps;

    const Control * control; 
    
    const Action * on_pick_up;
    const Action * on_drop;
    const Action * on_walk_over;
    const Action * on_hit;

    int tutorial_key;

    std::string name;
};

#endif

/*
 * item_type.hpp
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

/*
 * ItemType is a 'class' of items. (An Item is an ItemType plus a
 * number-of-items-in-the-pile.)
 *
 * Note that there are a lot of member functions/variables in
 * ItemType, indeed it contains the union of all possible properties
 * of all possible item types. This is not ideal, but it is what we
 * have. It could be mitigated by switching to a so-called "game
 * object component system", but that might introduce another kind of
 * complexity into the system...
 * 
 */

#ifndef ITEM_TYPE_HPP
#define ITEM_TYPE_HPP

#include "lua_func.hpp"
#include "lua_ref.hpp"

#include "random_int.hpp"
using namespace KConfig;

#include <boost/shared_ptr.hpp>
using namespace boost;

#include <string>

class Anim;
class Control;
class Creature;
class DungeonMap;
class Graphic;
class Item;
class KnightsConfigImpl;
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
    ItemType(lua_State *lua, int idx, KnightsConfigImpl *kc);

    // cut down ctor for stuff bags
    ItemType(const Graphic *gfx,
             ItemSize item_size,
             const LuaFunc &pickup_action,
             const LuaFunc &drop_action);
    
    void pushTable(lua_State *lua) const { table_ref.push(lua); }

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
    float getParryChance() const { return parry_chance; }
    
    // Missile properties
    bool canThrow() const { return can_throw; }
    int getMissileRange() const { return missile_range; }
    int getMissileSpeed() const { return missile_speed; }
    float getMissileAccessChance() const { return missile_access_chance; } // chance of passing through gates etc
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
    const LuaFunc & getReloadAction() const { return reload_action; }
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

    // Critical items
    // (A critical item will automatically be respawned if it is destroyed for any reason. See ItemCheckTask.)
    bool isCritical() const { return is_critical; }
    const std::string & getCriticalMsg() const { return critical_msg; }

    
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
    LuaFunc melee_action;
    float parry_chance;

    bool can_throw;
    int missile_range;
    int missile_speed;
    float missile_access_chance;
    int missile_hit_multiplier;
    int missile_backswing_time;
    int missile_downswing_time;
    const RandomInt * missile_damage;
    const RandomInt * missile_stun_time;  // in ticks
    const Anim * missile_anim;
    
    int reload_time;                       // 0 if can't fire, -ve if loaded
    const ItemType * ammo;
    const ItemType * loaded;
    LuaFunc reload_action;
    int reload_action_time;
    
    int key;
    bool open_traps;

    bool is_critical;
    std::string critical_msg;

    const Control * control; 
    
    LuaFunc on_pick_up;
    LuaFunc on_drop;
    LuaFunc on_walk_over;
    LuaFunc on_hit;

    int tutorial_key;

    LuaRef table_ref;
};

#endif

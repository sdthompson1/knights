/*
 * tile.hpp
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
 * Tiles are things that stay fixed in one square. Eg walls, chests,
 * tables, pentagrams, switches, pits, homes.
 *
 */

#ifndef TILE_HPP
#define TILE_HPP

#include "lua_ref.hpp"
#include "map_support.hpp"
#include "mini_map_colour.hpp"

#include "random_int.hpp"
using namespace KConfig;

#include "boost/enable_shared_from_this.hpp"
using namespace boost;

class Action;
class ColourChange;
class Control;
class Creature;
class DungeonMap;
class Graphic;
class Item;
class KnightsConfigImpl;

enum ActivateType {
    ACT_NORMAL,
    ACT_DISARM_TRAPS,  // e.g. Staff
    ACT_UNLOCK_ALL     // e.g. Wand of open ways
};

class Tile : public enable_shared_from_this<Tile> {
public:
    // NOTE: Constructors do not set the current hitpoints, only the initial hitpoints.
    // Current hitpoints are set by "setHitPoints", which is called by clone().

    // Construct from lua table at top of stack. Leaves the table on the stack.
    Tile(lua_State *lua, KnightsConfigImpl *kc);

    // Construct a "dummy" tile for switch-actions.
    Tile(const Action *walk_over, const Action *activate);
    
    // clone
    // Clones the tile.
    // WARNING: some tiles are copied and some are shared, so "clone" could either return a new copy
    // of *this or it could just return shared_from_this(), as appropriate.
    // If "force_copy" is set then ALWAYS make a true clone (i.e. a new copy).
    // --> see also "doClone" below -- NB subclasses must always override doClone!
    shared_ptr<Tile> clone(bool force_copy);

    // make a clone with a different graphic to the original.
    shared_ptr<Tile> cloneWithNewGraphic(const Graphic *new_graphic);

    // If a tile was cloned this returns a pointer back to the original tile.
    // Otherwise it returns this.
    // Used for things like monster_ai_avoid tiles, monster generator tiles etc.
    // See also #139
    shared_ptr<Tile> getOriginalTile() const;
    
    // Graphic, visibility, etc
    const Graphic * getGraphic() const { return graphic; }
    shared_ptr<const ColourChange> getColourChange() const { return cc; }
    virtual MiniMapColour getColour() const;
    int getDepth() const { return depth; }

    // Access
    MapAccess getAccess(MapHeight ht) const
        { return ht > H_MISSILES? access[H_MISSILES] : access[ht]; }
    bool itemsAllowed() const { return items_mode == ALLOWED; }
    bool destroyItems() const { return items_mode == DESTROY; }
    int getItemCategory() const { return item_category; }  // <0 means no category has been assigned.

    // Stairs
    bool isStair() const { return is_stair; }
    bool isStairOrTop() const { return is_stair || stair_top; }
    MapDirection stairsDownDirection() const { return stair_direction; }

    // Damage
    // -- if tile is destroyed then onDestroy is called, AFTER the tile has been removed from
    //    the map.
    // -- the default onDestroy will call the "on_destroy" action, AND ALSO will remove any
    //    fragile item present.
    // -- "destructible": can the target be destroyed. Used by zombies to see if they should
    //    attack a tile. (Destructible implies targettable, but not vice versa.)
    //    (Also used by the default mini map colouring, see Tile::getColour().)
    // -- "targettable": used to determine whether an item's melee_action should be called
    //    when attacking this tile. (e.g. does the wand flash the screen if you zap it against
    //    this tile.)
    // These fns are virtual because some tiles (doors) can be immune to damage
    // (if they are open).
    virtual void damage(DungeonMap &, const MapCoord &, int amount, shared_ptr<Creature> actor);
    virtual void onDestroy(DungeonMap &, const MapCoord &, shared_ptr<Creature> actor, const Originator &);
    bool destructible() const { return hit_points > 0 && targettable(); }
    virtual bool targettable() const;

    
    // Actions
    // canActivateHere: should return false for tiles that can't be activated while you are
    // standing directly on them (eg doors).
    virtual bool canActivateHere() const { return true; }

    // cannotActivateFrom: returns true & a direction if you cannot activate this tile
    // from a certain direction (ie treasure chests). Else returns false and dir is unchanged.
    virtual bool cannotActivateFrom(MapDirection& dir) const { return false; }
    
    // onActivate -- runs on_activate (and can be overridden by subclasses to do other
    // things e.g. opening/closing doors).
    // 'success' defaults to true, but is set false by things like locked doors (where
    // the player doesn't have the key).
    virtual void onActivate(DungeonMap &, const MapCoord &, shared_ptr<Creature> actor,
                            const Originator &originator, ActivateType act_type);

    // onWalkOver -- runs on_walk_over
    // (NB Does nothing if actor->getHeight() != H_WALKING.)
    void onWalkOver(DungeonMap &, const MapCoord &, shared_ptr<Creature> actor, const Originator &originator);

    // onApproach -- runs on_approach
    virtual void onApproach(DungeonMap &, const MapCoord &, shared_ptr<Creature> actor, const Originator &originator);

    // onWithdraw -- runs on_withdraw
    virtual void onWithdraw(DungeonMap &, const MapCoord &, shared_ptr<Creature> actor, const Originator &originator);

    // onHit -- runs on_hit
    virtual void onHit(DungeonMap &, const MapCoord &, shared_ptr<Creature> actor, const Originator &originator);


    // Custom control
    const Control * getControl(const MapCoord &pos) const;
    

    // placeItem: This is overridden by chests, barrels to place items
    // into 'storage' as opposed to directly onto the map.
    // canPlaceItem: this tile accepts 'placed' items.
    // itemPlacedAlready: we have accepted a 'placed' item; no more should be placed.
    // getPlacedItem: access the placed item directly...
    virtual bool canPlaceItem() const { return false; }
    virtual shared_ptr<Item> getPlacedItem() const { return shared_ptr<Item>(); }
    bool itemPlacedAlready() const { return getPlacedItem(); }
    virtual void placeItem(shared_ptr<Item>) { }

    // Override the standard connectivity checks
    // -1 = Cannot pass
    // 1 = Can pass
    // 0 = Use the standard checks (see dungeon_generator.cpp)
    int getConnectivityCheck() const { return connectivity_check; }

    // Tutorial Key (or 0 if unset)
    int getTutorialKey() const { return tutorial_key; }

    // Reflection/Rotation
    shared_ptr<Tile> getReflect();   // X reflection of this tile
    shared_ptr<Tile> getRotate();    // 90 degree clockwise rotation of this tile
    void setRotate(shared_ptr<Tile> t) { rotate = t; }
    void setReflect(shared_ptr<Tile> t) { reflect = t; }
    
protected:
    // Clone this tile (or return shared_from_this() if the same tile can be shared)
    // This must be overridden by subclasses.
    virtual shared_ptr<Tile> doClone(bool force_copy);

    // The following are used by the Tile subclasses.
    // They will automatically notify Mediator of any changes as necessary.
    void setGraphic(DungeonMap *, const MapCoord &, const Graphic *,
                    shared_ptr<const ColourChange>);
    void setItemsAllowed(DungeonMap *, const MapCoord &, bool allow, bool destroy);
    void setAccess(DungeonMap *, const MapCoord &, MapHeight height, MapAccess access, const Originator &originator);
    void setAccess(DungeonMap *, const MapCoord &, MapAccess access, const Originator &originator);  // set all heights at once
    void setAccessNoSweep(MapHeight height, MapAccess acc) { access[height] = acc; }

    void setItemsAllowedNoSweep(bool allow, bool destroy);
    void setGraphicNoNotify(const Graphic *g) { graphic = g; } // called by subclass ctors
    void setCCNoNotify(shared_ptr<ColourChange> cc_) { cc = cc_; }
    
private:
    // SetHitPoints -- this sets hit_points, using initial_hit_points.
    // This is called automatically by "clone", so new copies of tiles will automatically
    // be given an appropriate (possibly randomized) number of hit points.
    void setHitPoints();

    // assignment is not implemented currently
    void operator=(const Tile &);
    
private:
    LuaRef control_func_ref;
    
    const Graphic * graphic;
    shared_ptr<const ColourChange> cc;
    int depth;
    int item_category;
    bool is_stair, stair_top;
    enum { BLOCKED, ALLOWED, DESTROY } items_mode;
    MapAccess access[H_MISSILES+1];
    MapDirection stair_direction;

    int hit_points;
    const RandomInt * initial_hit_points;
    int connectivity_check;
    
    const Action * on_activate;
    const Action * on_walk_over;
    const Action * on_approach;
    const Action * on_withdraw;
    const Action * on_hit;
    const Action * on_destroy;

    const Control * control;
    
    int tutorial_key;

    boost::weak_ptr<Tile> reflect, rotate;
    boost::weak_ptr<Tile> original_tile;
};


//
// A useful predicate : BlocksItems
//

struct BlocksItems {
    bool operator()(const shared_ptr<Tile> &t) { return t && !t->itemsAllowed(); }
};



#endif  // C_TILE_HPP

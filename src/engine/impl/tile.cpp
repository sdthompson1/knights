/*
 * tile.cpp
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

#include "misc.hpp"

#include "action.hpp"
#include "creature.hpp"
#include "dungeon_map.hpp"
#include "item.hpp"
#include "item_type.hpp"
#include "knights_callbacks.hpp"
#include "lua_userdata.hpp"
#include "mediator.hpp"
#include "sweep.hpp"
#include "tile.hpp"

#include "lua.hpp"


//
// public functions
//

Tile::Tile()
    : graphic(0), depth(0), item_category(-1), is_stair(false), stair_top(false), items_mode(BLOCKED), 
      stair_direction(D_NORTH), hit_points(0), initial_hit_points(0), connectivity_check(0), on_activate(0), 
      on_walk_over(0), on_approach(0), on_withdraw(0), on_hit(0), on_destroy(0),
      control(0), tutorial_key(0)
{
    for (int i=0; i<=H_MISSILES; ++i) access[i] = A_CLEAR;
}

void Tile::construct(shared_ptr<lua_State> lua,
                     const Graphic *g, int d,
                     bool ita, bool dstry, int item_category_, 
                     const MapAccess acc[],
                     bool is, bool stop, MapDirection sdir, const RandomInt * ihp,
                     int cchk,
                     const Action *dto,
                     const Action *ac, 
                     const Action *wo, const Action *ap, const Action *wi, const Action *hit,
                     int t_key,
                     boost::shared_ptr<Tile> reflect_, boost::shared_ptr<Tile> rotate_)
{
    lua_state = lua;
    
    graphic = g; depth=d; item_category = item_category_;
    is_stair=is; stair_top=stop; stair_direction=sdir; 
    initial_hit_points=ihp;
    connectivity_check = cchk;
    on_destroy = dto;
    on_activate = ac; 
    on_walk_over = wo; on_approach = ap; on_withdraw = wi;
    on_hit = hit;
    for (int i=0; i<=H_MISSILES; ++i) { access[i] = acc[i]; }

    items_mode = BLOCKED;
    if (ita) items_mode = ALLOWED;
    if (dstry) items_mode = DESTROY;

    tutorial_key = t_key;

    reflect = reflect_ ? reflect_ : shared_from_this();
    rotate = rotate_ ? rotate_ : shared_from_this();
}

void Tile::construct(shared_ptr<lua_State> lua, const Graphic *g, int dpth)
{
    lua_state = lua;
    graphic = g; depth=dpth;
    items_mode = ALLOWED;
    // the rest take default values as set in the ctor.

    reflect = rotate = shared_from_this();
}

void Tile::construct(shared_ptr<lua_State> lua, const Action *walkover, const Action *activate)
{
    lua_state = lua;
    on_walk_over = walkover;
    on_activate = activate;
    items_mode = ALLOWED;

    reflect = rotate = shared_from_this();
}

Tile::~Tile()
{
    shared_ptr<lua_State> lua_lock(lua_state.lock());
    if (lua_lock) {
        // note: registry keys used are:
        //  this -- "user table" for the tile
        //  this+1 -- control func.
        lua_State * lua = lua_lock.get();
        lua_pushlightuserdata(lua, this);  // [key]
        lua_pushnil(lua);          // [key nil]
        lua_settable(lua, LUA_REGISTRYINDEX);  // []
        lua_pushlightuserdata(lua, (char*)(this)+1);  // [key2]
        lua_pushnil(lua);  // [key2 nil]
        lua_settable(lua, LUA_REGISTRYINDEX);
    }
}

shared_ptr<Tile> Tile::clone(bool force_copy)
{
    shared_ptr<lua_State> lua_lock(lua_state);
    lua_State *lua = lua_lock.get();
    
    lua_pushlightuserdata(lua, this);  // [this]
    lua_gettable(lua, LUA_REGISTRYINDEX);  // [oldtable]
    const bool has_lua_table = !lua_isnil(lua, -1);
    
    // if there is a lua user table for this tile then we must force a copy
    shared_ptr<Tile> new_tile = doClone(force_copy || has_lua_table);
    new_tile->setHitPoints();

    if (has_lua_table) {
        // clone the lua 'user table' for the tile
        lua_newtable(lua);      // [oldtable newtable]
        lua_pushnil(lua);       // [oldtable newtable nil]
        while (lua_next(lua, -3) != 0) {    // [oldtable newtable key value]
            lua_pushvalue(lua, -2);     // [oldtable newtable key value key]
            lua_pushvalue(lua, -2);     // [oldtable newtable key value key value]
            lua_settable(lua, -5);      // [oldtable newtable key value]
            lua_pop(lua, 1);            // [oldtable newtable key]
        }
        lua_pushlightuserdata(lua, new_tile.get());    // [oldtable newtable newtileptr]
        lua_pushvalue(lua, -2);    // [oldtable newtable newtileptr newtable]
        lua_settable(lua, LUA_REGISTRYINDEX);     // [oldtable newtable]
        lua_pop(lua, 2);           // []
    } else {
        // clean up the lua stack
        lua_pop(lua, 1);  // []
    }

    // copy the reference to the control func, if there is one
    lua_pushlightuserdata(lua, (char*)(this)+1);   // [oldkey2]
    lua_gettable(lua, LUA_REGISTRYINDEX);   // [func]
    if (lua_isnil(lua, -1)) {
        lua_pop(lua, 1);  // []
    } else {
        lua_pushlightuserdata(lua, (char*)(new_tile.get())+1);  // [func newkey2]
        lua_pushvalue(lua, -2);    // [func newkey2 func]
        lua_settable(lua, LUA_REGISTRYINDEX);   // [func]
        lua_pop(lua, 1);   // []
    }
    
    return new_tile;
}

shared_ptr<Tile> Tile::cloneWithNewGraphic(const Graphic *new_graphic)
{
    shared_ptr<Tile> new_tile = clone(true);  // we must force a copy because this is the only way we can sensibly change the graphic.
    new_tile->graphic = new_graphic;
    return new_tile;
}

void Tile::setHitPoints()
{
    if (initial_hit_points) {
        hit_points = initial_hit_points->get();
    }
}

void Tile::damage(DungeonMap &dmap, const MapCoord &mc, int amount, shared_ptr<Creature> actor)
{
    // NB: hit_points <= 0 means that the tile is indestructible.
    if (hit_points > 0) {
        hit_points -= amount;
        if (hit_points <= 0) {
            // Tile has been destroyed
            shared_ptr<Tile> self(shared_from_this());
            dmap.rmTile(mc, self, actor->getOriginator());
            self->onDestroy(dmap, mc, actor, actor->getOriginator());
        }
    }
}

void Tile::onDestroy(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor, const Originator &originator)
{
    // Run on_destroy event
    if (on_destroy) {
        ActionData ad;
        ad.setActor(actor, false);
        ad.setOriginator(originator);
        ad.setTile(&dmap, mc, shared_ptr<Tile>());
        ad.setLuaPos(mc);
        on_destroy->execute(ad);
    }
    
    // Destroy any fragile items present
    shared_ptr<Item> item = dmap.getItem(mc);
    if (item && item->getType().isFragile()) dmap.rmItem(mc);
}

bool Tile::targettable() const
{
    // Current definition of targettable is that it is not A_CLEAR at H_WALKING,
    // *OR* that it has an on_hit action.
    // This rules out "floor" tiles (except the special pentagram which has on_hit),
    // but pretty much everything else is included.
    return (getAccess(H_WALKING) != A_CLEAR || on_hit);
}

void Tile::onActivate(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor,
                      const Originator &originator, ActivateType act_type, bool success)
{
    // NB act_type isn't used here, but it is used in subclasses (in particular Lockable).
    if (on_activate) {
        ActionData ad;
        ad.setActor(actor, false);
        ad.setOriginator(originator);
        ad.setTile(&dmap, mc, shared_from_this());
        ad.setSuccess(success);
        ad.setLuaPos(mc);
        on_activate->execute(ad);
    }
}

void Tile::onWalkOver(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor, const Originator &originator)
{
    if (on_walk_over && actor && actor->getHeight() == H_WALKING) {
        ActionData ad;
        ad.setActor(actor, false);
        ad.setOriginator(originator);
        ad.setTile(&dmap, mc, shared_from_this());
        ad.setLuaPos(mc);
        on_walk_over->execute(ad);
    }
}

void Tile::onApproach(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor, const Originator &originator)
{
    if (on_approach) {
        ActionData ad;
        ad.setActor(actor, false);
        ad.setOriginator(originator);
        ad.setTile(&dmap, mc, shared_from_this());
        ad.setLuaPos(mc);
        on_approach->execute(ad);
    }
}

void Tile::onWithdraw(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor, const Originator &originator)
{
    if (on_withdraw) {
        ActionData ad;
        ad.setActor(actor, false);
        ad.setOriginator(originator);
        ad.setTile(&dmap, mc, shared_from_this());
        ad.setLuaPos(mc);
        on_withdraw->execute(ad);
    }
}

void Tile::onHit(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor, const Originator &originator)
{
    if (on_hit) {
        ActionData ad;
        ad.setActor(actor, false);
        ad.setOriginator(originator);
        ad.setTile(&dmap, mc, shared_from_this());
        ad.setLuaPos(mc);
        on_hit->execute(ad);
    }
}


//
// custom control
//

const Control * Tile::getControl(const MapCoord &pos) const
{
    if (control) {
        return control;
    } else {
        shared_ptr<lua_State> lua = lua_state.lock();
        if (!lua) return 0;

        lua_pushlightuserdata(lua.get(), (char*)(this)+1);   // [key2]
        lua_gettable(lua.get(), LUA_REGISTRYINDEX);   // [func]
        if (lua_isnil(lua.get(), -1)) {
            lua_pop(lua.get(), 1);  // []
            return 0;
        } else {
        
            PushMapCoord(lua.get(), pos);   // [func pos]
            const int result = lua_pcall(lua.get(), 1, 1, 0);  // [result]

            if (result != 0) {
                const std::string err_msg = lua_tostring(lua.get(), -1);
                lua_pop(lua.get(), 1);   // []
                Mediator::instance().getCallbacks().gameMsg(-1, err_msg);
                return 0;
            } else {
                const Control * ctrl = ReadLuaPtr<Control>(lua.get(), -1);
                lua_pop(lua.get(), 1);   // []
                return ctrl;
            }
        }
    }
}

void Tile::setControl(const Control *ctrl)
{
    control = ctrl;
}

void Tile::setControlFunc()
{
    shared_ptr<lua_State> lua = lua_state.lock();
    if (!lua) return;

    lua_pushlightuserdata(lua.get(), (char*)(this)+1);  // [func key2]
    lua_pushvalue(lua.get(), -2);   // [func key2 func]
    lua_settable(lua.get(), LUA_REGISTRYINDEX);  // [func]
    lua_pop(lua.get(), 1);  // []
}


//
// protected functions
//

shared_ptr<Tile> Tile::doClone(bool force_copy)
{
    ASSERT(typeid(*this)==typeid(Tile)); //doClone must be overridden in subclasses.

    // basic tiles are shared rather than cloned
    // however, if a tile has hit points, it must be copied. (and hit points must be rerolled, also.)
    if (initial_hit_points || force_copy) {
        return shared_ptr<Tile>(new Tile(*this));
    } else {
        return shared_from_this();
    }
}

void Tile::setGraphic(DungeonMap *dmap, const MapCoord &mc, const Graphic *g,
                      shared_ptr<const ColourChange> ccnew)
{
    graphic = g;
    cc = ccnew;
    if (dmap) {
        // Graphic-change needs to be propagated to clients
        Mediator::instance().onChangeTile(*dmap, mc, *this);
    }
}

void Tile::setItemsAllowed(DungeonMap *dmap, const MapCoord &mc, bool allow, bool destroy) 
{
    if (destroy) items_mode = DESTROY;
    else if (allow) items_mode = ALLOWED;
    else items_mode = BLOCKED;
    if (dmap && items_mode != ALLOWED) {
        // We don't need to tell Mediator about items-allowed changes.
        // However we do need to call SweepItems if items-allowed is now BLOCKED, or destroy
        // item if mode is DESTROY.
        if (items_mode == DESTROY) {
            dmap->rmItem(mc);
        } else {
            SweepItems(*dmap, mc);
        }
    }
}

void Tile::setAccess(DungeonMap *dmap, const MapCoord &mc, MapHeight height, MapAccess acc, const Originator &originator)
{
    access[height] = acc;
    if (dmap) {
        // We don't need to tell Mediator about access changes, but we do need 
        // to call SweepCreatures.
        SweepCreatures(*dmap, mc, true, height, originator);
    }
}

void Tile::setAccess(DungeonMap *dmap, const MapCoord &mc, MapAccess acc, const Originator &originator)
{
    for (int i=0; i<=H_MISSILES; ++i) {
        access[i] = acc;
    }
    SweepCreatures(*dmap, mc, false, H_MISSILES, originator);
}


MiniMapColour Tile::getColour() const
{
    // Current method of determining mini map colour is as follows: 
    // 
    // * colour is COL_WALL if access level != A_CLEAR at all available heights, AND
    //   tile is indestructible
    // * else, colour is COL_FLOOR
    //
    // This can be overridden by subclasses if required.

    if (destructible()) return COL_FLOOR;

    for (int h=0; h<=H_MISSILES; ++h) {
        if (getAccess(MapHeight(h)) == A_CLEAR) {
            return COL_FLOOR;
        }
    }

    return COL_WALL;
}

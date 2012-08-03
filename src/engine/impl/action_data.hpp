/*
 * action_data.hpp
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

#ifndef ACTION_DATA_HPP
#define ACTION_DATA_HPP

#include "map_support.hpp"
#include "originator.hpp"

#include "boost/shared_ptr.hpp"

class Creature;
class DungeonMap;
class ItemType;
class Tile;

struct lua_State;


//
// ActionData -- represents parameters passed to Lua in the "cxt"
// table.
//
// ACTOR is the creature who is performing the action. For most
// actions it is also the creature that the action is to be applied
// to. For example, A_PitKill kills the Actor (by making him fall down
// a pit), A_TeleportRandom teleports the Actor, etc. An exception is the
// melee actions, which are usually applied to the Victim instead (see
// below).
//
// VICTIM is only used with melee_action, and is set to the *target*
// creature. (Actor is set to the *attacking* creature for these
// actions.)
//
// ITEM or TILE will be set if the action came from a specific item or
// tile.
// 
// For ITEMs, a null dmap & mc for an item means "item not in map" ie
// someone is carrying it. A null mc but non-null dm means the item is
// going into the "displaced items" list. Null item itself means that
// no Item was given.
//
// For TILEs, null dm & mc means that no tile was given. Null Tile ptr
// means "no specific tile" (this is used by on_impact when a square
// is being hit, or by on_destroy when the destroyed tile has already
// been removed from the map).
//
// The GENERIC POS (lua cxt field "pos") will be equal to one of the item, 
// tile, actor or victim positions
// depending on what is most appropriate. For example, if the action being run
// is an "on_open_or_close" for a tile, then it will be the tile pos. If it is 
// "on_hit" for some item, it will be the item pos. Etc.
//
// The FLAG parameter is a hack, used only by the pretrapped chests
// code. (It is not represented in Lua "cxt" and is hard-wired to false in
// Lua contexts.)
//
// ORIGINATOR is the player who set this action in motion. It's used
// for attributing kills (in certain actions). For example, if player
// 1 presses a switch which opens a pit beneath player 2, then
// A_PitKill is called with Actor set to player 2's knight and
// Originator set to player 1. The kill is then attributed to player
// 1.
//
// Also, when the item on_walk_over event is called, Originator is
// always set to the player who placed the item into the dungeon
// (rather than the player who, for example, teleported the knight
// onto the item). This is important for bear traps.
//

class ActionData {
public:
    ActionData()
        : flag(false), item(0), item_dmap(0), tile_dmap(0), 
          generic_dmap(0), originator(OT_None()) { }

    // construct from global var "cxt" in lua state
    // (See also fn GetOriginatorFromCxt, below)
    explicit ActionData(lua_State *lua);

    // push contents of *this as a table onto the lua stack
    void pushCxtTable(lua_State *lua) const;
    
    // modifier fns:
    void setActor(boost::shared_ptr<Creature> c) { actor = c; }
    void setVictim(boost::shared_ptr<Creature> c) { victim = c; }
    void setItem(DungeonMap *, const MapCoord &, ItemType *);
    void setTile(DungeonMap *, const MapCoord &, boost::shared_ptr<Tile>);
    void setGenericPos(DungeonMap *, const MapCoord &);
    void setFlag(bool f) { flag = f; }
    void setOriginator(const Originator &o) { originator = o; }

    // accessor fns:
    boost::shared_ptr<Creature> getActor() const { return actor; }
    boost::shared_ptr<Creature> getVictim() const { return victim; }
    void getItem(DungeonMap *&dm, MapCoord &mc, ItemType * &it) const
        { dm = item_dmap; mc = item_coord; it = item; }
    void getTile(DungeonMap *&dm, MapCoord &mc, boost::shared_ptr<Tile> &t) const
        { dm = tile_dmap; mc = tile_coord; t = tile; }
    void getGenericPos(DungeonMap *&dm, MapCoord &mc) const
        { dm = generic_dmap; mc = generic_coord; }
    bool getFlag() const { return flag; }
    const Originator & getOriginator() const { return originator; }
    
private:
    boost::shared_ptr<Creature> actor, victim;
    bool flag;
    ItemType * item;
    boost::shared_ptr<Tile> tile;
    DungeonMap *item_dmap, *tile_dmap, *generic_dmap;
    MapCoord item_coord, tile_coord, generic_coord;
    Originator originator;
};

// shortcuts to access certain fields of lua cxt table directly:
Originator GetOriginatorFromCxt(lua_State *lua);

#endif

/*
 * action.hpp
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

#ifndef ACTION_HPP
#define ACTION_HPP

#include "kconfig_fwd.hpp"
#include "map_support.hpp"
#include "originator.hpp"

#include "boost/shared_ptr.hpp"
#include "boost/weak_ptr.hpp"
using namespace boost;

#include <map>
#include <string>
#include <vector>
using namespace std;

class Creature;
class DungeonMap;
class Item;
class ItemType;
class Player;
class Sound;
class TaskManager;
class Tile;

struct lua_State;


//
// ActionData -- parameters passed to an Action (when it is called).
//
// ACTOR is the creature who is performing the action. For most
// actions it is also the creature that the action is to be applied
// to. For example, A_PitKill kills the Actor (by making him fall down
// a pit), A_Teleport teleports the Actor, etc. An exception is the
// melee actions, which are usually applied to the Victim instead (see
// below).
//
// DIRECT is true if the action was called directly from a control; it
// is false if the action was called by an onSomething routine or some
// such. (This seems only to be used within A_Activate at the moment.)
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
// The FLAG parameter is a hack, used only by the pretrapped chests
// code.
//
// SUCCESS will be set if this action was invoked as a result of
// something "successful", where "successful" is defined as follows
// (currently):
//  - when opening doors, "success" means the door was opened, and
//    "failure" means that the door was locked and could not be opened.
//  - for the second and subsequent actions in a ListAction, the "success" flag
//    reflects whether possible() was true for the previous action in the list.
//    (i.e. "possible" is used as a proxy for "success" of the previous action,
//    in this case.)
//
// PLAYER is the player who set this action in motion. It's used for
// attributing kills (in certain actions). For example, if player 1
// presses a switch which opens a pit beneath player 2, then A_PitKill
// is called with Actor set to player 2's knight and Player set to
// player 1. The kill is then attributed to player 1.
//
// Also, when the item on_walk_over event is called, Player is always
// set to the player who placed the item into the dungeon (rather than
// the player who, for example, teleported the knight onto the item).
// This is important for bear traps.
//

class ActionData {
public:
    ActionData()
        : direct(false), flag(false), success(true), item(0), item_dmap(0), tile_dmap(0), originator(OT_None()) { }

    void setActor(shared_ptr<Creature> c, bool dir) { actor = c; direct = dir; }
    void setVictim(shared_ptr<Creature> c) { victim = c; }
    void setItem(DungeonMap *, const MapCoord &, const ItemType *);
    void setTile(DungeonMap *, const MapCoord &, shared_ptr<Tile>);
    void setFlag(bool f) { flag = f; }
    void setSuccess(bool f) { success = f; }
    void setOriginator(const Originator &o) { originator = o; }

    // this is used for the lua "pos" field
    void setLuaPos(const MapCoord &mc) { lua_pos = mc; }
    
    shared_ptr<Creature> getActor() const { return actor; }
    shared_ptr<Creature> getVictim() const { return victim; }
    bool isDirect() const { return direct; } // see above
    void getItem(DungeonMap *&dm, MapCoord &mc, const ItemType * &it) const
        { dm = item_dmap; mc = item_coord; it = item; }
    void getTile(DungeonMap *&dm, MapCoord &mc, shared_ptr<Tile> &t) const
        { dm = tile_dmap; mc = tile_coord; t = tile; }
    bool getFlag() const { return flag; }
    bool getSuccess() const { return success; }
    const Originator & getOriginator() const { return originator; }

    pair<DungeonMap*, MapCoord> getPos() const;  // lookup pos from creature, item and tile (in that order).

    const MapCoord & getLuaPos() const { return lua_pos; }
    
private:
    shared_ptr<Creature> actor, victim;
    bool direct, flag, success;
    const ItemType * item;
    shared_ptr<Tile> tile;
    DungeonMap *item_dmap, *tile_dmap;
    MapCoord item_coord, tile_coord;
    MapCoord lua_pos;
    Originator originator;
};


//
// Action -- Actions are essentially C++ routines that can be embedded
// into the config system. They represent everything from Knight
// actions to potion/scroll effects.
//

class Action {
public:
    virtual ~Action() { }
    
    // execute the action
    virtual void execute(const ActionData &) const = 0;


    // Actions used for "Controls" must also say whether it is possible
    // for the knight/creature to perform this action.

    // this is true if the action can be performed while stunned
    // (currently only used for A_Suicide)
    virtual bool canExecuteWhileStunned() const { return false; }

    // this is true if the action can be performed by a moving creature
    virtual bool canExecuteWhileMoving() const { return false; }

    // possible(): should return true if the action is possible
    // in the creature's current situation. Note however:
    //  - If the creature is stunned and !canExecuteWhileStunned(),
    //    return whether action *would be* possible, were the creature
    //    not stunned.
    //  - If the creature is moving and !canExecuteWhileMoving(),
    //    return whether the action *will become* possible, once the
    //    creature arrives at its destination.
    virtual bool possible(const ActionData &) const { return true; }
};


//
// RandomAction and ListAction
//

class RandomAction : public Action {
public:
    RandomAction() : total_weight(0) { }
    void reserve(int n) { data.reserve(n); }
    void add(const Action *ac, int wt);
    virtual void execute(const ActionData &) const;
private:
    vector<pair<const Action *, int> > data;
    int total_weight; 
};

class ListAction : public Action {
public:
    void reserve(int n) { data.reserve(n); }
    void add(const Action *ac) { if (ac) data.push_back(ac); }
    virtual bool possible(const ActionData &) const;
    virtual void execute(const ActionData &) const;
private:
    vector<const Action *> data;
};


//
// The following are used by KnightsConfig to create Actions:-
// ActionMaker -- pluggable factory to create Actions from "ActionPars" objects.
// ActionPars -- parameters passed to an Action when it is created.
//

class ActionPars {
public:
    virtual ~ActionPars() { }

    // require a certain number of parameters
    virtual void require(int n1, int n2=-1) = 0;

    // get the number of parameters
    virtual int getSize() = 0;

    // get each parameter by index (from 0 to npars-1)
    virtual const Action * getAction(int index) { error(); return 0; }
    virtual int getInt(int index) { error(); return 0; }
    virtual const ItemType * getItemType(int index) { error(); return 0; }
    virtual MapDirection getMapDirection(int index);  // default is to convert from a string
    virtual int getProbability(int index) { error(); return 0; }
    virtual const KConfig::RandomInt * getRandomInt(int index) { error(); return 0; }
    virtual const Sound * getSound(int index) { error(); return 0; }
    virtual string getString(int index) { error(); return string(); }
    virtual shared_ptr<Tile> getTile(int index) { error(); return shared_ptr<Tile>(); }

    // this is called if anything goes wrong.
    virtual void error() { }
};


class ActionMaker {
public:
    ActionMaker(const string &name);
    virtual ~ActionMaker() { }
    
    // it's assumed that the argument list (not the directive itself!) is at top of the KFile stack.
    // stack is to be left unchanged by createAction.
    // NOTE: ActionMakers should be re-entrant (if the ActionPars objects are different) - as they may
    // be called from different threads.
    static Action * createAction(const string &name, ActionPars &);

protected:
    virtual Action * make(ActionPars &) const = 0;
};


// macro to declare an ActionMaker inside a class
#define ACTION_MAKER(n) \
  struct Maker : ActionMaker { \
      virtual Action * make(ActionPars &) const; \
      static Maker register_me; \
      Maker() : ActionMaker(n) { } \
  }


// LuaAction
class LuaAction : public Action {
public:
    // ctor pops a Lua function off the top of the Lua stack and
    // stores it in the Lua registry
    explicit LuaAction(boost::shared_ptr<lua_State> lua);

    // dtor removes the function from the Lua registry
    ~LuaAction();

    // execute calls the stored lua function with one "cxt" argument
    // (representing the ActionData).
    virtual void execute(const ActionData &) const;

private:
    // prevent copying.
    LuaAction(const LuaAction &);
    void operator=(const LuaAction &) const;

    boost::weak_ptr<lua_State> lua_state;
};

#endif

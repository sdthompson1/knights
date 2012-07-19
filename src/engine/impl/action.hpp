/*
 * action.hpp
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

#ifndef ACTION_HPP
#define ACTION_HPP

#include "kconfig_fwd.hpp"
#include "lua_ref.hpp"
#include "map_support.hpp"
#include "originator.hpp"

#include "boost/shared_ptr.hpp"
#include "boost/thread/mutex.hpp"
using namespace boost;

#include <map>
#include <string>
#include <vector>
using namespace std;

class Creature;
class DungeonMap;
class Item;
class ItemType;
class MonsterType;
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
    // (See also fn GetOriginatorFromLua, below)
    explicit ActionData(lua_State *lua);

    // push contents of *this as a table onto the lua stack
    void pushCxtTable(lua_State *lua) const;
    
    // modifier fns:
    void setActor(shared_ptr<Creature> c) { actor = c; }
    void setVictim(shared_ptr<Creature> c) { victim = c; }
    void setItem(DungeonMap *, const MapCoord &, const ItemType *);
    void setTile(DungeonMap *, const MapCoord &, shared_ptr<Tile>);
    void setGenericPos(DungeonMap *, const MapCoord &);
    void setFlag(bool f) { flag = f; }
    void setOriginator(const Originator &o) { originator = o; }

    // accessor fns:
    shared_ptr<Creature> getActor() const { return actor; }
    shared_ptr<Creature> getVictim() const { return victim; }
    void getItem(DungeonMap *&dm, MapCoord &mc, const ItemType * &it) const
        { dm = item_dmap; mc = item_coord; it = item; }
    void getTile(DungeonMap *&dm, MapCoord &mc, shared_ptr<Tile> &t) const
        { dm = tile_dmap; mc = tile_coord; t = tile; }
    void getGenericPos(DungeonMap *&dm, MapCoord &mc) const
        { dm = generic_dmap; mc = generic_coord; }
    bool getFlag() const { return flag; }
    const Originator & getOriginator() const { return originator; }
    
private:
    shared_ptr<Creature> actor, victim;
    bool flag;
    const ItemType * item;
    shared_ptr<Tile> tile;
    DungeonMap *item_dmap, *tile_dmap, *generic_dmap;
    MapCoord item_coord, tile_coord, generic_coord;
    Originator originator;
};

// shortcuts to access certain fields of lua cxt table directly:
Originator GetOriginatorFromCxt(lua_State *lua);


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
    virtual float getProbability(int index) { error(); return 0; }
    virtual const KConfig::RandomInt * getRandomInt(int index) { error(); return 0; }
    virtual const Sound * getSound(int index) { error(); return 0; }
    virtual string getString(int index) { error(); return string(); }
    virtual shared_ptr<Tile> getTile(int index) { error(); return shared_ptr<Tile>(); }
    virtual const MonsterType * getMonsterType(int index) { error(); return 0; }

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

    // Create an Action given an ActionMaker and ActionPars.
    virtual Action * make(ActionPars &) const = 0;
};


// macro to declare an ActionMaker inside a class
#define ACTION_MAKER(n) \
  struct Maker : ActionMaker { \
      virtual Action * make(ActionPars &) const; \
      static Maker register_me; \
      Maker() : ActionMaker(n) { } \
  }


// this allows access to the global "makers map" (maps action name -> ActionMaker).
// make sure you lock g_makers_mutex while accessing it.
extern boost::mutex g_makers_mutex;
std::map<std::string, const ActionMaker *> & MakersMap();


// LuaAction
class LuaAction : public Action {
public:
    // ctor pops a Lua function off the top of the Lua stack and
    // stores it in the Lua registry
    // 
    // NOTE: It is assumed that 'lua' refers to the lua state that
    // will be stored in the Mediator. (It doesn't have to be in the
    // Mediator at construction time, but does by the time execute()
    // is called.)
    //
    explicit LuaAction(lua_State * lua_);

    // execute calls the stored lua function with one "cxt" argument
    // (representing the ActionData).
    virtual void execute(const ActionData &) const;

private:
    LuaRef function_ref;
};

#endif

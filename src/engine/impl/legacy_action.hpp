/*
 * legacy_action.hpp
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

#ifndef LEGACY_ACTION_HPP
#define LEGACY_ACTION_HPP

#include "kconfig_fwd.hpp" // needed for RandomInt

#include "map_support.hpp"

#include "boost/shared_ptr.hpp"
#include "boost/thread/mutex.hpp"

#include <map>
#include <string>

class ActionData;
class ItemType;
class MonsterType;
class Sound;
class Tile;


//
// LegacyAction is the old KConfig-based action system. It lives on as
// a way to create Lua functions for the "kts" table.
//
// Each LegacyAction generates two functions: the action itself (e.g.
// "kts.Throw"), and a "possible-function" (e.g. "kts.Can_Throw").
//

class LegacyAction {
public:
    virtual ~LegacyAction() { }
    virtual void execute(const ActionData &) const = 0;
    virtual bool possible(const ActionData &) const { return true; }
};


//
// ActionPars is an interface used by ActionMakers to read the
// parameters from Lua.
//

class ActionPars {
public:
    virtual ~ActionPars() { }

    // require a certain number of parameters
    virtual void require(int n1, int n2=-1) = 0;

    // get the number of parameters
    virtual int getSize() = 0;

    // get each parameter by index (from 0 to npars-1)
    virtual int getInt(int index) { error(); return 0; }
    virtual const ItemType * getItemType(int index) { error(); return 0; }
    virtual MapDirection getMapDirection(int index);  // default is to convert from a string
    virtual float getProbability(int index) { error(); return 0; }
    virtual const KConfig::RandomInt * getRandomInt(int index) { error(); return 0; }
    virtual const Sound * getSound(int index) { error(); return 0; }
    virtual std::string getString(int index) { error(); return std::string(); }
    virtual boost::shared_ptr<Tile> getTile(int index) { error(); return boost::shared_ptr<Tile>(); }
    virtual const MonsterType * getMonsterType(int index) { error(); return 0; }

    // this is called if anything goes wrong.
    virtual void error() { }
};


//
// ActionMaker is a factory class that can create LegacyActions given
// an action name and some ActionPars.
//

class ActionMaker {
public:
    explicit ActionMaker(const std::string &name);
    virtual ~ActionMaker() { }
    
    // NOTE: ActionMakers should be re-entrant (if the ActionPars objects are different) - as they may
    // be called from different threads.
    static LegacyAction * createAction(const std::string &name, ActionPars &);

    // Create a LegacyAction given an ActionMaker and ActionPars.
    virtual LegacyAction * make(ActionPars &) const = 0;
};


// macro to declare an ActionMaker inside a class
#define ACTION_MAKER(n) \
  struct Maker : ActionMaker { \
      virtual LegacyAction * make(ActionPars &) const; \
      static Maker register_me; \
      Maker() : ActionMaker(n) { } \
  }


// this allows access to the global "makers map" (maps action name -> ActionMaker).
// make sure you lock g_makers_mutex while accessing it.
extern boost::mutex g_makers_mutex;
std::map<std::string, const ActionMaker *> & MakersMap();


#endif

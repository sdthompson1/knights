/*
 * event_manager.hpp
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
 * Processes "script events". Usually called via Mediator.
 *
 */

#ifndef EVENT_MANAGER_HPP
#define EVENT_MANAGER_HPP

#include "map_support.hpp"

#include <map>
#include <string>
using namespace std;

class Action;
class ActionData;
class Creature;
class DungeonMap;
class Player;
class Tile;

class EventManager {
public:
    static void onAddCreature(Creature &);                          // on_walk_over [Player set from creature]
    static void onRmCreature(Creature &);                           // on_withdraw  [Player null]
    static void postRepositionCreature(Creature &);                 // on_walk_over [Player set from creature]
    static void onChangeEntityMotion(Creature &);                   // on_approach, on_withdraw [Player null]
    static void onAddTile(DungeonMap &, const MapCoord &, Tile &, const Originator &);  // on_walk_over
    static void onRmTile(DungeonMap &, const MapCoord &, Tile &, const Originator &);   // on_withdraw

    // A generic system for "event hooks"
    void setupHooks(const map<string, const Action*> &h) { hooks = h; }
    void runHook(const string &h, shared_ptr<Creature> cr) const;
    void runHook(const string &h, DungeonMap *dmap, const MapCoord &mc) const;
    
private:
    static void walkOverEvent(Creature &, const Originator &);
    static void withdrawEvent(Creature &);

    void doHook(const string &name, const ActionData &ad) const;

    map<string, const Action*> hooks;
};

#endif

/*
 * mediator.hpp
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
 * The Mediator holds references to the key objects in the system
 * (mainly the DungeonMap and the various "Manager" objects). These
 * objects can easily be accessed via the Mediator::instance()
 * function.
 *
 * Note that there are various "onEvent" routines that get called when
 * certain game events occur. (These are always called AFTER the
 * change is made.) These "onEvent" routines call the Managers (e.g.
 * the ViewManager and the EventManager) to tell them about the event.
 * Note that the Managers themselves cannot be accessed directly; they
 * are only accessed through the Mediator. This both reduces
 * dependencies and also increases abstraction (since clients don't
 * need to know the details of which managers get called in response
 * to which events).
 *
 * The GVT (global virtual time) can also be accessed from this class,
 * as can other things like the player list, the dungeon map, the task
 * manager, etc.
 * 
 * Mediator::instance() is programmed in a thread-local way, i.e.
 * there is one Mediator per server thread. This allows several games
 * to run concurrently, provided they run in separate threads.
 * 
 */

#ifndef MEDIATOR_HPP
#define MEDIATOR_HPP

#include "exception_base.hpp"
#include "map_support.hpp"

#include "boost/shared_ptr.hpp"
using namespace boost;

#include <set>
#include <vector>
using namespace std;

class ConfigMap;
class CoordTransform;
class Creature;
class DungeonMap;
class Entity;
class EventManager;
class GoreManager;
class Graphic;
class HomeManager;
class Item;
class ItemType;
class Knight;
class KnightsCallbacks;
class MapCoord;
class MonsterManager;
class MonsterType;
class Originator;
class Player;
class QuestHintManager;
class Sound;
class StuffManager;
class TaskManager;
class Tile;
class TutorialManager;
class ViewManager;

struct lua_State;


// Exception classes:-

class MediatorCreatedTwice : public ExceptionBase {
public:
    MediatorCreatedTwice() : ExceptionBase("Mediator created twice!") { }
};

class MediatorUnavailable : public ExceptionBase {
public:
    // NB if this is thrown it probably indicates that either (a) the mediator was accessed
    // before the game started, or (b) the mediator was accessed from outside of a game thread.
    MediatorUnavailable() : ExceptionBase("No Mediator instance available!") { }
};

class CallbacksUnavailable : public ExceptionBase {
public:
    CallbacksUnavailable() : ExceptionBase("KnightsCallbacks not found") { }
};


// The mediator class itself:-

class Mediator {
public: 
    // Access to instance. (This is like the Singleton pattern except
    // (a) there is one Mediator per thread rather than one across the
    // whole program, and (b) the instance has to be created
    // explicitly by createInstance.)
    static Mediator & instance();

    //
    // Tell us which KnightsCallbacks to use when events of interest
    // to the client occur.
    //
    // NOTE: KnightsEngine has to be quite careful to ensure that the
    // KnightsCallbacks pointer is always valid when running anything
    // that might call the Mediator (i.e. KnightsEngine::update).
    //
    void setCallbacks(KnightsCallbacks *cb) { callbacks = cb; }
    KnightsCallbacks & getCallbacks() const;   // throws if cb==0.

    //
    // Access general configuration settings.
    //
    int cfgInt(const std::string &key) const;
    float cfgProbability(const std::string &key) const;
    const std::string &cfgString(const std::string &key) const;

    //
    // Access the lua state
    //

    lua_State * getLuaState() { return lua_state.get(); }


    //
    // Interface to print msgs. Usually this forwards to
    // KnightsCallbacks::gameMsg, but during game setup it bypasses
    // this and calls the function in lua_game_setup.cpp instead (this
    // is to work around lack of a KnightsCallbacks object during game
    // startup).
    //

    void gameMsg(int player_num, const std::string &msg);
    
    
    //
    // "Event" routines:-
    //

    // Entities
    void onAddEntity(shared_ptr<Entity>);
    void onRmEntity(shared_ptr<Entity>);
    void onRepositionEntity(shared_ptr<Entity>);
    void onChangeEntityMotion(shared_ptr<Entity>, bool missile_mode);
    void onFlipEntityMotion(shared_ptr<Entity>);
    void onChangeEntityAnim(shared_ptr<Entity>);
    void onChangeEntityFacing(shared_ptr<Entity>);
    void onChangeEntityVisible(shared_ptr<Entity>);
    void onChangeSpeechBubble(shared_ptr<Knight>);
    
    // Icons
    void placeIcon(DungeonMap &, const MapCoord &, const Graphic *, int duration);

    // Items
    void onAddItem(const DungeonMap &, const MapCoord &, const Item &);
    void onRmItem(const DungeonMap &, const MapCoord &, const Item &);
    void onChangeItemGraphic(const DungeonMap &, const MapCoord &, const Item &);
    void onPickup(const Player &, const ItemType &);

    // Tiles
    void onAddTile(DungeonMap &, const MapCoord &, Tile &, const Originator &);
    void onRmTile(DungeonMap &, const MapCoord &, Tile &, const Originator &);
    void onChangeTile(const DungeonMap &, const MapCoord &, const Tile &);

    // Generic Event Hooks (Usually used for sound effects.)
    void runHook(const string &hook_name, shared_ptr<Creature> cr);
    void runHook(const string &hook_name, DungeonMap *dmap, const MapCoord &mc);
    
    // Gore effects.
    // (These just call into GoreManager at present.)
    // Creature is responsible for calling these at appropriate times.
    void placeBlood(DungeonMap &, const MapCoord &);
    void placeKnightCorpse(DungeonMap &, const MapCoord &, const Player &, bool blood);
    void placeMonsterCorpse(DungeonMap &, const MapCoord &, const MonsterType &);

    // Notification of monster death (passed to MonsterManager, to keep track of how many
    // monsters of each type there are)
    void onMonsterDeath(const MonsterType &);

    // Misc view effects
    void flashScreen(shared_ptr<Entity> ent, int delay);
    void playSound(DungeonMap &, const MapCoord &, const Sound &sound, int frequency, bool all);

    // Tutorial specific
    void onOpenLockable(const MapCoord &);

    // Knight death routine (used for tutorial among other things)
    void onKnightDeath(Player &player, const DungeonMap &dmap, const MapCoord &mc);
    
    // Home manager (wand of securing)
    bool isSecurableHome(const Player &pl, DungeonMap *dmap, const MapCoord &pos, MapDirection facing) const;
    bool secureHome(Player &pl, DungeonMap &dmap, const MapCoord &pos,
                    MapDirection facing, shared_ptr<Tile> secured_wall_tile);

    
    // End-of-Game handling
    void winGame(const Player & p); // makes p win the game (everyone on p's team wins)
    void timeLimitExpired();        // all players lose, except for deathmatch games where plyr(s) with most frags win(s)
    void eliminatePlayer(Player &);
    bool gameRunning() const { return game_running; }
    int getNumPlayersRemaining() const
        { return remaining_players.size(); }

    // Various accessor functions.
    shared_ptr<DungeonMap> getMap() const { return dmap; }
    shared_ptr<CoordTransform> getCoordTransform() const { return coord_transform; }
    const vector<Player*> &getPlayers() const { return players; }
    MonsterManager & getMonsterManager() const { return monster_manager; }
    QuestHintManager & getQuestHintManager() const { return quest_hint_manager; }
    StuffManager & getStuffManager() const { return stuff_manager; }
    HomeManager & getHomeManager() const { return home_manager; }
    TaskManager & getTaskManager() const { return task_manager; }
    int getGVT() const;
    
    
    //
    // Initialization stuff -- The following routines are called by
    // KnightsEngine (which is responsible for the creation and
    // destruction of the Mediator).
    //
    
    static void createInstance(EventManager &em, GoreManager &gm, HomeManager &hm,
                               MonsterManager &mm,
                               QuestHintManager &qm,
                               StuffManager &sm, TaskManager &tm,
                               ViewManager &vm, boost::shared_ptr<const ConfigMap> cmap,
                               boost::shared_ptr<lua_State> lua);
    static void destroyInstance();  // must be called before the game thread exits, otherwise will leak memory
    void setMap(shared_ptr<DungeonMap> dm, shared_ptr<CoordTransform> ct) { dmap = dm; coord_transform = ct; }
    void addPlayer(Player &);
    
    void setDeathmatchMode(bool d) { deathmatch_mode = d; }
    bool getDeathmatchMode() const { return deathmatch_mode; }

    //
    // Add tutorial manager -- can be called in middle of game if required
    //
    void setTutorialManager(boost::shared_ptr<TutorialManager> tm) { tutorial_manager = tm; }
    
private:
    // Note -- Mediator is not responsible for creating all of these
    // objects. That is done by the higher-ups (ie KnightsEngine).
    // Here, we mostly only have to keep references to things...
    Mediator(EventManager &em, GoreManager &gm, HomeManager &hm, MonsterManager &mm,
             QuestHintManager &qm,
             StuffManager &sm, TaskManager &tm, ViewManager &vm, boost::shared_ptr<const ConfigMap> cmap,
             boost::shared_ptr<lua_State> lua)
        : config_map(cmap), event_manager(em), gore_manager(gm), home_manager(hm), monster_manager(mm),
          quest_hint_manager(qm),
          stuff_manager(sm), task_manager(tm), view_manager(vm), game_running(true), callbacks(0),
          lua_state(lua), deathmatch_mode(false) { }
    void operator=(const Mediator &) const;  // not defined
    Mediator(const Mediator &);              // not defined


    void endGame(const std::vector<const Player*> &, std::string msg);
    
private:

    // Config Map
    boost::shared_ptr<const ConfigMap> config_map;
    
    // References to the Manager objects for this game.
    EventManager &event_manager;
    GoreManager &gore_manager;
    HomeManager &home_manager;
    MonsterManager &monster_manager;
    QuestHintManager &quest_hint_manager;
    StuffManager &stuff_manager;
    TaskManager &task_manager;
    ViewManager &view_manager;  

    // The tutorial manager is optional
    boost::shared_ptr<TutorialManager> tutorial_manager;
    
    // Other variables
    shared_ptr<DungeonMap> dmap;
    shared_ptr<CoordTransform> coord_transform;
    vector<Player*> players;
    set<const Player*> remaining_players;
    bool game_running;

    // KnightsCallbacks.
    KnightsCallbacks *callbacks;

    // Keep a pointer to the lua state for easy access from various places.
    boost::shared_ptr<lua_State> lua_state;

    // this affects the behaviour of timeLimitExpired()
    bool deathmatch_mode;
};

#endif

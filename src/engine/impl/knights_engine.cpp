/*
 * knights_engine.cpp
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

#include "misc.hpp"

#include "config_map.hpp"
#include "control.hpp"
#include "coord_transform.hpp"
#include "dummy_callbacks.hpp"
#include "dungeon_map.hpp"
#include "event_manager.hpp"
#include "gore_manager.hpp"
#include "home_manager.hpp"
#include "item_check_task.hpp"
#include "item_respawn_task.hpp"
#include "knights_config.hpp"
#include "knights_engine.hpp"
#include "lua_game_setup.hpp"
#include "magic_map.hpp"
#include "mediator.hpp"
#include "mini_map.hpp"
#include "monster_manager.hpp"
#include "my_exceptions.hpp"
#include "player.hpp"
#include "quest_hint_manager.hpp"
#include "round.hpp"
#include "stuff_bag.hpp"
#include "task_manager.hpp"
#include "time_limit_task.hpp"
#include "view_manager.hpp"

class KnightsEngineImpl {
public:
    explicit KnightsEngineImpl(int nplayers)
        : view_manager(nplayers),
          initial_update_needed(true),
          initial_msgs(0),
          premapped(false),
          respawn_delay(-1),
          lockpick_itemtype(0), lockpick_init_time(-1), lockpick_interval(-1),
          final_gvt(0)
    { } 

    // Keep a reference on the configuration
    boost::shared_ptr<const KnightsConfig> config;
    
    // Game state
    boost::shared_ptr<DungeonMap> dungeon_map;
    boost::shared_ptr<CoordTransform> coord_transform;
    std::vector<boost::shared_ptr<Player> > players;

    // "Manager" classes (these handle various types of update to the
    // game world, and in some cases hold game state as well).    
    EventManager event_manager;      // processes "script events"
    GoreManager gore_manager;        // places blood splats
    HomeManager home_manager;        // handles securing of homes
    MonsterManager monster_manager;  // controls monster generation
    QuestHintManager quest_hint_manager;   // stores quest hints, sends out to players
    StuffManager stuff_manager;      // holds contents of stuff bags
    TaskManager task_manager;        // holds all "tasks" waiting to run.
    ViewManager view_manager;        // sends updates of dungeon views / mini maps to clients.

    // setup variables (set by Lua functions during game startup)
    bool initial_update_needed;
    std::vector<std::string> *initial_msgs;
    std::vector<std::pair<ItemType *, std::vector<int> > > starting_gears;
    bool premapped;
    std::vector<ItemType*> respawn_items;
    int respawn_delay, respawn_interval;
    ItemType *lockpick_itemtype;
    int lockpick_init_time, lockpick_interval;

    // this holds the GVT at which the game will end (or zero if no time limit)
    int final_gvt;
    
    // players to be eliminated are queued, because we have to call Mediator::eliminatePlayer()
    // from within update() (otherwise the callbacks won't have been set)
    std::vector<int> players_to_eliminate;

    // ditto for speech bubble requests.
    std::map<int,bool> speech_bubble_requests;

    std::vector<int> house_col_idxs;

    void doInitialUpdateIfNeeded();
};


KnightsEngine::KnightsEngine(boost::shared_ptr<KnightsConfig> config,
                             const std::vector<int> &hse_cols,
                             const std::vector<std::string> &player_names,
                             bool &deathmatch_mode,
                             std::vector<std::string> &messages)
{
    try {

        // Create new KnightsEngineImpl
        pimpl.reset(new KnightsEngineImpl(hse_cols.size()));
        
        // Create the mediator, add stuff to it
        Mediator::createInstance(pimpl->event_manager, pimpl->gore_manager,
                                 pimpl->home_manager, pimpl->monster_manager,
                                 pimpl->quest_hint_manager,
                                 pimpl->stuff_manager, pimpl->task_manager,
                                 pimpl->view_manager, config->getConfigMap(),
                                 config->getLuaState());


        // Most initialization work is delegated to the KnightsConfig object
        config->initializeGame(pimpl->home_manager,
                               pimpl->players,
                               pimpl->stuff_manager,
                               pimpl->gore_manager,
                               pimpl->monster_manager,
                               pimpl->event_manager,
                               pimpl->task_manager,
                               hse_cols,
                               player_names);
        
        pimpl->house_col_idxs = hse_cols;

        for (int i = 0; i < pimpl->players.size(); ++i) {
            Mediator::instance().addPlayer(*pimpl->players[i]);
        }

        // Make sure we save a copy of the config.
        pimpl->config = config;        

        // Create the map etc ready for Lua to fill in
        resetMap();

        // Run the Lua game startup functions.
        {
            LuaStartupSentinel s(config->getLuaState().get(), *this);
            pimpl->initial_msgs = &messages;
            std::string err_msg;
            bool can_start = config->runGameStartup(err_msg);
            pimpl->initial_msgs = 0;
            if (!can_start) {
                throw LuaError(err_msg);
            }
        }

        // Setup the ItemRespawnTask
        // 
        // (Can't do this directly from Lua because the item respawn
        // and lockpick spawn are mixed together in the same Task...
        // maybe should separate them out?)

        boost::shared_ptr<ItemRespawnTask>
            respawn_task(new ItemRespawnTask(pimpl->respawn_items,
                                             pimpl->respawn_delay,
                                             config->getConfigMap()->getInt("item_respawn_interval"),
                                             pimpl->lockpick_itemtype,
                                             pimpl->lockpick_init_time,
                                             pimpl->lockpick_interval));
        pimpl->task_manager.addTask(respawn_task,
                                    TP_NORMAL,
                                    pimpl->task_manager.getGVT() + 1);

        // Set up task to check critical items and respawn them if necessary
        const int item_check_interval = config->getConfigMap()->getInt("item_check_interval");
        boost::shared_ptr<ItemCheckTask> chktsk(
            new ItemCheckTask(*pimpl->dungeon_map, item_check_interval));
        pimpl->task_manager.addTask(chktsk, TP_NORMAL, pimpl->task_manager.getGVT() + item_check_interval);

        // Set up time limit task
        if (pimpl->final_gvt > 0) {
            boost::shared_ptr<TimeLimitTask> ttsk(new TimeLimitTask);
            pimpl->task_manager.addTask(ttsk, TP_NORMAL, pimpl->final_gvt);
        }

        // Tell caller whether this is a deathmatch
        deathmatch_mode = Mediator::instance().getDeathmatchMode();
        
    } catch (...) {

        pimpl->initial_msgs = 0;
        
        // If there was an error in ctor then make sure Mediator gets
        // destroyed properly.
        Mediator::destroyInstance();
        
        throw; // re-throw the exception
    }
}

void KnightsEngine::resetMap()
{
    pimpl->coord_transform.reset(new CoordTransform);
    pimpl->dungeon_map.reset(new DungeonMap);
    Mediator &m = Mediator::instance();
    m.getHomeManager().clear();
    m.getMonsterManager().clear();
    m.setMap(pimpl->dungeon_map, pimpl->coord_transform);
}

KnightsEngine::~KnightsEngine()
{
    try {
        // To prevent MediatorUnavailable exceptions we create a "dummy" callbacks object
        DummyCallbacks dcb;
        Mediator::instance().setCallbacks(&dcb);

        // Clear the map out first. This makes sure that (for example)
        // Knight dtor does not attempt to call methods on the deleted Player objects.
        if (pimpl->dungeon_map) {
            pimpl->dungeon_map->clearAll();
            pimpl->dungeon_map.reset();
        }

        // Now delete the pimpl
        pimpl.reset();

        // Finally, release the mediator.
        Mediator::destroyInstance();
    
    } catch (...) {
        // Prevent exceptions from escaping.
    }
}


//
// update
//

int KnightsEngine::getTimeToNextUpdate() const
{
    return pimpl->task_manager.getTimeToNextUpdate();
}

void KnightsEngine::update(int time_delta, KnightsCallbacks &callbacks)
{
    Mediator::instance().setCallbacks(&callbacks); 

    for (std::vector<int>::const_iterator it = pimpl->players_to_eliminate.begin(); it != pimpl->players_to_eliminate.end(); ++it) {
        if (*it >= 0 && *it < pimpl->players.size()) {
            Mediator::instance().eliminatePlayer(*pimpl->players[*it]);
        }
    }
    pimpl->players_to_eliminate.clear();

    for (std::map<int, bool>::const_iterator it = pimpl->speech_bubble_requests.begin(); it != pimpl->speech_bubble_requests.end(); ++it) {
        boost::shared_ptr<Player> p = pimpl->players.at(it->first);
        p->setSpeechBubble(it->second);
    }
    pimpl->speech_bubble_requests.clear();
    
    pimpl->doInitialUpdateIfNeeded();
    pimpl->task_manager.advanceToTime(pimpl->task_manager.getGVT() + time_delta);
    Mediator::instance().setCallbacks(0);
}

void KnightsEngineImpl::doInitialUpdateIfNeeded()
{
    if (initial_update_needed) {

        // Set each player's mini map size
        const int width = dungeon_map->getWidth();
        const int height = dungeon_map->getHeight();
        for (int i = 0; i < players.size(); ++i) {
            players[i]->setMiniMapSize(width, height);
        }

        for (int i = 0; i < players.size(); ++i) {
            // Transfer the starting gear to the Player
            for (int j = 0; j < starting_gears.size(); ++j) {
                players[i]->addStartingGear(*starting_gears[j].first, starting_gears[j].second);
            }
            
            // Spawn the player (and keeps trying if it fails)
            players[i]->scheduleRespawn(0);
        }
        
        // Do premapping if wanted
        if (premapped) {
            for (int i = 0; i < players.size(); ++i) {
                MagicMapping(players[i]->getKnight());
            }
        }

        // Send initial quest hint update
        Mediator::instance().getQuestHintManager().sendHints();

        initial_update_needed = false;
    }
}

//
// catchUp
//

void KnightsEngine::catchUp(int player, DungeonView &dungeon_view, MiniMap &mini_map, StatusDisplay &status_display)
{
    // Send the mini map
    pimpl->players[player]->sendMiniMap(mini_map);

    // Send the status display
    pimpl->players[player]->sendStatusDisplay(status_display);
    
    // Send the current room to the DungeonView
    pimpl->view_manager.sendCurrentRoom(player, dungeon_view);
}


//
// setControl
//

void KnightsEngine::setControl(int player, const UserControl *control)
{
    const Control * ctrl = static_cast<const Control*>(control);
    boost::shared_ptr<Player> p = pimpl->players.at(player);
    p->setControl(ctrl);

    // Clear speech bubble whenever we receive a control.
    setSpeechBubble(player, false);
}

void KnightsEngine::setApproachBasedControls(int player, bool flag)
{
    boost::shared_ptr<Player> p = pimpl->players.at(player);
    p->setApproachBasedControls(flag);
}

void KnightsEngine::setActionBarControls(int player, bool flag)
{
    boost::shared_ptr<Player> p = pimpl->players.at(player);
    p->setActionBarControls(flag);
}

void KnightsEngine::setSpeechBubble(int player, bool show)
{
    pimpl->speech_bubble_requests[player] = show;
}

int KnightsEngine::getNumPlayersRemaining() const
{
    return Mediator::instance().getNumPlayersRemaining();
}

void KnightsEngine::eliminatePlayer(int player)
{
    pimpl->players_to_eliminate.push_back(player);
}

namespace {
    struct ComparePlayerInfo {
        bool operator()(const PlayerInfo &lhs, const PlayerInfo &rhs) const
        {
            // have to write in this strange way (call operator< directly)
            // because some other operator< is coming in (from boost?)
            // and causing ambiguity.
            return lhs.house_colour.operator< (rhs.house_colour) ? true
                : rhs.house_colour.operator< (lhs.house_colour) ? false
                : lhs.name < rhs.name;
        }
    };
}

void KnightsEngine::getPlayerList(std::vector<PlayerInfo> &player_list) const
{
    player_list.clear();
    player_list.reserve(pimpl->players.size());

    std::vector<Coercri::Color> house_colours;
    pimpl->config->getHouseColours(house_colours);
    
    int plyr_num = 0;
    for (std::vector<boost::shared_ptr<Player> >::const_iterator it = pimpl->players.begin(); it != pimpl->players.end(); ++it, ++plyr_num) {
        PlayerInfo inf;
        inf.name = (*it)->getName();
        inf.house_colour = house_colours.at(pimpl->house_col_idxs.at(plyr_num));
        inf.player_num = plyr_num;
        inf.kills = (*it)->getKills();
        inf.deaths = (*it)->getNSkulls();
        inf.frags = (*it)->getFrags();
        inf.eliminated = (*it)->getElimFlag();
        player_list.push_back(inf);
    }

    std::sort(player_list.begin(), player_list.end(), ComparePlayerInfo());
}

int KnightsEngine::getSkullsPlusKills() const
{
    int result = 0;
    for (std::vector<boost::shared_ptr<Player> >::const_iterator it = pimpl->players.begin(); it != pimpl->players.end(); ++it) {
        result += (*it)->getNSkulls() + (*it)->getKills();
    }
    return result;
}

int KnightsEngine::getTimeRemaining() const
{
    if (pimpl->final_gvt == 0) return -1;
    else return std::max(0, pimpl->final_gvt - pimpl->task_manager.getGVT());
}


//
// Functions called by Lua.
//

void KnightsEngine::setPremapped(bool pm)
{
    pimpl->premapped = pm;
}

void KnightsEngine::gameStartupMsg(const std::string &msg)
{
    if (pimpl->initial_msgs) {
        pimpl->initial_msgs->push_back(msg);
    } else {
        throw UnexpectedError("KnightsEngine::gameStartupMsg called at unexpected time");
    }
}

void KnightsEngine::addStartingGear(ItemType *itype, const std::vector<int> &num)
{
    pimpl->starting_gears.push_back(std::make_pair(itype, num));
}

void KnightsEngine::setItemRespawn(const std::vector<ItemType*> &items_to_respawn, int respawn_delay)
{
    pimpl->respawn_items = items_to_respawn;
    pimpl->respawn_delay = respawn_delay;
}

void KnightsEngine::setLockpickSpawn(ItemType *lockpicks, int init_time, int interval)
{
    pimpl->lockpick_itemtype = lockpicks;
    pimpl->lockpick_init_time = init_time;
    pimpl->lockpick_interval = interval;
}

void KnightsEngine::setTimeLimit(int ms)
{
    pimpl->final_gvt = pimpl->task_manager.getGVT() + ms;
}

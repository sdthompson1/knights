/*
 * knights_engine.cpp
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

#include "control.hpp"
#include "dummy_callbacks.hpp"
#include "dungeon_map.hpp"
#include "event_manager.hpp"
#include "gore_manager.hpp"
#include "home_manager.hpp"
#include "knights_config.hpp"
#include "knights_engine.hpp"
#include "magic_map.hpp"
#include "mediator.hpp"
#include "menu_selections.hpp"
#include "mini_map.hpp"
#include "monster_manager.hpp"
#include "player.hpp"
#include "round.hpp"
#include "stuff_bag.hpp"
#include "task_manager.hpp"
#include "tutorial_manager.hpp"
#include "view_manager.hpp"

class KnightsEngineImpl {
public:
    explicit KnightsEngineImpl(int nplayers)
        : view_manager(nplayers),
          initial_update_needed(true),
          premapped(false)
    { } 

    // Keep a reference on the configuration
    boost::shared_ptr<const KnightsConfig> config;
    
    // Game state
    boost::shared_ptr<DungeonMap> dungeon_map;
    std::vector<boost::shared_ptr<Player> > players;

    // "Manager" classes (these handle various types of update to the
    // game world, and in some cases hold game state as well).    
    EventManager event_manager;      // processes "script events"
    GoreManager gore_manager;        // places blood splats
    HomeManager home_manager;        // handles securing of homes
    MonsterManager monster_manager;  // controls monster generation
    StuffManager stuff_manager;      // holds contents of stuff bags
    TaskManager task_manager;        // holds all "tasks" waiting to run.
    ViewManager view_manager;        // sends updates of dungeon views / mini maps to clients.

    std::vector<std::pair<const ItemType *, std::vector<int> > > starting_gears;
    bool initial_update_needed;
    bool premapped;

    // this holds the GVT at which the game will end (or zero if no time limit)
    int final_gvt;
    
    // players to be eliminated are queued, because we have to call Mediator::eliminatePlayer()
    // from within update() (otherwise the callbacks won't have been set)
    std::vector<int> players_to_eliminate;

    // ditto for speech bubble requests.
    std::map<int,bool> speech_bubble_requests;

    std::vector<int> house_col_idxs;

    boost::shared_ptr<TutorialManager> tutorial_manager;

    void doInitialUpdateIfNeeded();
};


KnightsEngine::KnightsEngine(boost::shared_ptr<KnightsConfig> config,
                             const MenuSelections &msel,
                             const std::vector<int> &hse_cols,
                             const std::vector<std::string> &player_names,
                             bool tutorial_mode,
                             std::string &warning_msg)
    : pimpl(new KnightsEngineImpl(hse_cols.size()))
{
    boost::shared_ptr<TutorialManager> tutorial_manager;
    if (tutorial_mode) tutorial_manager.reset(new TutorialManager);
    pimpl->tutorial_manager = tutorial_manager;

    // Create the mediator, add stuff to it
    Mediator::createInstance(pimpl->event_manager, pimpl->gore_manager,
                             pimpl->home_manager, pimpl->monster_manager,
                             pimpl->stuff_manager, pimpl->task_manager,
                             pimpl->view_manager, config->getConfigMap(),
                             tutorial_manager, config->getLuaState());

    try {
        // Most initialization work is delegated to the KnightsConfig object
        std::vector<boost::shared_ptr<Quest> > quests;
        bool premapped = false;
        warning_msg =
            config->initializeGame(msel,
                                   pimpl->dungeon_map,
                                   quests,
                                   pimpl->home_manager,
                                   pimpl->players,
                                   pimpl->stuff_manager,
                                   pimpl->gore_manager,
                                   pimpl->monster_manager,
                                   pimpl->event_manager,
                                   pimpl->premapped,
                                   pimpl->starting_gears,
                                   pimpl->task_manager,
                                   hse_cols,
                                   player_names,
                                   tutorial_manager.get(),
                                   pimpl->final_gvt);
        
        pimpl->house_col_idxs = hse_cols;

        Mediator::instance().setMap(pimpl->dungeon_map);
        
        for (int i = 0; i < pimpl->players.size(); ++i) {
            Mediator::instance().addPlayer(*pimpl->players[i]);
        }
        
        // Make sure we save a copy of the config.
        pimpl->config = config;

    } catch (...) {
        // If there was an error in ctor then make sure Mediator gets
        // destroyed properly.
        Mediator::destroyInstance();
        
        throw; // re-throw the exception
    }
}

KnightsEngine::~KnightsEngine()
{
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
}


//
// update
//

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
            
            // Spawn the player
            players[i]->respawn();
        }
        
        // Do premapping if wanted
        if (premapped) {
            for (int i = 0; i < players.size(); ++i) {
                MagicMapping(players[i]->getKnight());
            }
        }

        // Send out special quest message if needed
        for (int i = 0; i < players.size(); ++i) {
            Mediator::instance().getCallbacks().getStatusDisplay(i).setQuestMessage(players[i]->getQuestMessage());
        }
        
        // Send out initial quest icons
        for (int i = 0; i < players.size(); ++i) {
            std::vector<StatusDisplay::QuestIconInfo> icons;
            players[i]->getQuestIcons(icons);
            Mediator::instance().getCallbacks().getStatusDisplay(i).setQuestIcons(icons);
        }

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
    if (pimpl->tutorial_manager && !flag) pimpl->tutorial_manager->setOldStyleControls();
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
        inf.eliminated = (*it)->getElimFlag();
        player_list.push_back(inf);
    }
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

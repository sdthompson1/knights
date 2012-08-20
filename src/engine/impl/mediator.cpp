/*
 * mediator.cpp
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
#include "creature.hpp"
#include "dungeon_view.hpp"
#include "entity.hpp"
#include "event_manager.hpp"
#include "gore_manager.hpp"
#include "home_manager.hpp"
#include "knight.hpp"
#include "knights_callbacks.hpp"
#include "lua_game_setup.hpp"
#include "mediator.hpp"
#include "monster_manager.hpp"
#include "originator.hpp"
#include "player.hpp"
#include "player_task.hpp"
#include "task_manager.hpp"
#include "tutorial_manager.hpp"
#include "view_manager.hpp"

#include "boost/thread/tss.hpp"

#include <sstream>
#include <stdexcept>

namespace {
    // Thread-local storage for the mediator.
    boost::thread_specific_ptr<Mediator> g_mediator_ptr;
}


//
// initialization stuff
//

void Mediator::createInstance(EventManager &em, GoreManager &gm, HomeManager &hm,
                              MonsterManager &mm,
                              QuestHintManager &qm,
                              StuffManager &sm, TaskManager &tm,
                              ViewManager &vm, boost::shared_ptr<const ConfigMap> cmap,
                              boost::shared_ptr<lua_State> lua)
{
    if (g_mediator_ptr.get()) throw MediatorCreatedTwice();
    g_mediator_ptr.reset(new Mediator(em, gm, hm, mm, qm, sm, tm, vm, cmap, lua));
}

void Mediator::destroyInstance()
{
    g_mediator_ptr.reset();
}

void Mediator::addPlayer(Player &player)
{
    players.push_back(&player);
    remaining_players.insert(&player);
    view_manager.onAddPlayer(player);
    shared_ptr<PlayerTask> pt(new PlayerTask(player)); 
    task_manager.addTask(pt, TP_NORMAL, 1);  // initial exec time does not really matter
}


//
// instance
//

Mediator & Mediator::instance()
{
    Mediator *m = g_mediator_ptr.get();
    if (!m) throw MediatorUnavailable();
    else return *m;
}


//
// configmap
//

int Mediator::cfgInt(const std::string &key) const
{
    return config_map->getInt(key);
}

float Mediator::cfgProbability(const std::string &key) const
{
    float p = config_map->getFloat(key);
    if (p < 0 || p > 1) throw std::runtime_error("Config error: " + key + " must be between 0 and 1");
    return p;
}

const std::string &Mediator::cfgString(const std::string &key) const
{
    return config_map->getString(key);
}


//
// printing msgs
//

void Mediator::gameMsg(int player_num, const std::string &msg, bool is_err)
{
    if (callbacks) {
        callbacks->gameMsg(player_num, msg, is_err);
    } else {
        GameStartupMsg(lua_state.get(), msg);
    }
}


//
// entities
//

void Mediator::onAddEntity(shared_ptr<Entity> entity)
{
    view_manager.onAddEntity(entity);
    Creature *cr = dynamic_cast<Creature*>(entity.get());
    if (cr) {
        event_manager.onAddCreature(*cr);
        if (tutorial_manager) {
            Knight *kt = dynamic_cast<Knight*>(cr);
            if (kt && kt->getPlayer()) {
                tutorial_manager->onMoveKnight(*kt->getPlayer());
            }
        }
    }
}

void Mediator::onRmEntity(shared_ptr<Entity> entity)
{
    view_manager.onRmEntity(entity);
    Creature *cr = dynamic_cast<Creature*>(entity.get());
    if (cr) event_manager.onRmCreature(*cr);
}

void Mediator::onRepositionEntity(shared_ptr<Entity> entity)
{
    view_manager.onRepositionEntity(entity);
    Creature *cr = dynamic_cast<Creature*>(entity.get());
    if (cr) {
        event_manager.postRepositionCreature(*cr);
        if (tutorial_manager) {
            Knight *kt = dynamic_cast<Knight*>(cr);
            if (kt && kt->getPlayer()) {
                tutorial_manager->onMoveKnight(*kt->getPlayer());
            }
        }
    }
}

void Mediator::onChangeEntityMotion(shared_ptr<Entity> entity, bool missile_mode)
{
    view_manager.onChangeEntityMotion(entity, missile_mode);
    Creature *cr = dynamic_cast<Creature*>(entity.get());
    if (cr) event_manager.onChangeEntityMotion(*cr);
}

void Mediator::onFlipEntityMotion(shared_ptr<Entity> entity)
{
    view_manager.onFlipEntityMotion(entity);
}

void Mediator::onChangeEntityAnim(shared_ptr<Entity> entity)
{
    view_manager.onChangeEntityAnim(entity);
}

void Mediator::onChangeEntityFacing(shared_ptr<Entity> entity)
{
    view_manager.onChangeEntityFacing(entity);
}

void Mediator::onChangeEntityVisible(shared_ptr<Entity> entity)
{
    view_manager.onChangeEntityVisible(entity);
}

void Mediator::onChangeSpeechBubble(shared_ptr<Knight> kt)
{
    view_manager.onChangeSpeechBubble(kt);
}


//
// icons
//

void Mediator::placeIcon(DungeonMap &dmap, const MapCoord &mc, const Graphic *g, int duration)
{
    view_manager.placeIcon(dmap, mc, g, duration);
}


//
// items
//

void Mediator::onAddItem(const DungeonMap &dmap, const MapCoord &mc, const Item &item)
{
    view_manager.onAddItem(dmap, mc, item);
}

void Mediator::onRmItem(const DungeonMap &dmap, const MapCoord &mc, const Item &item)
{
    view_manager.onRmItem(dmap, mc, item);
}

void Mediator::onChangeItemGraphic(const DungeonMap &dmap, const MapCoord &mc, const Item &item)
{
    view_manager.onChangeItemGraphic(dmap, mc, item);
}

void Mediator::onPickup(const Player &pl, const ItemType &it)
{
    if (tutorial_manager) tutorial_manager->onPickup(pl, it);
}


//
// tiles
//

void Mediator::onAddTile(DungeonMap &dmap, const MapCoord &mc, Tile &tile, const Originator &originator)
{
    view_manager.onAddTile(dmap, mc, tile);
    event_manager.onAddTile(dmap, mc, tile, originator);
}

void Mediator::onRmTile(DungeonMap &dmap, const MapCoord &mc, Tile &tile, const Originator &originator)
{
    view_manager.onRmTile(dmap, mc, tile);
    event_manager.onRmTile(dmap, mc, tile, originator);
}

void Mediator::onChangeTile(const DungeonMap &dmap, const MapCoord &mc, const Tile &tile)
{
    view_manager.onChangeTile(dmap, mc, tile);
}


//
// generic event hooks
//

void Mediator::runHook(const string &hook_name, shared_ptr<Creature> cr)
{
    event_manager.runHook(hook_name, cr);
}

void Mediator::runHook(const string &hook_name, DungeonMap *dmap, const MapCoord &mc)
{
    event_manager.runHook(hook_name, dmap, mc);
}


//
// blood/gore effects
//

void Mediator::placeBlood(DungeonMap &dmap, const MapCoord &mc)
{
    gore_manager.placeBlood(dmap, mc);
}

void Mediator::placeKnightCorpse(DungeonMap &dmap, const MapCoord &mc, const Player &p, bool b)
{
    gore_manager.placeKnightCorpse(dmap, mc, p, b);
    monster_manager.onPlaceKnightCorpse(mc);
}

void Mediator::placeMonsterCorpse(DungeonMap &dmap, const MapCoord &mc, const MonsterType &m)
{
    gore_manager.placeMonsterCorpse(dmap, mc, m);
    monster_manager.onPlaceMonsterCorpse(mc, m);
}

//
// monster death notification
//

void Mediator::onMonsterDeath(const MonsterType &type)
{
    // NOTE: This is called from a dtor, therefore MUST NOT raise Lua errors.
    monster_manager.subtractMonster(type);
}


//
// tutorial specific
//

void Mediator::onOpenLockable(const MapCoord &mc)
{
    // This is used to show the 'Item' message after a chest is opened
    if (tutorial_manager && !players.empty() && players[0]) {
        // we assume there is only one player in the tutorial
        tutorial_manager->onOpenLockable(*players[0], mc);
    }
}

void Mediator::onKnightDeath(Player &player, const DungeonMap &dmap, const MapCoord &mc)
{
    if (tutorial_manager) {
        tutorial_manager->onKnightDeath(player, dmap, mc);
    }

    home_manager.onKnightDeath(player);
}


//
// Misc view effects
//

void Mediator::flashScreen(shared_ptr<Entity> ent, int delay)
{
    view_manager.flashScreen(ent, delay);
}

void Mediator::playSound(DungeonMap &dm, const MapCoord &mc, const Sound &sound, int frequency, bool all)
{
    view_manager.playSound(dm, mc, sound, frequency, all);
}


//
// secureHome
//

bool Mediator::isSecurableHome(const Player &pl, DungeonMap *dmap, const MapCoord &mc, MapDirection facing) const
{
    return home_manager.isSecurableHome(pl, dmap, mc, facing);
}

bool Mediator::secureHome(Player &pl, DungeonMap &dmap, const MapCoord &pos,
                          MapDirection facing, shared_ptr<Tile> wall)
{
    return home_manager.secureHome(pl, dmap, pos, facing, wall);
}

//
// ending the game
//

void Mediator::winGame(const Player &pl)
{
    std::vector<const Player*> winners;

    const int team_num = pl.getTeamNum();
    for (std::vector<Player*>::const_iterator it = players.begin(); it != players.end(); ++it) {
        if ((*it)->getTeamNum() == team_num) {
            winners.push_back(*it);
        }
    }
    endGame(winners, "");
}

void Mediator::timeLimitExpired()
{
    std::vector<const Player *> winners;
    std::string msg;
    
    if (deathmatch_mode) {

        // Find out who has the most frags...
        int most_frags = -999999;

        for (std::vector<Player*>::const_iterator it = players.begin(); it != players.end(); ++it) {

            if ((*it)->getFrags() > most_frags) {
                most_frags = (*it)->getFrags();
                winners.clear();
            }

            if ((*it)->getFrags() == most_frags) {
                winners.push_back(*it);
            }
        }

    } else {
        msg = "Time limit expired! ";
    }
    
    endGame(winners, msg);
}

void Mediator::endGame(const std::vector<const Player *> &winners, std::string msg)
{
    game_running = false;

    // tell clients that the game has ended
    for (std::vector<Player*>::const_iterator it = players.begin(); it != players.end(); ++it) {
        if (std::find(winners.begin(), winners.end(), *it) == winners.end()) {
            Mediator::getCallbacks().loseGame((*it)->getPlayerNum());
        } else {
            Mediator::getCallbacks().winGame((*it)->getPlayerNum());
        }
    }

    // Send the message saying who has won.

    if (winners.empty()) {
        msg += "All players lose!";
        
    } else if (winners.size() == 1) {
        msg += winners.front()->getName() + " is the winner!";
        
    } else {
        for (int i = 0; i < int(winners.size()) - 2; ++i) {
            msg += winners[i]->getName() + ", ";
        }
        msg += winners[winners.size()-2]->getName() + " and ";
        msg += winners[winners.size()-1]->getName() + " are the winners!";
    }
    
    Mediator::getCallbacks().gameMsg(-1, msg);

    // print length of game, in mins and seconds
    const int time = getGVT()/1000;
    const int mins = time / 60;
    const int secs = time % 60;
    std::ostringstream str;
    str << "Game completed in " << mins << "m " << secs << "s.";
    Mediator::getCallbacks().gameMsg(-1, str.str());
    
    // kill all tasks, this prevents anything further from happening in-game.
    task_manager.rmAllTasks();
}

void Mediator::eliminatePlayer(Player &pl)
{
    if (remaining_players.find(&pl) == remaining_players.end()) return;   // already eliminated!

    shared_ptr<Knight> kt = pl.getKnight();
    if (kt) {
        // suicide him
        runHook("HOOK_KNIGHT_DAMAGE", kt);
        kt->onDeath(Creature::NORMAL_MODE, Originator(OT_None()));
        kt->rmFromMap();
    }

    remaining_players.erase(&pl);
    pl.resetHome(0, MapCoord(), D_NORTH);  // Stops him respawning

    pl.setElimFlag();

    bool two_teams_found = false;
    int team = -1;
    for (std::set<const Player*>::const_iterator it = remaining_players.begin(); it != remaining_players.end(); ++it) {
        if (team == -1) {
            team = (*it)->getTeamNum();
        } else if (team != (*it)->getTeamNum()) {
            // Two different teams found
            two_teams_found = true;
            break;
        }
    }
    const bool game_over = !two_teams_found;
    
    if (game_over) {
        // end the game
        if (remaining_players.empty()) {
            // The only way this can happen is if no entry points have been set at 
            // the start of the game
            endGame(std::vector<const Player *>(), "ERROR: Entry point has not been set! ");
        } else {
            // All remaining players are on the same team so any one of them can be used in the call to winGame
            winGame(**remaining_players.begin());
        }
        
    } else {
        // this puts him into observer mode, but he is still in the game (as an observer).
        Mediator::getCallbacks().onElimination(pl.getPlayerNum());
    }
}

int Mediator::getGVT() const
{
    return task_manager.getGVT();
}

KnightsCallbacks & Mediator::getCallbacks() const
{
    if (!callbacks) {
        throw CallbacksUnavailable();
    }
    return *callbacks;
}

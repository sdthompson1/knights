/*
 * mediator.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2025.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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
#include "localization.hpp"
#include "lua_game_setup.hpp"
#include "mediator.hpp"
#include "monster_manager.hpp"
#include "originator.hpp"
#include "player.hpp"
#include "player_task.hpp"
#include "quest_hint_manager.hpp"
#include "task_manager.hpp"
#include "view_manager.hpp"

#ifndef VIRTUAL_SERVER
#include "boost/thread/tss.hpp"
#endif

#include <sstream>
#include <stdexcept>

using std::string;

namespace {
    // Thread-local storage for the mediator.
#ifdef VIRTUAL_SERVER
    // Note: the VIRTUAL_SERVER only supports one game at a time, so using a plain
    // std::unique_ptr is valid in this case
    std::unique_ptr<Mediator> g_mediator_ptr;
#else
    // For the full server, multiple games are supported but each game runs in its
    // own thread, so we use boost::thread_specific_ptr
    boost::thread_specific_ptr<Mediator> g_mediator_ptr;
#endif
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

void Mediator::gameMsgLoc(int player_num, const LocalKey &key, const std::vector<LocalParam> &params, bool is_err)
{
    if (callbacks) {
        callbacks->gameMsgLoc(player_num, key, params, is_err);
    } else {
        GameStartupMsg(lua_state.get(), key, params);
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
    // no longer used
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
    // no longer used
}

void Mediator::onKnightDeath(Player &player, const DungeonMap &dmap, const MapCoord &mc)
{
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

SecureResult Mediator::secureHome(Player &pl, DungeonMap &dmap, const MapCoord &pos,
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
    endGame(winners, false);
}

void Mediator::timeLimitExpired()
{
    std::vector<const Player *> winners;
    bool time_limit_expired_msg = false;
    
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
        time_limit_expired_msg = true;
    }
    
    endGame(winners, time_limit_expired_msg);
}

void Mediator::endGame(const std::vector<const Player *> &winners, bool time_limit_expired)
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

    LocalKey key;
    std::vector<LocalParam> params;

    if (winners.empty()) {
        if (time_limit_expired) {
            key = LocalKey("time_expired");  // Time limit expired! All players lose!
        } else {
            key = LocalKey("all_lose");      // All players lose!
        }
        
    } else if (winners.size() == 1) {
        // <N> is the winner!
        key = LocalKey("is_winner");
        params.push_back(LocalParam(winners.front()->getPlayerID()));

    } else {
        // <N>, <N> and <N> are the winners!
        key = LocalKey("are_winners");
        for (const Player * winner : winners) {
            params.push_back(LocalParam(winner->getPlayerID()));
        }
    }

    Mediator::getCallbacks().gameMsgLoc(-1, key, params);

    // print length of game, in mins and seconds
    const int time = getGVT()/1000;
    const int mins = time / 60;
    const int secs = time % 60;
    // Game completed in {0}m {1}s.
    params.clear();
    params.push_back(LocalParam(mins));
    params.push_back(LocalParam(secs));
    Mediator::getCallbacks().gameMsgLoc(-1, LocalKey("game_completed"), params);

    // kill all tasks, this prevents anything further from happening in-game.
    task_manager.rmAllTasks();
}

void Mediator::changePlayerState(Player &pl, PlayerState new_state)
{
    switch (new_state) {
    case PlayerState::NORMAL:
        // Returning to NORMAL from DISCONNECTED is allowed.
        // Going to NORMAL from any other state is blocked.
        if (pl.getPlayerState() == PlayerState::DISCONNECTED) {
            pl.setPlayerState(PlayerState::NORMAL);

            // Clear their available controls -- this will force available controls
            // to be resent when KnightTask next runs.
            pl.clearCurrentControls();
        }
        break;

    case PlayerState::DISCONNECTED:
        // Going to DISCONNECTED from NORMAL is allowed.
        // Going to DISCONNECTED from any other state is blocked.
        if (pl.getPlayerState() == PlayerState::NORMAL) {
            pl.setPlayerState(PlayerState::DISCONNECTED);
        }
        break;

    case PlayerState::ELIMINATED:
        // We can go to ELIMINATED from either NORMAL or DISCONNECTED,
        // but going from ELIMINATED to itself is blocked.
        if (pl.getPlayerState() != PlayerState::ELIMINATED) {

            // Kill the knight if they still exist
            shared_ptr<Knight> kt = pl.getKnight();
            if (kt) {
                // suicide him
                runHook("HOOK_KNIGHT_DAMAGE", kt);
                kt->onDeath(Creature::NORMAL_MODE, Originator(OT_None()));
                kt->rmFromMap();
            }

            // Remove from remaining_players list and remove their home
            // (which stops them respawning)
            remaining_players.erase(&pl);
            pl.resetHome(0, MapCoord(), D_NORTH);

            // Change the state
            pl.setPlayerState(PlayerState::ELIMINATED);

            // If, post-elimination, only one team remains, the game ends
            bool two_teams_found = false;
            int team = -1;
            for (auto player : remaining_players) {
                if (team == -1) {
                    team = player->getTeamNum();
                } else if (team != player->getTeamNum()) {
                    two_teams_found = true;
                    break;
                }
            }

            const bool game_over = !two_teams_found;
    
            if (game_over) {
                // End the game
                if (remaining_players.empty()) {
                    endGame(std::vector<const Player *>(), "");
                } else {
                    // All remaining players are on the same team so any one of them can be used in the call to winGame
                    winGame(**remaining_players.begin());
                }

            } else {
                // The game continues.
                // Note: ServerCallbacks::onElimination puts the player into observer mode.
                getCallbacks().onElimination(pl.getPlayerNum());
            }
        }
        break;
    }
}

void Mediator::addQuestHint(const std::string &msg, double order, double group)
{
    quest_hint_manager.addHint(msg, order, group);
}

void Mediator::clearQuestHints()
{
    quest_hint_manager.clearHints();
}

void Mediator::sendQuestHints()
{
    for (int i = 0; i < int(players.size()); ++i) {
        quest_hint_manager.sendHints(players[i]->getStatusDisplay());
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

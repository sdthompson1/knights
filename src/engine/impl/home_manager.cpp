/*
 * home_manager.cpp
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

#include "dungeon_map.hpp"
#include "home_manager.hpp"
#include "mediator.hpp"
#include "player.hpp"
#include "rng.hpp"
#include "special_tiles.hpp"

void HomeManager::addHome(const MapCoord &pos, MapDirection facing)
{
    homes[make_pair(pos,facing)] = 0;
}

bool HomeManager::isSecurableHome(const Player &pl, const MapCoord &pos, MapDirection facing) const
{
    HomeMap::const_iterator it = homes.find(make_pair(pos, facing));
    if (it == homes.end()) return false;
    if (it->second == &pl) return false;
    return true;
}

void HomeManager::secureHome(const Player &pl, DungeonMap &dmap, const MapCoord &pos,
                             MapDirection facing, shared_ptr<Tile> secured_wall_tile)
{
    HomeMap::iterator it = homes.find(make_pair(pos,facing));
    if (it == homes.end()) return;
    if (it->second == &pl) return; // don't secure homes twice ...

    // Also: don't secure homes twice by different members of the same team.
    if (it->second && it->second->getTeamNum() != -1 && it->second->getTeamNum() == pl.getTeamNum()) return;
    
    // We have to secure the home
    // First find the home tile
    MapCoord home_mc = DisplaceCoord(pos,facing);
    shared_ptr<Home> home_tile;
    
    vector<shared_ptr<Tile> > tiles;
    dmap.getTiles(home_mc, tiles);
    for (int i=0; i<tiles.size(); ++i) {
        home_tile = dynamic_pointer_cast<Home>(tiles[i]);
        if (home_tile) break;
    }

    // Now do the actual securing
    if (it->second == 0) {
        it->second = &pl; // let the record show that this home is secured by this player only
        if (home_tile) {
            home_tile->secure(dmap, home_mc, pl.getSecuredCC()); // set the colour-change too.
        }
    } else {
        homes.erase(it);  // let the record show that this home is secured by both players
        if (home_tile) {
            dmap.rmTile(home_mc, home_tile, Originator(OT_None()));   // remove the home tile
            dmap.addTile(home_mc, secured_wall_tile, Originator(OT_None()));  // add a wall tile instead
        }
    }

    // Finally, we have to reassign homes if necessary
    const vector<Player*> &players (Mediator::instance().getPlayers());
    for (int i=0; i<players.size(); ++i) {
        // Check the home
        HomeMap::iterator it = homes.find(make_pair(players[i]->getHomeLocation(),
                                                    players[i]->getHomeFacing()));

        bool my_home_secured = false;
        if (it == homes.end()) {
            my_home_secured = true;   // My home has been secured by two players
        } else if (it->second) {
            // My home has been secured by some knight
            if (it->second->getTeamNum() == -1) {
                // Not a team game
                // My home is secured if the securing player is some player other than myself.
                my_home_secured = it->second != players[i];
            } else {
                // Team game
                // My home is secured if the securing player's team is not equal to my team.
                my_home_secured = it->second->getTeamNum() != players[i]->getTeamNum();
            }
        }
        
        if (my_home_secured) {
            // My home seems to have been secured. Replace it with a new one
            pair<MapCoord,MapDirection> p = getRandomHomeFor(*players[i]);
            players[i]->resetHome(p.first, p.second);
        }

        // NOTE: We don't bother reassigning exit points. This means that the exit point
        // remain usable by both players until it is secured by both players. (After that,
        // the quest cannot be completed by escaping the dungeon. This never makes the
        // game impossible though, because you can always win by fighting to the death.)

        // ALSO: The above only applies to random or guarded exits. If the exit is set
        // to "own entry" or "other's entry" then the exit will get reassigned automatically
        // when the relevant home is reassigned.
    }
}

pair<MapCoord,MapDirection> HomeManager::getRandomHomeFor(const Player &pl) const
{
    const bool is_team_game = pl.getTeamNum() >= 0;
    
    // First of all, work out which homes are not secured against pl
    std::vector<std::pair<MapCoord, MapDirection> > unsecured_homes;
    
    for (HomeMap::const_iterator it = homes.begin(); it != homes.end(); ++it) {

        const bool unsecured = it->second == 0;
        const bool secured_by_me = it->second == &pl;
        const bool secured_by_my_team = !unsecured && it->second->getTeamNum() == pl.getTeamNum();
        
        if (unsecured || (is_team_game && secured_by_my_team) || (!is_team_game && secured_by_me)) {
            unsecured_homes.push_back(it->first);
        }
    }

    if (unsecured_homes.empty()) return make_pair(MapCoord(), D_NORTH);
    
    // Generate a random number ...
    int r = g_rng.getInt(0, int(unsecured_homes.size()));

    // ... and return the rth home
    return unsecured_homes[r];
}

void HomeManager::onKnightDeath(Player &pl) const
{
    if (pl.getRespawnType() == Player::R_DIFFERENT_EVERY_TIME) {
        // Randomize the home after every death.
        pair<MapCoord, MapDirection> p = getRandomHomeFor(pl);
        pl.resetHome(p.first, p.second);
    }
}

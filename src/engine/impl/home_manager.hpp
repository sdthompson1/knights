/*
 * home_manager.hpp
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

/*
 * Handles Wands of Securing.
 *
 */

#ifndef HOME_MANAGER_HPP
#define HOME_MANAGER_HPP

#include "map_support.hpp"

#include <map>
using namespace std;

class Player;
class Tile;

class HomeManager {
public:
    // setup -- All homes in the dungeon must be added.
    // The pos is the tile one outside the home, and the facing must point towards
    // the home.
    // NOTE: we assume all homes are in the same DungeonMap at present. 
    void addHome(const MapCoord &pos, MapDirection facing);

    // Secure a home by a given player
    // (This does nothing if the given tile is not a home.)
    void secureHome(const Player &pl,
                    DungeonMap &dmap, const MapCoord &pos, MapDirection facing,
                    shared_ptr<Tile> secured_wall_tile);

    // is this a securable home
    bool isSecurableHome(const Player &pl, const MapCoord &pos, MapDirection facing) const;
    
private:
    // returns a null MapCoord if no such home can be found
    pair<MapCoord,MapDirection> getRandomHomeFor(const Player &pl) const;
    
private:
    // This map stores the secure-status of each home.
    // stored player == 0 means the home is unsecured
    // stored player != 0 means the home is secured by that player
    // Home not in map at all == The home was secured by both players.
    // NOTE: We assume all homes are in the same DungeonMap at present. 
    typedef map<pair<MapCoord,MapDirection>, const Player *> HomeMap;
    HomeMap homes;
};

#endif

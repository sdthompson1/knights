/*
 * home_manager.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2013.
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
    void addHome(DungeonMap &dmap, const MapCoord &pos, MapDirection facing);

    void clear() { homes.clear(); }  // remove all added homes
    
    // Secure a home by a given player
    // (This does nothing if the given tile is not a home.)
    // Returns true if a home was secured
    // (The secured_wall_tile is cloned if necessary.)
    bool secureHome(Player &pl,
                    DungeonMap &dmap, const MapCoord &pos, MapDirection facing,
                    shared_ptr<Tile> secured_wall_tile);

    // is this a securable home
    bool isSecurableHome(const Player &pl, DungeonMap *dmap, const MapCoord &pos, MapDirection facing) const;

    // On-knight-death routine. In "different every time" respawn mode, this
    // randomizes the knight's "home" location. Otherwise it does nothing.
    void onKnightDeath(Player &pl) const;

    // Push home(s) to Lua stack
    void pushAllHomes(lua_State *lua) const;
    void pushHomeFor(lua_State *lua, Player &player) const;
    
private:
    // returns a null DungeonMap/MapCoord if no such home can be found
    void getRandomHomeFor(Player &pl,
                          DungeonMap *& dmap_out,
                          MapCoord &mc_out,
                          MapDirection &dir_out) const;

private:
    struct HomeLocation {
        DungeonMap *dmap;
        MapCoord mc;            // one tile outside the home
        MapDirection facing;    // points from mc towards the home

        bool operator<(const HomeLocation &other) const
        {
            // Dmap/mc are enough to disambiguate different homes,
            // so no need to compare facing
            return mc < other.mc ? true
                : other.mc < mc ? false
                : dmap < other.dmap;
        }
    };

    static void pushHome(lua_State *lua, const std::pair<HomeLocation, Player *> &home);

    // This map stores info about each home.
    // secured_by == 0 means the home is unsecured
    // secured_by != 0 means the home is secured by that player
    // Home not in map at all == The home was secured by both players.
    typedef map<HomeLocation, Player *> HomeMap;
    HomeMap homes;
};

#endif

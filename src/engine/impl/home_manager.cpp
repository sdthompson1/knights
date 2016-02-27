/*
 * home_manager.cpp
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

#include "misc.hpp"

#include "dungeon_map.hpp"
#include "home_manager.hpp"
#include "lua_userdata.hpp"
#include "mediator.hpp"
#include "player.hpp"
#include "rng.hpp"
#include "special_tiles.hpp"

#include "lua.hpp"

using std::vector;

void HomeManager::addHome(DungeonMap &dmap, const MapCoord &pos, MapDirection facing, bool is_special_exit)
{
    ASSERT(!pos.isNull());
    HomeLocation loc;
    loc.dmap = &dmap;
    loc.mc = pos;
    loc.facing = facing;
    if (is_special_exit) {
        special_exits.insert(loc);
    } else {
        homes[loc] = 0;
    }
}

bool HomeManager::isSecurableHome(const Player &pl, DungeonMap *dmap, const MapCoord &pos, MapDirection facing) const
{
    HomeLocation loc;
    loc.dmap = dmap;
    loc.mc = pos;
    loc.facing = facing;
    HomeMap::const_iterator it = homes.find(loc);
    if (it == homes.end()) return false;
    if (it->second == &pl) return false;
    return true;
}

SecureResult HomeManager::secureHome(Player &pl, DungeonMap &dmap, const MapCoord &pos,
                                     MapDirection facing, shared_ptr<Tile> secured_wall_tile)
{
    HomeLocation loc;
    loc.dmap = &dmap;
    loc.mc = pos;
    loc.facing = facing;
    HomeMap::iterator it = homes.find(loc);
    if (it == homes.end()) {
        if (special_exits.find(loc) == special_exits.end()) {
            return SECURE_FAILED_NOT_A_HOME;
        } else {
            return SECURE_FAILED_SPECIAL_EXIT;
        }
    }
    if (it->second == &pl) return SECURE_FAILED_ALREADY_SECURE;

    // Also: don't secure homes twice by different members of the same team.
    if (it->second && it->second->getTeamNum() == pl.getTeamNum()) return SECURE_FAILED_ALREADY_SECURE;
    
    // We have to secure the home
    // First find the home tile
    MapCoord home_mc = DisplaceCoord(pos, facing);
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
            dmap.addTile(home_mc, secured_wall_tile->clone(false), Originator(OT_None()));  // add a wall tile instead
        }
    }

    // Finally, we have to reassign homes if necessary
    const vector<Player*> &players (Mediator::instance().getPlayers());
    for (int i=0; i<players.size(); ++i) {
        // Check the home
        HomeLocation loc;   // shadows previous 'loc'
        loc.dmap = players[i]->getHomeMap();
        loc.mc   = players[i]->getHomeLocation();
        loc.facing = players[i]->getHomeFacing();
        HomeMap::iterator it = homes.find(loc);   // shadows previous 'it'

        bool my_home_secured = false;
        if (it == homes.end()) {
            my_home_secured = true;   // My home has been secured by two players
        } else if (it->second) {
            // My home has been secured by some knight
            // My home is secured if the securing player's team is not equal to my team.
            my_home_secured = it->second->getTeamNum() != players[i]->getTeamNum();
        }
        
        if (my_home_secured) {
            // My home seems to have been secured. Replace it with a new one
            DungeonMap * dmap_new;
            MapCoord mc_new;
            MapDirection dir_new;
            getRandomHomeFor(*players[i], dmap_new, mc_new, dir_new);
            players[i]->resetHome(dmap_new, mc_new, dir_new);
        }
    }

    return SECURE_SUCCEEDED;
}

void HomeManager::getRandomHomeFor(Player &pl, 
                                   DungeonMap *& dmap_out,
                                   MapCoord &mc_out,
                                   MapDirection &dir_out) const
{
    // First of all, work out which homes are not secured against pl
    std::vector<HomeLocation> unsecured_homes;
    
    for (HomeMap::const_iterator it = homes.begin(); it != homes.end(); ++it) {

        const bool unsecured = it->second == 0;
        const bool secured_by_my_team = !unsecured && it->second->getTeamNum() == pl.getTeamNum();
        
        if (unsecured || secured_by_my_team) {
            unsecured_homes.push_back(it->first);
        }
    }

    if (unsecured_homes.empty()) {
        dmap_out = 0;
        mc_out = MapCoord();
        dir_out = D_NORTH;
    
    } else {
    
        // Generate a random number ...
        int r = g_rng.getInt(0, int(unsecured_homes.size()));

        // ... and return the rth home
        dmap_out = unsecured_homes[r].dmap;
        mc_out = unsecured_homes[r].mc;
        dir_out = unsecured_homes[r].facing;
    }
}

void HomeManager::onKnightDeath(Player &pl) const
{
    if (pl.getRespawnType() == Player::R_DIFFERENT_EVERY_TIME) {
        // Randomize the home after every death.
        DungeonMap *dmap;
        MapCoord mc;
        MapDirection facing;
        getRandomHomeFor(pl, dmap, mc, facing);
        pl.resetHome(dmap, mc, facing);
    }
}

void HomeManager::pushHome(lua_State *lua,
                           const HomeLocation &loc,
                           Player * secured_by) 
{
    if (loc.dmap == 0 || loc.mc.isNull()) {
        // The home position is not valid.
        // This means the player does not currently have a home.
        // Return a "nil" value to lua.
        lua_pushnil(lua);

    } else {

        lua_newtable(lua);  // [home]

        lua_pushinteger(lua, loc.mc.getX());   // [home x]
        lua_setfield(lua, -2, "x");            // [home]
        lua_pushinteger(lua, loc.mc.getY());
        lua_setfield(lua, -2, "y");
        PushMapDirection(lua, loc.facing);
        lua_setfield(lua, -2, "facing");
        if (secured_by) {
            NewLuaPtr<Player>(lua, secured_by);   // [home plyr]
            lua_setfield(lua, -2, "secured_by");  // [home]
        }

        // find the home tile itself, makes it easier for lua to inspect it if required.
        vector<shared_ptr<Tile> > tiles;
        loc.dmap->getTiles(DisplaceCoord(loc.mc, loc.facing), tiles);
        for (vector<shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
            Home *home = dynamic_cast<Home*>(it->get());
            if (home) {
                NewLuaSharedPtr<Tile>(lua, *it);
                lua_setfield(lua, -2, "tile");
            }
        }
    }
}

void HomeManager::pushAllHomes(lua_State *lua) const
{
    lua_createtable(lua, homes.size(), 0);   // [homes]
    int n = 1;
    for (HomeMap::const_iterator it = homes.begin(); it != homes.end(); ++it) {
        pushHome(lua, it->first, it->second);   // [homes home]
        lua_rawseti(lua, -2, n++);  // [homes]
    }
    for (HomeSet::const_iterator it = special_exits.begin(); it != special_exits.end(); ++it) {
        pushHome(lua, *it, 0);
        lua_rawseti(lua, -2, n++);
    }
}

void HomeManager::pushHomeFor(lua_State *lua, Player &player) const
{
    HomeLocation loc;
    loc.dmap = player.getHomeMap();
    loc.mc = player.getHomeLocation();
    loc.facing = player.getHomeFacing();

    HomeMap::const_iterator it = homes.find(loc);

    if (it == homes.end()) {
        pushHome(lua, loc, 0);
    } else {
        pushHome(lua, loc, it->second);
    }
}

/*
 * lua_ingame.cpp
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

#include "coord_transform.hpp"
#include "creature.hpp"
#include "dungeon_map.hpp"
#include "knights_callbacks.hpp"
#include "lockable.hpp"
#include "lua_ingame.hpp"
#include "lua_userdata.hpp"
#include "map_support.hpp"
#include "mediator.hpp"
#include "missile.hpp"
#include "teleport.hpp"

#include "lua.hpp"

#include <cstring>

class Player;

namespace {

    // Read a position (MapCoord) from the lua stack at the given index
    MapCoord GetMapCoord(lua_State *lua, int index)
    {
        if (lua_isnil(lua, index)) {
            return MapCoord();
        } else {
            lua_getfield(lua, index, "x");
            if (lua_isnil(lua, -1)) return MapCoord();
            const int x = lua_tointeger(lua, -1);
            lua_pop(lua, 1);

            lua_getfield(lua, index, "y");
            if (lua_isnil(lua, -1)) return MapCoord();
            const int y = lua_tointeger(lua, -1);
            lua_pop(lua, 1);

            return MapCoord(x, y);
        }
    }

    MapDirection GetMapDirection(lua_State *lua, int index)
    {
        const char * x = lua_tostring(lua, index);
        if (strcmp(x, "south") == 0) return D_SOUTH;
        else if (strcmp(x, "west") == 0) return D_WEST;
        else if (strcmp(x, "east") == 0) return D_EAST;
        else return D_NORTH;
    }

    // Read the originator from "cxt"
    Originator GetOriginator(lua_State *lua)
    {
        lua_getglobal(lua, "cxt");          // [cxt]
        lua_getfield(lua, -1, "originator");    // [cxt originator]

        Originator orig = Originator(OT_None());

        if (lua_isstring(lua, -1)) {
            if (strcmp("monster", lua_tostring(lua, -1)) == 0) {
                orig = Originator(OT_Monster());
            }
        } else {
            Player * p = ReadLuaPtr<Player>(lua, -1);
            orig = Originator(OT_Player(), p);
        }

        lua_pop(lua, 2);
        return orig;
    }


    typedef void (Lockable::* LockableFnPtr)(DungeonMap &, const MapCoord &, const Originator &);

    // Reads a position from the top of the lua stack, and a Player from "cxt".
    // Does not modify the lua stack.
    void DoOpenOrClose(lua_State *lua, LockableFnPtr function_ptr)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return;

        MapCoord mc = GetMapCoord(lua, 1);
        
        std::vector<shared_ptr<Tile> > tiles;
        dmap->getTiles(mc, tiles);

        for (std::vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end(); ++it) {
            Tile *tile = it->get();
            Lockable *lockable = dynamic_cast<Lockable*>(tile);
            if (lockable) {
                (lockable->*function_ptr)(*dmap, mc, GetOriginator(lua));
            }
        }
    }

    // Input: position
    // Cxt: player
    // Output: none
    int Open(lua_State *lua)
    {
        DoOpenOrClose(lua, &Lockable::open);
        return 0;
    }

    // Input: position
    // Cxt: player
    // Output: none
    int Close(lua_State *lua)
    {
        DoOpenOrClose(lua, &Lockable::close);
        return 0;
    }

    // Input: position
    // Cxt: player
    // Output: none
    int OpenOrClose(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;

        MapCoord mc = GetMapCoord(lua, 1);
        Originator orig = GetOriginator(lua);

        std::vector<shared_ptr<Tile> > tiles;
        dmap->getTiles(mc, tiles);

        for (std::vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end(); ++it) {
            Tile *tile = it->get();
            Lockable *lockable = dynamic_cast<Lockable*>(tile);
            if (lockable) {
                if (lockable->isClosed()) {
                    lockable->open(*dmap, mc, orig);
                } else {
                    lockable->close(*dmap, mc, orig);
                }
            }
        }

        return 0;
    }

    // Input: position
    // Cxt: none
    // Output: true, false or nil
    int IsOpen(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;

        MapCoord mc = GetMapCoord(lua, 1);

        std::vector<shared_ptr<Tile> > tiles;
        dmap->getTiles(mc, tiles);

        for (std::vector<shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
            Tile *tile = it->get();
            Lockable *lockable = dynamic_cast<Lockable*>(tile);
            if (lockable) {
                lua_pushboolean(lua, ! lockable->isClosed());
                return 1;
            }
        }

        return 0;
    }
    
    // Input: position, direction, item type, boolean (drop_after flag)
    // Cxt: player
    // Output: none
    int AddMissile(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;

        const MapCoord mc = GetMapCoord(lua, 1);
        const MapDirection dir = GetMapDirection(lua, 2);
        const ItemType * itype = ReadLuaPtr<ItemType>(lua, 3);
        const bool drop_after = lua_toboolean(lua, 4) != 0;

        if (itype) {
            CreateMissile(*dmap, mc, dir, *itype, drop_after, false, GetOriginator(lua), true);
        }

        return 0;
    }

    // Input: creature, new position.
    // Cxt: none
    // Output: none
    int Teleport(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;

        shared_ptr<Creature> cr = ReadLuaSharedPtr<Creature>(lua, 1);
        const MapCoord dest = GetMapCoord(lua, 2);

        TeleportToSquare(cr, *dmap, dest);
        return 0;
    }

    // Input: position
    // Cxt: none
    // Output: list of tiles
    int GetTiles(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;

        const MapCoord &mc = GetMapCoord(lua, 1);
        
        std::vector<shared_ptr<Tile> > tiles;
        dmap->getTiles(mc, tiles);
        const int num_tiles = int(tiles.size());
        
        lua_createtable(lua, num_tiles, 0);
        
        for (int idx = 1; idx <= num_tiles; ++idx) {
            lua_pushinteger(lua, idx);
            NewLuaSharedPtr(lua, tiles[idx-1]);
            lua_settable(lua, -3);
        }

        return 1; // return the table
    }

    // Input: position, tile
    // Cxt: player
    // Output: none
    int RemoveTile(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;

        const MapCoord &mc = GetMapCoord(lua, 1);
        shared_ptr<Tile> tile = ReadLuaSharedPtr<Tile>(lua, 2);

        dmap->rmTile(mc, tile, GetOriginator(lua));
        return 0;
    }

    // Input: position, tile
    // Cxt: player
    // Output: none
    int AddTile(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;

        const MapCoord mc = GetMapCoord(lua, 1);
        shared_ptr<Tile> tile = ReadLuaSharedPtr<Tile>(lua, 2);

        if (tile) dmap->addTile(mc, tile->clone(false), GetOriginator(lua));
        return 0;
    }

    // Input: tile
    // Cxt: none
    // Output: user-table associated with the given tile
    int UserTable(lua_State *lua)
    {
        Tile * tile = ReadLuaPtr<Tile>(lua, 1);
        if (tile) {
            lua_pushlightuserdata(lua, tile);     // [tile]
            lua_gettable(lua, LUA_REGISTRYINDEX); // [usertable]
        } else {
            lua_pushnil(lua);   // [nil]
        }
        return 1;
    }
            
    // Input: position (can be null), sound, frequency
    // Cxt: none
    // Output: none
    int PlaySound(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        DungeonMap *dmap = med.getMap().get();
        if (!dmap) return 0;

        const bool all = lua_isnil(lua, 1);
        const MapCoord mc = all ? MapCoord() : GetMapCoord(lua, 1);

        const Sound *sound = ReadLuaPtr<Sound>(lua, 2);
        if (!sound) return 0;

        const int frequency = lua_tointeger(lua, 3);

        med.playSound(*dmap, mc, *sound, frequency, all);
        return 0;
    }

    // Input: creature
    // Cxt: none
    // Output: map coord, or nil if creature is not valid / has been killed.
    int GetPos(lua_State *lua)
    {
        shared_ptr<Creature> cr = ReadLuaSharedPtr<Creature>(lua, 1);
        if (cr) {
            PushMapCoord(lua, cr->getPos());
        } else {
            lua_pushnil(lua);
        }
        return 1;
    }

    // Input: (optional) player, followed by any number of args.
    // Cxt: none
    // Output: none
    int Print(lua_State *lua)
    {
        Mediator &med = Mediator::instance();
        KnightsCallbacks& cb = med.getCallbacks();

        // find out if there is a 'player' argument, if so, convert it to a player number
        const Player * player = ReadLuaPtr<Player>(lua, 1);
        int player_num = -1;
        if (player) {
            for (int i = 0; i < med.getPlayers().size(); ++i) {
                if (med.getPlayers()[i] == player) {
                    player_num = i;
                    break;
                }
            }
            if (player_num < 0) return 0;  // that player doesn't seem to exist (shouldn't happen)
        }

        // build the message
        std::string msg;
        const int start = player ? 2 : 1;
        const int top = lua_gettop(lua);
        for (int i = start; i <= top; ++i) {
            const char *x = lua_tostring(lua, i);
            if (!x) return luaL_error(lua, "'tostring' must return a value to 'print'");
            if (i > start) msg += " ";
            msg += x;
        }

        // print the message
        cb.gameMsg(player_num, msg);
        return 0;
    }

    // Input: map position + two integers
    // Cxt: none
    // Output: two integers (transformed offset)
    int TransformOffset(lua_State *lua)
    {
        // Get the CoordTransform
        Mediator &med = Mediator::instance();
        const CoordTransform * ct = med.getCoordTransform().get();
        if (!ct) return 0;

        // Read the inputs
        const MapCoord base = GetMapCoord(lua, 1);
        int x = lua_tointeger(lua, 2);
        int y = lua_tointeger(lua, 3);
        
        // Do the transform
        ct->transformOffset(base, x, y);

        // Return results
        lua_pushinteger(lua, x);
        lua_pushinteger(lua, y);
        return 2;
    }

    // Input: map position + direction (string)
    // Cxt: none
    // Output: new direction
    int TransformDirection(lua_State *lua)
    {
        // Get the CoordTransform
        Mediator &med = Mediator::instance();
        const CoordTransform * ct = med.getCoordTransform().get();
        if (!ct) return 0;

        // Read the inputs
        const MapCoord base = GetMapCoord(lua, 1);
        MapDirection dir = GetMapDirection(lua, 2);

        // Do the transform
        ct->transformDirection(base, dir);

        // Return result
        PushMapDirection(lua, dir);
        return 1;
    }
}

void AddLuaIngameFunctions(lua_State *lua)
{
    lua_pushcfunction(lua, &Open);
    lua_setglobal(lua, "open_door");

    lua_pushcfunction(lua, &Close);
    lua_setglobal(lua, "close_door");

    lua_pushcfunction(lua, &OpenOrClose);
    lua_setglobal(lua, "open_or_close_door");

    lua_pushcfunction(lua, &IsOpen);
    lua_setglobal(lua, "is_door_open");
    
    lua_pushcfunction(lua, &AddMissile);
    lua_setglobal(lua, "add_missile");

    lua_pushcfunction(lua, &Teleport);
    lua_setglobal(lua, "teleport");

    lua_pushcfunction(lua, &GetTiles);
    lua_setglobal(lua, "get_tiles");

    lua_pushcfunction(lua, &RemoveTile);
    lua_setglobal(lua, "remove_tile");

    lua_pushcfunction(lua, &AddTile);
    lua_setglobal(lua, "add_tile");

    lua_pushcfunction(lua, &UserTable);
    lua_setglobal(lua, "user_table");

    lua_pushcfunction(lua, &PlaySound);
    lua_setglobal(lua, "play_sound");

    lua_pushcfunction(lua, &GetPos);
    lua_setglobal(lua, "get_pos");

    lua_pushcfunction(lua, &Print);
    lua_setglobal(lua, "print");

    lua_pushcfunction(lua, &TransformOffset);
    lua_setglobal(lua, "transform_offset");

    lua_pushcfunction(lua, &TransformDirection);
    lua_setglobal(lua, "transform_direction");
}

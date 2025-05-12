/*
 * action_data.cpp
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

#include "action_data.hpp"
#include "creature.hpp"
#include "lua_userdata.hpp"
#include "mediator.hpp"

#include "include_lua.hpp"

#include <cstring>

namespace {
    void PushOriginator(lua_State *lua, const Originator &originator)
    {
        if (originator.isPlayer()) {
            NewLuaPtr<Player>(lua, originator.getPlayer());
        } else if (originator.isMonster()) {
            lua_pushstring(lua, "monster");
        } else {
            lua_pushnil(lua);  // unknown originator
        }
    }

    Originator ReadOriginator(lua_State *lua, int idx)
    {
        Originator orig = Originator(OT_None());
    
        if (lua_isstring(lua, idx)) {
            if (std::strcmp("monster", lua_tostring(lua, idx)) == 0) {
                orig = Originator(OT_Monster());
            }
        } else {
            Player * p = ReadLuaPtr<Player>(lua, idx);
            orig = Originator(OT_Player(), p);
        }

        return orig;
    }
}

// Read the originator from "cxt"
Originator GetOriginatorFromCxt(lua_State *lua)
{
    lua_getglobal(lua, "cxt");              // [cxt]
    if (lua_isnil(lua, -1)) {
        lua_pop(lua, 1);                    // []
        return Originator(OT_None());
    } else {
        lua_getfield(lua, -1, "originator");    // [cxt originator]
        Originator orig = ReadOriginator(lua, -1);
        lua_pop(lua, 2);                        // []
        return orig;
    }
}

// Read everything from "cxt", create new ActionData
ActionData::ActionData(lua_State *lua)
    : originator(OT_None()) // will overwrite below
{
    // read cxt table.
    lua_getglobal(lua, "cxt");   // [cxt]

    lua_pushstring(lua, "actor"); // [cxt "actor"]
    lua_gettable(lua, -2);        // [cxt actor]
    actor = ReadLuaSharedPtr<Creature>(lua, -1);
    lua_pop(lua, 1);              // [cxt]

    lua_pushstring(lua, "victim");
    lua_gettable(lua, -2);
    victim = ReadLuaSharedPtr<Creature>(lua, -1);
    lua_pop(lua, 1);

    lua_pushstring(lua, "item_type");
    lua_gettable(lua, -2);
    item = ReadLuaPtr<ItemType>(lua, -1);
    lua_pop(lua, 1);

    lua_pushstring(lua, "tile");
    lua_gettable(lua, -2);
    tile = ReadLuaSharedPtr<Tile>(lua, -1);
    lua_pop(lua, 1);

    lua_pushstring(lua, "item_pos");
    lua_gettable(lua, -2);
    item_coord = GetMapCoord(lua, -1);
    lua_pop(lua, 1);

    lua_pushstring(lua, "tile_pos");
    lua_gettable(lua, -2);
    tile_coord = GetMapCoord(lua, -1);
    lua_pop(lua, 1);

    lua_pushstring(lua, "pos");
    lua_gettable(lua, -2);
    generic_coord = GetMapCoord(lua, -1);
    lua_pop(lua, 1);

    lua_pushstring(lua, "originator");
    lua_gettable(lua, -2);
    originator = ReadOriginator(lua, -1);
    lua_pop(lua, 1);

    if (!item_coord.isNull() || !tile_coord.isNull() || !generic_coord.isNull()) {
        // We don't currently support multiple DungeonMaps, so just get the map from Mediator.
        DungeonMap *dmap = Mediator::instance().getMap().get();
        item_dmap = item_coord.isNull() ? 0 : dmap;
        tile_dmap = tile_coord.isNull() ? 0 : dmap;
        generic_dmap = generic_coord.isNull() ? 0 : dmap;
    } else {
        item_dmap = tile_dmap = generic_dmap = 0;
    }
}

void ActionData::pushCxtTable(lua_State *lua) const
{
    // create 'cxt' table
    lua_createtable(lua, 0, 9);   // [cxt]

    // use weak ptr for the creature, so that the creature is not
    // prevented from dying just because the lua code kept a
    // reference to it.
    NewLuaWeakPtr<Creature>(lua, getActor());   // [cxt actor]
    lua_setfield(lua, -2, "actor");                   // [cxt]

    if (getActor()) {
        PushMapCoord(lua, getActor()->getPos());  // [cxt pos]
        lua_setfield(lua, -2, "actor_pos");     // [cxt]
    }

    // same for 'victim'
    NewLuaWeakPtr<Creature>(lua, getVictim());
    lua_setfield(lua, -2, "victim");

    if (getVictim()) {
        PushMapCoord(lua, getVictim()->getPos());
        lua_setfield(lua, -2, "victim_pos");
    }

    NewLuaPtr<ItemType>(lua, item);
    lua_setfield(lua, -2, "item_type");

    PushMapCoord(lua, item_coord);
    lua_setfield(lua, -2, "item_pos");

    NewLuaSharedPtr<Tile>(lua, tile);
    lua_setfield(lua, -2, "tile");

    PushMapCoord(lua, tile_coord);
    lua_setfield(lua, -2, "tile_pos");

    // generic pos
    PushMapCoord(lua, generic_coord);
    lua_setfield(lua, -2, "pos");

    PushOriginator(lua, getOriginator());  // [cxt player]
    lua_setfield(lua, -2, "originator");          // [cxt]
}


void ActionData::setItem(DungeonMap *dmap, const MapCoord &mc, ItemType * it)
{
    // Can't set the item location (dmap & mc) without setting an item
    // (although an item without a location IS allowed). Also, can't
    // set mc without setting dmap & vice versa.
    if (it && dmap && !mc.isNull()) {
        item_dmap = dmap;
        item_coord = mc;
    } else {
        item_dmap = 0;
        item_coord = MapCoord();
    }
    item = it;
}

void ActionData::setTile(DungeonMap *dmap, const MapCoord &mc, shared_ptr<Tile> t)
{
    // Can't set only one of dmap & mc, they must be set together or
    // not at all. Also, while we can set a dmap & mc with no Tile, we
    // must not set a Tile with no dmap & mc.
    if (dmap && !mc.isNull()) {
        tile_dmap = dmap;
        tile_coord = mc;
        tile = t;
    } else {
        tile_dmap = 0;
        tile_coord = MapCoord();
        tile = shared_ptr<Tile>();
    }
}

void ActionData::setGenericPos(DungeonMap *dmap, const MapCoord &mc)
{
    if (dmap && !mc.isNull()) {
        generic_dmap = dmap;
        generic_coord = mc;
    } else {
        generic_dmap = 0;
        generic_coord = MapCoord();
    }
}

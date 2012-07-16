/*
 * action.cpp
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

#include "action.hpp"
#include "creature.hpp"
#include "knights_callbacks.hpp"
#include "lua_exec_coroutine.hpp"
#include "lua_userdata.hpp"
#include "mediator.hpp"
#include "my_exceptions.hpp"
#include "rng.hpp"

#include "lua.hpp"

#include "boost/thread/mutex.hpp"

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
            if (strcmp("monster", lua_tostring(lua, idx)) == 0) {
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
    lua_getfield(lua, -1, "originator");    // [cxt originator]
    Originator orig = ReadOriginator(lua, -1);
    lua_pop(lua, 2);                        // []
    return orig;
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

    flag = false;

    success = false;  // TODO success should be removed.

    lua_pushstring(lua, "item");
    lua_gettable(lua, -2);
    item = ReadLuaPtr<const ItemType>(lua, -1);
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

    NewLuaPtr<const ItemType>(lua, item);
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


void ActionData::setItem(DungeonMap *dmap, const MapCoord &mc, const ItemType * it)
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

void RandomAction::add(const Action *ac, int wt)
{
    if (wt > 0) {
        data.push_back(make_pair(ac, wt));
        total_weight += wt;
    }
}

void RandomAction::execute(const ActionData &ad) const
{
    if (total_weight > 0) {
        int r = g_rng.getInt(0, total_weight);
        for (int i=0; i<data.size(); ++i) {
            r -= data[i].second;
            if (r < 0) {
                if (data[i].first) {
                    data[i].first->execute(ad);
                }
                return;
            }
        }
        ASSERT(0);
    }
}

bool ListAction::possible(const ActionData &ad) const
{
    for (int i=0; i<data.size(); ++i) {
        if (!data[i]->possible(ad)) return false;
    }
    return true;
}

void ListAction::execute(const ActionData &a) const
{
    ActionData ad(a);
    for (int i=0; i<data.size(); ++i) {
        // Note: we update the "success" flag after each action in the sequence.
        // This is done based on the possible() flag.
        const bool successful = data[i]->possible(ad);
        data[i]->execute(ad);
        ad.setSuccess(successful);
    }
}


//
// ActionPars
//

MapDirection ActionPars::getMapDirection(int index)
{
    string d = getString(index);
    for (string::iterator it = d.begin(); it != d.end(); ++it) { 
        *it = toupper(*it);
    }
    if (d == "NORTH") return D_NORTH;
    else if (d == "EAST") return D_EAST;
    else if (d == "SOUTH") return D_SOUTH;
    else if (d == "WEST") return D_WEST;
    error();
    return D_NORTH;
}


//
// ActionMaker
//

boost::mutex g_makers_mutex;

map<string, const ActionMaker*> & MakersMap()
{
    static boost::shared_ptr<map<string, const ActionMaker*> > p(new map<string, const ActionMaker*>);
    return *p;
}

ActionMaker::ActionMaker(const string &name)
{
    // this is called before main() starts (in a single threaded way)
    // so don't need to lock the mutex. Indeed 'g_makers_mutex' may not
    // have been constructed yet so it would not be safe to do so.
    MakersMap()[name] = this;
}

Action * ActionMaker::createAction(const string &name, ActionPars &pars)
{
    const ActionMaker * maker = 0;
    {
        // We lock 'g_makers_mutex' in case std::map does not support
        // multiple concurrent readers.
        boost::lock_guard<boost::mutex> lock(g_makers_mutex);
        map<string, const ActionMaker *> & makers = MakersMap();
        map<string, const ActionMaker *>::iterator it = makers.find(name);
        if (it != makers.end()) maker = it->second;
    }
    if (maker) return maker->make(pars);
    else return 0;
}

LuaAction::LuaAction(boost::shared_ptr<lua_State> lua)
    : lua_state(lua)
{
    lua_rawsetp(lua.get(), LUA_REGISTRYINDEX, this);   // pops function from the stack.
}

LuaAction::~LuaAction()
{
    shared_ptr<lua_State> lua_lock(lua_state.lock());
    if (lua_lock) {
        lua_State * lua = lua_lock.get();
        lua_pushnil(lua);                  // does not raise Lua errors
        lua_rawsetp(lua, LUA_REGISTRYINDEX, this);  // should not raise lua error (except if out of memory, perhaps, but we ignore this possibility.)
    }
}

void LuaAction::execute(const ActionData &ad) const
{
    shared_ptr<lua_State> lua_lock(lua_state);
    lua_State * lua = lua_lock.get();
    
    // Set up "cxt" table
    ad.pushCxtTable(lua);  // [cxt]

    // Get the function from the registry
    lua_rawgetp(lua, LUA_REGISTRYINDEX, const_cast<LuaAction*>(this));  // [cxt func]

    // Call it (with 0 arguments)
    LuaExecCoroutine(lua, 0);
}

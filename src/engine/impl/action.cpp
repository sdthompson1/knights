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
#include "lua.hpp"
#include "lua_load_from_rstream.hpp"  // for LuaExec
#include "lua_userdata.hpp"
#include "mediator.hpp"
#include "my_exceptions.hpp"
#include "rng.hpp"

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

pair<DungeonMap *, MapCoord> ActionData::getPos() const
{
    DungeonMap *dmap;
    MapCoord mc;
    
    if (actor && actor->getMap()) dmap = actor->getMap();
    else if (victim && victim->getMap()) dmap = victim->getMap();
    else if (item_dmap) dmap = item_dmap;
    else if (tile_dmap) dmap = tile_dmap;
    else dmap = 0;

    if (dmap) {
        if (actor && actor->getMap()) mc = actor->getPos();
        else if (victim && victim->getMap()) mc = victim->getPos();
        else if (!item_coord.isNull()) mc = item_coord;
        else if (!tile_coord.isNull()) mc = tile_coord;
    }

    return make_pair(dmap, mc);
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
    lua_pushlightuserdata(lua.get(), this);   // [function key]
    lua_pushvalue(lua.get(), -2);             // [function key function]
    lua_settable(lua.get(), LUA_REGISTRYINDEX);   // [function]
    lua_pop(lua.get(), 1);  // []
}

LuaAction::~LuaAction()
{
    shared_ptr<lua_State> lua_lock(lua_state.lock());
    if (lua_lock) {
        lua_State * lua = lua_lock.get();
        lua_pushlightuserdata(lua, this);
        lua_pushnil(lua);
        lua_settable(lua, LUA_REGISTRYINDEX);
    }
}

void LuaAction::execute(const ActionData &ad) const
{
    shared_ptr<lua_State> lua_lock(lua_state);
    lua_State * lua = lua_lock.get();

    // create 'cxt' table
    lua_createtable(lua, 0, 3);   // [cxt]

    // use weak ptr for the creature, so that the creature is not
    // prevented from dying just because the lua code kept a
    // reference to it.
    NewLuaWeakPtr<Creature>(lua, ad.getActor());   // [cxt actor]
    lua_setfield(lua, -2, "actor");                   // [cxt]

    PushOriginator(lua, ad.getOriginator());  // [cxt player]
    lua_setfield(lua, -2, "player");          // [cxt]

    const MapCoord &mc = ad.getLuaPos();
    PushMapCoord(lua, mc);   // [cxt pos]
    lua_setfield(lua, -2, "pos");   // [cxt]
    lua_setglobal(lua, "cxt");      // []
    
    // get the function from the registry
    lua_pushlightuserdata(lua, const_cast<LuaAction*>(this));  // [key]
    lua_gettable(lua, LUA_REGISTRYINDEX);  // [func]

    try {
        // call the function (with no arguments)
        LuaExec(lua);    // []
    } catch (const LuaError &err) {
        Mediator::instance().getCallbacks().gameMsg(-1, err.what());
    }
}

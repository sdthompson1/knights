/*
 * stuff_bag.cpp
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

#include "action_data.hpp"
#include "dungeon_map.hpp"
#include "mediator.hpp"
#include "knight.hpp"
#include "lua_func_wrapper.hpp"
#include "stuff_bag.hpp"

void StuffContents::addItem(shared_ptr<Item> i)
{
    if (i) contents.push_back(i);
}

namespace {
    struct ItemNumberZero {
        bool operator()(shared_ptr<Item> it) {
            return it->getNumber() == 0;
        }
    };
}

// this also runs "onPickup" events, hence the extra parameters. 
void StuffContents::giveToKnight(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Knight> kt)
{
    if (!kt) return;
    
    // Add items to kt's backpack; remove them from the stuff bag
    for (vector<shared_ptr<Item> >::iterator it = contents.begin(); it != contents.end();
    ++it) {
        int no_added = kt->addToBackpack((*it)->getType(), (*it)->getNumber());
        if (no_added > 0) {
            (*it)->setNumber((*it)->getNumber() - no_added);
            (*it)->getType().onPickUp(dmap, mc, kt); // run the pickup event for that item
        }
    }

    // Clean out any items with zero "number"
    contents.erase(remove_if(contents.begin(), contents.end(), ItemNumberZero()),
                   contents.end());
}

void StuffContents::runDropEvents(DungeonMap &dmap, const MapCoord &mc,
                                  shared_ptr<Knight> kt) const
{
    for (vector<shared_ptr<Item> >::const_iterator it = contents.begin();
    it != contents.end(); ++it) {
        (*it)->getType().onDrop(dmap, mc, kt);
    }
}

void StuffContents::putIntoDisplacedItems(DungeonMap &dmap, shared_ptr<Knight> kt)
{
    for (vector<shared_ptr<Item> >::iterator it = contents.begin(); it != contents.end();
    ++it) {
        dmap.addDisplacedItem(*it);
    }
    runDropEvents(dmap, MapCoord(), kt);
    contents.clear();
}
    
shared_ptr<Item> StuffContents::getSingleItem() const
{
    if (contents.size() == 1) return contents.front();
    else return shared_ptr<Item>();
}


//
// Pickup, Drop actions for stuff bags
//

namespace {
    void DoAction(lua_State *lua, void (StuffManager::*pfn)(const MapCoord &, shared_ptr<Knight>))
    {
        ActionData ad(lua);
        StuffManager &stuff_mgr(Mediator::instance().getStuffManager());

        // The knight comes from the "actor" field of ActionData
        shared_ptr<Knight> kt(dynamic_pointer_cast<Knight>( ad.getActor() ));
        
        // Map and pos come from the "Item" field of ActionData
        DungeonMap *dmap;
        MapCoord mc;
        ItemType *dummy;
        ad.getItem(dmap, mc, dummy);
        if (!dmap || mc.isNull()) return;

        (stuff_mgr.*pfn)(mc, kt);
    }

    int StuffPickup(lua_State *lua)
    {
        DoAction(lua, &StuffManager::doPickUp);
        return 0;
    }

    int StuffDrop(lua_State *lua)
    {
        DoAction(lua, &StuffManager::doDrop);
        return 0;
    }
}


//
// StuffManager
//

void StuffManager::setStuffBagGraphic(lua_State *lua, const Graphic *gfx)
{
    PushCFunction(lua, &StuffPickup);
    LuaFunc pickup_action(lua);

    PushCFunction(lua, &StuffDrop);
    LuaFunc drop_action(lua);

    stuff_bag_item_type.reset(new ItemType(gfx, IS_MAGIC, pickup_action, drop_action));
}

void StuffManager::setStuffContents(DungeonMap &dmap, const MapCoord &mc, const StuffContents &stuff)
{
    ASSERT(!mc.isNull());

    Location loc;
    loc.dmap = &dmap;
    loc.mc = mc;

    stuff_map.erase(loc); // Just in case a "ghost" stuff bag is there already
    stuff_map.insert(make_pair(loc, stuff));
}

const vector<shared_ptr<Item> > * StuffManager::getItems(DungeonMap *dmap, const MapCoord &mc) const
{
    Location loc;
    loc.dmap = dmap;
    loc.mc = mc;

    map<Location, StuffContents>::const_iterator it = stuff_map.find(loc);
    if (it == stuff_map.end()) {
        return 0;
    } else {
        shared_ptr<Item> item_here = dmap->getItem(mc);
        if (!item_here || &item_here->getType() != &getStuffBagItemType()) return 0;  // No actual stuff bag here
        return &it->second.getItems();
    }
}

void StuffManager::doPickUp(const MapCoord &mc, shared_ptr<Knight> actor)
{
    if (!actor) return;
    if (!actor->getMap()) return;

    Location loc;
    loc.dmap = actor->getMap();
    loc.mc = mc;

    map<Location, StuffContents>::iterator it = stuff_map.find(loc);
    if (it == stuff_map.end()) {
        // An empty stuff bag. (This shouldn't really happen.)
        return;
    }

    it->second.giveToKnight(*loc.dmap, mc, actor);

    if (it->second.isEmpty()) {
        // Get rid of the stuff bag once and for all
        stuff_map.erase(it);
    } else {
        // Some items still left in the bag (probably the knight didn't have room to carry
        // them all). In this case we place another bag onto the map.
        shared_ptr<Item> new_bag(new Item(getStuffBagItemType()));
        loc.dmap->addItem(mc, new_bag);
    }
}

void StuffManager::doDrop(const MapCoord &mc, shared_ptr<Knight> actor)
{
    // This only runs the drop events. It's assumed that the stuff bag has already been
    // placed into the map.

    if (!actor) return;
    if (!actor->getMap()) return;

    Location loc;
    loc.dmap = actor->getMap();
    loc.mc = mc;

    map<Location, StuffContents>::const_iterator it = stuff_map.find(loc);
    if (it == stuff_map.end()) return;

    it->second.runDropEvents(*loc.dmap, mc, actor);
}

void StuffManager::countItems(std::map<ItemType *, int> &result) const
{
    for (map<Location, StuffContents>::const_iterator stuff_it = stuff_map.begin();
    stuff_it != stuff_map.end();
    ++stuff_it) {

        shared_ptr<Item> item_here = stuff_it->first.dmap->getItem(stuff_it->first.mc);
        if (!item_here || &item_here->getType() != &getStuffBagItemType()) continue;  // no actual stuff bag here
        
        for (vector<shared_ptr<Item> >::const_iterator item_it = stuff_it->second.getItems().begin();
        item_it != stuff_it->second.getItems().end();
        ++item_it) {
            ItemType * item_type = &((*item_it)->getType());
            std::map<ItemType*, int>::iterator result_it = result.find(item_type);
            if (result_it != result.end()) ++ result_it->second;
        }
    }
}

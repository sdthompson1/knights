/*
 * stuff_bag.cpp
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
#include "mediator.hpp"
#include "knight.hpp"
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
    void DoAction(const ActionData &ad, void (StuffManager::*pfn)(const MapCoord &, shared_ptr<Knight>))
    {
        StuffManager &stuff_mgr(Mediator::instance().getStuffManager());
        shared_ptr<Knight> kt(dynamic_pointer_cast<Knight>( ad.getActor() ));
        // We get map and pos from the "Item" field of the ActionData
        DungeonMap *dmap;
        MapCoord mc;
        const ItemType *dummy;
        ad.getItem(dmap, mc, dummy);
        if (!dmap || mc.isNull()) return;
        (stuff_mgr.*pfn)(mc, kt);
    }
}

void StuffPickup::execute(const ActionData &ad) const
{
    DoAction(ad, &StuffManager::doPickUp);
}

void StuffDrop::execute(const ActionData &ad) const
{
    DoAction(ad, &StuffManager::doDrop);
}


//
// StuffManager
//

StuffPickup StuffManager::pickup_action;
StuffDrop StuffManager::drop_action;

void StuffManager::setStuffBagGraphic(const Graphic *gfx)
{
    stuff_bag_item_type.construct(gfx, 0, 0, 0, 0,  // gfx/overlay
                                  IS_MAGIC, 1, 0,        // size, maxstack, backpack slot
                                  false,               // fragile
                                  0, 0, 0, 0, 0, 0, 0, // melee properties
                                  false, 0, 0, 0,
                                  0,
                                  0, 0, 0,
                                  0, 0,
                                  0, 0, 0, 0, // missile properties
                                  0, false, // keys / traps
                                  0,    // control
                                  &pickup_action,
                                  &drop_action,
                                  0, 0,     // other actions
                                  false,  // allow_str
                                  0,   // tutorial_key
                                  "");  // name
}

void StuffManager::setStuffContents(const MapCoord &mc, const StuffContents &stuff)
{
    stuff_map.erase(mc); // Just in case a "ghost" stuff bag is there already
    stuff_map.insert(make_pair(mc, stuff));
}

const vector<shared_ptr<Item> > * StuffManager::getItems(const MapCoord &mc) const
{
    map<MapCoord, StuffContents>::const_iterator it = stuff_map.find(mc);
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
    
    map<MapCoord, StuffContents>::iterator it = stuff_map.find(mc);
    if (it == stuff_map.end()) {
        // An empty stuff bag. (This shouldn't really happen.)
        return;
    }

    it->second.giveToKnight(*dmap, mc, actor);

    if (it->second.isEmpty()) {
        // Get rid of the stuff bag once and for all
        stuff_map.erase(it);
    } else {
        // Some items still left in the bag (probably the knight didn't have room to carry
        // them all). In this case we place another bag onto the map.
        shared_ptr<Item> new_bag(new Item(getStuffBagItemType()));
        dmap->addItem(mc, new_bag);
    }
}

void StuffManager::doDrop(const MapCoord &mc, shared_ptr<Knight> actor)
{
    // This only runs the drop events. It's assumed that the stuff bag has already been
    // placed into the map.

    if (!actor) return;

    map<MapCoord, StuffContents>::const_iterator it = stuff_map.find(mc);
    if (it == stuff_map.end()) return;

    it->second.runDropEvents(*dmap, mc, actor);
}

void StuffManager::countItems(std::map<const ItemType *, int> &result) const
{
    for (map<MapCoord, StuffContents>::const_iterator stuff_it = stuff_map.begin();
    stuff_it != stuff_map.end();
    ++stuff_it) {

        shared_ptr<Item> item_here = dmap->getItem(stuff_it->first);
        if (!item_here || &item_here->getType() != &getStuffBagItemType()) continue;  // no actual stuff bag here
        
        for (vector<shared_ptr<Item> >::const_iterator item_it = stuff_it->second.getItems().begin();
        item_it != stuff_it->second.getItems().end();
        ++item_it) {
            const ItemType * item_type = &((*item_it)->getType());
            std::map<const ItemType*, int>::iterator result_it = result.find(item_type);
            if (result_it != result.end()) ++ result_it->second;
        }
    }
}

/*
 * stuff_bag.hpp
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

/*
 * Bags of stuff that get dropped when a knight dies
 *
 */

#ifndef STUFF_BAG_HPP
#define STUFF_BAG_HPP

#include "item.hpp"
#include "legacy_action.hpp"

#include "boost/shared_ptr.hpp"
using namespace boost;

#include <map>
#include <vector>
using namespace std;

class DungeonMap;
class Knight;
class MapCoord;


// StuffContents: a thin wrapper over a vector of Items, used to hold
// the contents of a stuff bag.
class StuffContents {
public:
    // Get/Set the list of items
    void addItem(shared_ptr<Item> i);
    const vector<shared_ptr<Item> > & getItems() const { return contents; }

    // remove items from bag and give to kt (as far as poss); also run pickup events:
    void giveToKnight(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Knight> kt);

    // run drop events for the contents of the stuff bag:
    void runDropEvents(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Knight> kt) const;

    // put the contents into the "displaced items" list, also running drop events:
    void putIntoDisplacedItems(DungeonMap &dmap, shared_ptr<Knight> kt);

    // If stuff bag contains only one item, then the following returns that item, else
    // it returns null.
    shared_ptr<Item> getSingleItem() const;

    bool isEmpty() const { return contents.empty(); }
        
private:
    vector<shared_ptr<Item> > contents;
};


// StuffManager:
// 
// Because ItemTypes are stateless, we can't directly store the
// contents of a stuff bag within the stuff bag's Item or ItemType
// objects. (The system just isn't set up for that -- it assumes all
// items of a given type are identical.)
//
// Therefore we set up a StuffManager class which maps MapCoords to
// StuffContents objects.

class StuffManager {
public:
    // initialization -- These should be set before calling the other StuffManager functions.
    void setStuffBagGraphic(lua_State *lua, const Graphic *gfx);
    void setDungeonMap(DungeonMap *dmap_) { dmap = dmap_; }
    
    // Get the ItemType for a stuff bag: (this is unique)
    const ItemType & getStuffBagItemType() const { return *stuff_bag_item_type; }
    
    // Assign a StuffContents to a given MapCoord
    // (it's assumed that a stuff bag item will be put down at that position)
    // This is used by Knight::dropAllItems. 
    void setStuffContents(const MapCoord &mc, const StuffContents &stuff);

    // Get the Items contained within a given stuff bag.
    // (returns NULL if no stuff could be found here.)
    const vector<shared_ptr<Item> > * getItems(const MapCoord &mc) const;
    
    // Pickup and Drop functions for stuff bags. (Only defined for Knights, not general
    // Creatures)

    // doPickUp does two things:
    // (i) runs the onPickUp event for each item type in the stuff bag
    // (ii) adds the items to the knight's inventory.
    // If any items weren't picked up, then a stuff bag containing the remainder will be
    // put back down into the map.
    void doPickUp(const MapCoord &, shared_ptr<Knight> actor);

    // doDrop runs the onDrop event for each item type in the stuff bag.
    void doDrop(const MapCoord &, shared_ptr<Knight> actor);

    // This is used for counting how many items of various types exist in stuff bags
    void countItems(std::map<const ItemType *, int> &result) const;    

    
private:
    DungeonMap *dmap;

    // NOTE: stuff_map may contain "stale" entries for squares where the stuff bag no longer exists.
    // These should be treated as bogus, and ignored.
    map<MapCoord, StuffContents> stuff_map;
    
    auto_ptr<ItemType> stuff_bag_item_type;
};

#endif

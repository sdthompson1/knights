/*
 * item_check_task.cpp
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

#include "dungeon_map.hpp"
#include "item.hpp"
#include "item_check_task.hpp"
#include "knight.hpp"
#include "mediator.hpp"
#include "player.hpp"
#include "stuff_bag.hpp"
#include "task_manager.hpp"
#include "tile.hpp"

ItemCheckTask::ItemCheckTask(DungeonMap &dmap_, int interval_)
    : dmap(dmap_), interval(interval_)
{
    // scan the dungeon for any critical items.
    // (this is the "inverse" of findMissingItems.)
    
    findMissingItems(required_items);

    for (std::map<ItemType *, int>::iterator it = required_items.begin(); it != required_items.end(); ++it) {
        it->second = -(it->second);
    }
}

void ItemCheckTask::execute(TaskManager &tm)
{
    std::map<ItemType *, int> missing_items;
    findMissingItems(missing_items);
    for (std::map<ItemType *, int>::const_iterator it = missing_items.begin(); it != missing_items.end(); ++it) {
        if (it->second > 0) {
            // Add to displaced items list
            boost::shared_ptr<Item> item(new Item(*it->first, it->second));
            dmap.addDisplacedItem(item);
        }
    }
    tm.addTask(shared_from_this(), TP_NORMAL, tm.getGVT() + interval);
}

void ItemCheckTask::processItemType(std::map<ItemType *, int> &missing_items, ItemType &itype, int no)
{
    // we are only interested in critical items.
    if (!itype.isCritical()) return;
    
    std::map<ItemType *, int>::iterator it = missing_items.find(&itype);
    if (it != missing_items.end()) {
        it->second -= no;
    } else {
        missing_items.insert(std::make_pair(&itype, -no));
    }
}    

void ItemCheckTask::processItem(std::map<ItemType *, int> &missing_items, const boost::shared_ptr<Item> &item)
{
    // Wrapper around processItemType.
    if (!item) return;
    processItemType(missing_items, item->getType(), item->getNumber());
}

void ItemCheckTask::findMissingItems(std::map<ItemType *, int> &missing_items) const
{
    Mediator & mediator = Mediator::instance();
    const StuffManager & stuff_manager = mediator.getStuffManager();
    ItemType & stuff_item_type = stuff_manager.getStuffBagItemType();
    
    std::vector<boost::shared_ptr<Tile> > tiles;
    
    missing_items = required_items;
    
    for (int x = 0; x < dmap.getWidth(); ++x) {
        for (int y = 0; y < dmap.getHeight(); ++y) {
            const MapCoord mc(x,y);

            // check for items on the map square
            boost::shared_ptr<Item> item = dmap.getItem(mc);
            processItem(missing_items, item);

            // check for items in stuff bags
            if (item && &item->getType() == &stuff_item_type) {
                const std::vector<boost::shared_ptr<Item> > * stuff_bag = stuff_manager.getItems(&dmap, mc);
                if (stuff_bag) {
                    for (std::vector<boost::shared_ptr<Item> >::const_iterator it = stuff_bag->begin(); it != stuff_bag->end(); ++it) {
                        processItem(missing_items, *it);
                    }
                }
            }

            // check for items in chests etc
            dmap.getTiles(mc, tiles);
            for (std::vector<boost::shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
                processItem(missing_items, (*it)->getPlacedItem());
            }
        }
    }

    // check for items held by knights.
    const std::vector<Player*> & players = mediator.getPlayers();
    for (std::vector<Player*>::const_iterator it = players.begin(); it != players.end(); ++it) {
        boost::shared_ptr<Knight> kt = (*it)->getKnight();
        if (kt) {
            if (kt->getItemInHand()) processItemType(missing_items, *kt->getItemInHand(), 1);
            for (int i = 0; i < kt->getBackpackCount(); ++i) {
                processItemType(missing_items, kt->getBackpackItem(i), kt->getNumCarried(i));
            }
        }
    }

    // check for displaced items.
    // call processItem on each.
    std::vector<DungeonMap::DisplacedItem> & displaced_items = dmap.getDisplacedItems();
    for (std::vector<DungeonMap::DisplacedItem>::iterator it = displaced_items.begin(); it != displaced_items.end(); ++it) {
        processItem(missing_items, it->item);
    }
}

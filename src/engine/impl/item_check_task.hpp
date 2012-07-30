/*
 * item_check_task.hpp
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
 * Trac #33.
 *
 * This task periodically checks whether all "treasure" items are
 * present in the dungeon. If not, it adds them to the dungeonmap for
 * respawning (as "displaced items").
 *
 * NOTE: This duplicates a lot of what is done in ItemRespawnTask so
 * maybe the two classes should be merged together somehow.
 * 
 */

#ifndef ITEM_CHECK_TASK_HPP
#define ITEM_CHECK_TASK_HPP

#include "task.hpp"

#include "boost/shared_ptr.hpp"

#include <map>
#include <vector>

class DungeonMap;
class Item;
class ItemType;

class ItemCheckTask : public Task
{
public:
    // NOTE: Dungeon map should be fully generated before calling the constructor.
    // Only items present initially in the dungeonmap will actually be checked, i.e. we won't
    // respawn anything that wasn't originally in the dungeonmap.
    ItemCheckTask(DungeonMap &dmap_, int interval_);

    virtual void execute(TaskManager &tm);

private:
    static void processItemType(std::map<const ItemType *, int> &missing_items, const ItemType &itype, int no);
    static void processItem(std::map<const ItemType *, int> &missing_items, const boost::shared_ptr<Item> &item);
    void findMissingItems(std::map<const ItemType *, int> &missing_items) const;
    
private:
    DungeonMap &dmap;
    std::map<const ItemType *, int> required_items;
    int interval;
};

#endif

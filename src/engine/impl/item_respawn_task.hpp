/*
 * item_respawn_task.hpp
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
 * Task for respawning items after a time delay.
 *
 * Note that one important function of the item respawning is to put down extra
 * lockpicks. If all keys and lockpicks were placed behind locked doors, then
 * this will (eventually) place extra lockpicks somewhere where the players
 * can reach them.
 *
 */

#ifndef ITEM_RESPAWN_TASK_HPP
#define ITEM_RESPAWN_TASK_HPP

#include "task.hpp"

#include <map>

class ItemType;

class ItemRespawnTask : public Task
{
public:
    ItemRespawnTask(const std::vector<ItemType*> &items_to_respawn,  // not including lockpicks
                    int respawn_delay_,
                    int interval_,
                    ItemType * lockpicks_,
                    int lockpick_init_time_,
                    int lockpick_interval_);

    virtual void execute(TaskManager &tm);

    void removeFromQueue(ItemType &);
    
private:
    void countItems(std::map<ItemType*, int> &result) const;
    int countActiveLockpicks() const;
    
private:
    std::map<ItemType*, int> initial_numbers;   // initial number of each item in the game.
    std::map<ItemType*, int> numbers_in_queue;  // number of each item with active respawn tasks.
    int interval;    // interval at which checks are performed.
    int respawn_delay;    // delay between item disappearing and the respawned item appearing.
    
    ItemType * lockpicks;        // lockpicks are handled separately.
    int lockpick_init_time;      // gvt at which we start respawning lockpicks
    int lockpick_interval;       // interval at which lockpicks are respawned.
    int last_lockpick_time;      // last time at which lockpicks were respawned
    
    bool first_run;
};

#endif

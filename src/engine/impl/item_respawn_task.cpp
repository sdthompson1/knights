/*
 * item_respawn_task.cpp
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
#include "item_respawn_task.hpp"
#include "knight.hpp"
#include "mediator.hpp"
#include "player.hpp"
#include "rng.hpp"
#include "room_map.hpp"
#include "stuff_bag.hpp"
#include "task_manager.hpp"
#include "tile.hpp"

namespace {
    class DoRespawnTask : public Task {
    public:
        DoRespawnTask(ItemType &it, boost::shared_ptr<ItemRespawnTask> rt)
            : itype(it), respawn_task(rt) { }
        void execute(TaskManager&);
    private:
        ItemType &itype;
        boost::shared_ptr<ItemRespawnTask> respawn_task;
    };

    void DoRespawnTask::execute(TaskManager&)
    {
        DungeonMap * dmap = Mediator::instance().getMap().get();
        if (dmap) {
            shared_ptr<Item> item(new Item(itype));
            dmap->addDisplacedItem(item);
        }
        respawn_task->removeFromQueue(itype);
    }
}

ItemRespawnTask::ItemRespawnTask(const std::vector<ItemType*> &items_to_respawn,
                                 int respawn_delay_,
                                 int interval_,
                                 ItemType *lockpicks_,
                                 int lockpick_init_time_,
                                 int lockpick_interval_)
    : interval(interval_),
      respawn_delay(respawn_delay_),
      lockpicks(lockpicks_),
      lockpick_init_time(lockpick_init_time_),
      lockpick_interval(lockpick_interval_),
      last_lockpick_time(0),
      first_run(true)
{
    for (std::vector<ItemType*>::const_iterator it = items_to_respawn.begin();
    it != items_to_respawn.end(); ++it) {
        initial_numbers.insert(std::make_pair(*it, 0));
    }
}

void ItemRespawnTask::countItems(std::map<ItemType*, int> &result) const
{
    // "result" is assumed to be initialized with the desired item types.
    
    const Mediator & mediator = Mediator::instance();

    // Count items held by players
    for (vector<Player*>::const_iterator player_it = mediator.getPlayers().begin();
    player_it != mediator.getPlayers().end(); ++player_it) {
        shared_ptr<Knight> kt((*player_it)->getKnight());
        if (kt) {
            for (std::map<ItemType*, int>::iterator item_it = result.begin();
            item_it != result.end(); ++item_it) {
                ItemType * item_type = item_it->first;
                item_it->second += kt->getNumCarried(*item_type);
                if (kt->getItemInHand() == item_type) ++ item_it->second;
            }
        }
    }

    // Count items stored in stuff bags
    mediator.getStuffManager().countItems(result);

    // Count items in the map
    if (mediator.getMap()) mediator.getMap()->countItems(result);
}

int ItemRespawnTask::countActiveLockpicks() const
{
    int result = 0;
    if (lockpicks) {
        
        const Mediator & mediator = Mediator::instance();

        // Count how many players are holding lockpicks
        for (vector<Player*>::const_iterator player_it = mediator.getPlayers().begin();
        player_it != mediator.getPlayers().end(); ++player_it) {
            shared_ptr<Knight> kt((*player_it)->getKnight());
            if (kt && kt->getNumCarried(*lockpicks) > 0) {
                ++result;
            }
        }

        // Count lockpicks in stuff bags
        std::map<ItemType*, int> n_in_stuff_bags;
        n_in_stuff_bags[lockpicks] = 0;
        mediator.getStuffManager().countItems(n_in_stuff_bags);
        result += n_in_stuff_bags[lockpicks];
    }

    return result;
}

void ItemRespawnTask::execute(TaskManager &tm)
{
    Mediator & mediator = Mediator::instance();
    DungeonMap * dungeon_map = mediator.getMap().get();

    if (dungeon_map) {
    
        if (first_run) {
            // Count the initial number of each item
            countItems(initial_numbers);
            first_run = false;

        } else if (respawn_delay >= 0) {
            // Re-count how many items need to be generated.

            std::map<ItemType*, int> counts;
            for (std::map<ItemType*, int>::const_iterator it = initial_numbers.begin();
            it != initial_numbers.end(); ++it) {
                counts.insert(std::make_pair(it->first, 0));
            }
            
            countItems(counts);
            
            for (std::map<ItemType*, int>::iterator it = counts.begin();
            it != counts.end(); ++it) {

                ItemType * item_type = it->first;
                const int num_in_play = it->second;
                
                // Count the number needed.
                const int num_to_generate =
                    std::max(0, initial_numbers[item_type] - num_in_play - numbers_in_queue[item_type]);

                for (int j = 0; j < num_to_generate; ++j) {
                    boost::shared_ptr<DoRespawnTask> task(new DoRespawnTask(*item_type,
                           boost::static_pointer_cast<ItemRespawnTask>(shared_from_this())));

                    // Divide the respawn delay by the number of players in the game. This ensures faster
                    // respawning when there are many knights (and therefore potions are being drunk more quickly).
                    const int denom = std::max(1, mediator.getNumPlayersRemaining()); //prevent div by zero
                    tm.addTask(task, TP_NORMAL, tm.getGVT() + (respawn_delay / denom));
                    ++numbers_in_queue[item_type];
                }
            }

        }

        
        // Also decide whether we should generate extra lockpicks

        const int num_active_lockpicks = countActiveLockpicks();

        if (lockpicks && num_active_lockpicks < 2 && tm.getGVT() > lockpick_init_time && lockpick_init_time >= 0) {
            const int time_since_last = tm.getGVT() - last_lockpick_time;
            const int time_required = lockpick_interval * (1 + num_active_lockpicks);
            if (time_since_last > time_required) {
                // Generate a lockpick
                shared_ptr<Item> item(new Item(*lockpicks));
                dungeon_map->addDisplacedItem(item);
                // Reset the timer for the next lockpick
                last_lockpick_time = tm.getGVT();
            }
        }
    }

    // re-schedule after "interval".
    tm.addTask(shared_from_this(), TP_NORMAL, tm.getGVT() + interval);
}

void ItemRespawnTask::removeFromQueue(ItemType &it)
{
    int & num = numbers_in_queue[&it];
    ASSERT(num > 0);
    num = std::max(num-1, 0);
}

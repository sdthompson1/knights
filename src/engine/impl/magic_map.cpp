/*
 * magic_map.cpp
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

#include "dispel_magic.hpp"
#include "dungeon_map.hpp"
#include "item.hpp"
#include "knight.hpp"
#include "magic_map.hpp"
#include "mediator.hpp"
#include "mini_map.hpp"
#include "player.hpp"
#include "room_map.hpp"
#include "stuff_bag.hpp"
#include "task_manager.hpp"
#include "tile.hpp"

void MagicMapping(shared_ptr<Knight> kt)
{
    if (!kt || !kt->getMap()) return;
    const DungeonMap &dmap(*kt->getMap());

    vector<shared_ptr<Tile> > tiles;
    vector<char> is_wall;
    is_wall.reserve(dmap.getWidth() * dmap.getHeight());

    for (int y=0; y<dmap.getHeight(); ++y) {
        for (int x=0; x<dmap.getWidth(); ++x) {
            MapCoord mc(x,y);
            dmap.getTiles(mc, tiles);
            bool flag = false;
            for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end();
            ++it) {
                if ((*it)->getColour() == COL_WALL) {
                    flag = true;
                    break;
                }
            }
            is_wall.push_back(flag);
        }
    }

    for (int y=0; y<dmap.getHeight(); ++y) {
        for (int x=0; x<dmap.getWidth(); ++x) {
            MapCoord mc(x,y);

            const bool wall_here = (is_wall[y*dmap.getWidth() + x] != 0);

            // If a wall is surrounded by 8 other walls then we don't draw it.
            // This prevents ugly blocks of colour for those segments that have large
            // areas of solid wall.

            int wallcount = 0;
            for (int yy = y-1; yy <= y+1; ++yy) {
                for (int xx = x-1; xx <= x+1; ++xx) {
                    if (xx < 0 || xx >= dmap.getWidth() || yy < 0 || yy >= dmap.getHeight()
                    || is_wall[yy*dmap.getWidth() + xx]) {
                        ++wallcount;
                    }
                }
            }
            if (wall_here && wallcount < 9) {
                kt->getPlayer()->setMiniMapColour(mc.getX(), mc.getY(), COL_WALL);
            }
        }
    }           
}

void WipeMap(shared_ptr<Knight> kt)
{
    if (!kt) return;
    kt->getPlayer()->wipeMap();
}


//
// Sense Items
//

//
// Note that sense items is implemented slightly differently from sense knight (which
// uses a Knight member function, Knight::setSenseKnight).
// This was for no particular reason... I just thought it would be better not to
// clutter Knight up with any additional member functions.
//

class SenseItemsTask : public Task, public DispelObserver {
public:
    SenseItemsTask(Player &pl, int expiry_);
    virtual void execute(TaskManager &tm);
    virtual void onDispel(shared_ptr<Knight> kt) { clearAllHighlights(); }
private:
    bool interestingItemAt(const DungeonMap &dmap, const MapCoord &mc,
                           const Player &pl) const;
    void setHighlight(const MapCoord &mc, bool on);
    void clearAllHighlights();
private:
    vector<MapCoord> items;
    Player &player;
    int expiry;
};

void SenseItems(shared_ptr<Knight> kt, int stop_time)
{
    if (!kt || !kt->getMap()) return;
    shared_ptr<SenseItemsTask> task(new SenseItemsTask(*kt->getPlayer(), stop_time));
    kt->addDispelObserver(task);
    TaskManager &tm(Mediator::instance().getTaskManager());
    tm.addTask(task, TP_NORMAL, tm.getGVT() + 1);
}

SenseItemsTask::SenseItemsTask(Player &pl, int expiry_)
    : player(pl), expiry(expiry_)
{
    if (!player.getKnight()) return;
    DungeonMap *dmap = player.getKnight()->getMap();
    if (dmap && dmap->getRoomMap()) {
        for (int x=0; x<dmap->getWidth(); ++x) {
            for (int y=0; y<dmap->getHeight(); ++y) {
                const MapCoord mc(x,y);
                if (interestingItemAt(*dmap, mc, pl)) {
                    // Item locations are only revealed for rooms that you've already
                    // mapped. If we were to reveal the location of all items, even those in
                    // not-yet-visited rooms, then that would unbalance the game, since it
                    // would be a huge advantage for certain quests (eg quest for gems).

                    // Note that an alternative way of implementing this would have been
                    // to *always* send the "setHighlight" message, but to filter out
                    // unmapped squares on the *client*. However, that's a bad idea because
                    // someone could cheat, by hacking the client program to remove that
                    // filtering step (hence they would find out the locations of all the gems
                    // which would be an unfair advantage to them). We want to prevent
                    // cheating whenever we can.

                    int r1, r2;
                    dmap->getRoomMap()->getRoomAtPos(mc, r1, r2);
                    if ((r1 != -1 && pl.isRoomMapped(r1)) || (r2 != -1 && pl.isRoomMapped(r2))) {
                        items.push_back( mc );
                        setHighlight(mc, true);
                    }
                }
            }
        }
    }
}

void SenseItemsTask::execute(TaskManager &tm)
{
    // Check whether time has run out, or player has died
    if (tm.getGVT() > expiry || !player.getKnight() || !player.getKnight()->getMap()) {
        clearAllHighlights();
        return;
    }

    // Check each mapcoord in turn, to see if an item is still there. If not,
    // then delete the appropriate highlight.
    int i = 0;
    while (i < items.size()) {
        if (!interestingItemAt(*player.getKnight()->getMap(), items[i], player)) {
            // The item was picked up. Switch off that highlight.
            setHighlight(items[i], false);
            items.erase(items.begin() + i);
        } else {
            ++i;
        }
    }

    // Reschedule
    tm.addTask(shared_from_this(), TP_NORMAL, Mediator::instance().cfgInt("player_task_interval") + tm.getGVT());
}

bool SenseItemsTask::interestingItemAt(const DungeonMap &dmap, const MapCoord &mc,
                                       const Player &pl) const
{
    // Check raw item
    shared_ptr<Item> item = dmap.getItem(mc);
    if (item && pl.isItemInteresting(item->getType())) return true;

    // Check for items within stuff bags
    const StuffManager &stuff(Mediator::instance().getStuffManager());
    if (item && &item->getType() == &stuff.getStuffBagItemType()) {
        const vector<shared_ptr<Item> > * items = stuff.getItems(mc);
        if (items) {
            for (vector<shared_ptr<Item> >::const_iterator it = items->begin();
            it != items->end(); ++it) {
                if (pl.isItemInteresting((*it)->getType())) return true;
            }
        }
    }

    // Check items hidden away within tiles (eg Chests)
    vector<shared_ptr<Tile> > tiles;
    dmap.getTiles(mc, tiles);
    for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end(); ++it) {
        item = (*it)->getPlacedItem();
        if (item && pl.isItemInteresting(item->getType())) return true;
    }

    // Nothing found.
    return false;
}

void SenseItemsTask::setHighlight(const MapCoord &mc, bool on)
{
    player.mapItemLocation(mc.getX(), mc.getY(), on);
}

void SenseItemsTask::clearAllHighlights()
{
    for (int i=0; i<items.size(); ++i) {
        player.mapItemLocation(items[i].getX(), items[i].getY(), false);
    }
    items.clear();
}


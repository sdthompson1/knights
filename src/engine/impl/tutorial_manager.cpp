/*
 * tutorial_manager.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2024.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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
#include "knight.hpp"
#include "knights_callbacks.hpp"
#include "lockable.hpp"
#include "mediator.hpp"
#include "player.hpp"
#include "room_map.hpp"
#include "tile.hpp"
#include "tutorial_manager.hpp"

namespace {

    // These constants must agree with the tutorial.txt file
    const int T_KEY_ITEM = 1;
    const int T_KEY_DEATH = 2;
    const int T_KEY_GEM = 3;
    const int T_KEY_TWO_GEMS = 4;
    const int T_KEY_THREE_GEMS = 5;
    
    int Distance(const MapCoord &mc1, const MapCoord &mc2)
    {
        if (mc1.isNull() || mc2.isNull()) return 99999;
        return std::abs(mc1.getX() - mc2.getX()) + std::abs(mc1.getY() - mc2.getY());
    }

    struct CompareDepth {
        bool operator()(const boost::shared_ptr<Tile> &lhs, const boost::shared_ptr<Tile> &rhs) const
        {
            return lhs->getDepth() > rhs->getDepth(); // reverse order (highest depth first)
        }
    };
}

void TutorialManager::addTutorialKey(int key, const std::string &title, const std::string &msg)
{
    titles[key] = title;
    messages[key] = msg;
}

void TutorialManager::onMoveKnight(const Player &player)
{
    const DungeonMap *dmap = player.getDungeonMap();
    const RoomMap *rmap = player.getRoomMap();
    const int curr_room = player.getCurrentRoom();
    const MapCoord kt_pos = player.getKnightPos();

    TutorialWindow win;
    int nearest_dist = 999999;
    int nearest_key = 0;
    MapCoord nearest_pos;
    
    if (dmap) {

        // Get current room position
        MapCoord top_left;
        int width = 0, height = 0;
        if (rmap) rmap->getRoomLocation(curr_room, top_left, width, height);

        // Search tiles in this room for triggers. Find the nearest.
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const MapCoord mc(top_left.getX() + x, top_left.getY() + y);

                // Tiles
                std::vector<boost::shared_ptr<Tile> > tiles;
                dmap->getTiles(mc, tiles);
                std::sort(tiles.begin(), tiles.end(), CompareDepth());
                for (std::vector<boost::shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
                    const int t_key = (*it)->getTutorialKey();
                    handleTutorial(mc, kt_pos, t_key, tiles, 0, win, nearest_dist, nearest_key, nearest_pos);
                }

                // Handle item if there is one
                boost::shared_ptr<Item> item = dmap->getItem(mc);
                if (item) {
                    // Tutorial key "T_KEY_ITEM" is hard coded to items
                    handleTutorial(mc, kt_pos, T_KEY_ITEM, tiles, item.get(), win, nearest_dist, nearest_key, nearest_pos);
                }
            }
        }
    }

    update(kt_pos, curr_room, nearest_key, nearest_pos, win, false);
}

void TutorialManager::handleTutorial(const MapCoord &mc, const MapCoord &kt_pos,
                                     int t_key, const std::vector<boost::shared_ptr<Tile> > &tiles,
                                     const Item *item,
                                     TutorialWindow &win, int &nearest_dist, int &nearest_key, MapCoord &nearest_pos)
{
    if (t_key > 0 && known_triggers.find(t_key) == known_triggers.end()) {

        // for 'door' tiles we only activate the trigger if the door is closed.
        // this prevents the portcullis message appearing for open portcullises.
        bool allow = true;
        for (std::vector<boost::shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
            const Lockable* lockable = dynamic_cast<Lockable*>(it->get());
            if (lockable && !lockable->isClosed()) {
                allow = false;
                break;
            }
        }

        const int dist = Distance(mc, kt_pos);
        allow = allow && (dist < nearest_dist);
        
        if (allow) {
            nearest_dist = dist;
            nearest_key = t_key;
            nearest_pos = mc;
            
            setupTutorialWindow(t_key, win);
            setupTileGraphics(tiles, item, win);
        }
    }
}

void TutorialManager::setupTileGraphics(const std::vector<boost::shared_ptr<Tile> > &tiles, const Item *item, TutorialWindow &win)
{
    // Set graphics for this tile
    for (std::vector<boost::shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
        if ((*it)->getGraphic()) {
            win.gfx.push_back((*it)->getGraphic());
            const ColourChange *cc = (*it)->getColourChange().get();
            win.cc.push_back(cc ? *cc : ColourChange());
        }
    }
    
    if (item) {
        win.gfx.push_back(item->getGraphic());
        win.cc.push_back(ColourChange());
    }
}

void TutorialManager::onPickup(const Player &player, const ItemType &itype)
{
    int t_key = itype.getTutorialKey();

    // special case for 2nd and 3rd gem picked up
    if (t_key == T_KEY_GEM && player.getKnight()) {
        const int ngems = player.getKnight()->getNumCarried(itype);
        if (ngems == 2) t_key = T_KEY_TWO_GEMS;
        else if (ngems == 3) t_key = T_KEY_THREE_GEMS;
    }
    
    if (t_key > 0 && known_triggers.find(t_key) == known_triggers.end()) {
        TutorialWindow win;
        setupTutorialWindow(t_key, win);
        win.gfx.push_back(itype.getBackpackGraphic());
        if (!win.gfx[0]) win.gfx[0] = itype.getSingleGraphic();
        win.cc.push_back(ColourChange());
        update(player.getKnightPos(), player.getCurrentRoom(), t_key, player.getKnightPos(), win, true);
    }
}

void TutorialManager::onOpenLockable(const Player &player, const MapCoord &mc)
{
    // This is to trap the case where the knight opens a chest and discovers his first item
    if (known_triggers.find(T_KEY_ITEM) == known_triggers.end()) {
        if (Distance(player.getKnightPos(), mc) <= 1) {
            
            // See if there is an item on the opened square
            const DungeonMap *dmap = player.getDungeonMap();
            if (dmap) {
                boost::shared_ptr<Item> item = dmap->getItem(mc);
                if (item) {
                    // Clear the current trigger -- this will force the item to appear
                    clearCurrentTrigger();
                    // Make the Item tutorial message appear.
                    TutorialWindow win;
                    setupTutorialWindowForSquare(T_KEY_ITEM, *dmap, mc, win);
                    update(player.getKnightPos(), player.getCurrentRoom(), T_KEY_ITEM, mc, win, false);
                }
            }
        }
    }
}

void TutorialManager::onKnightDeath(const Player &player, const DungeonMap &dmap, const MapCoord &mc)
{
    if (known_triggers.find(T_KEY_DEATH) == known_triggers.end()) {
        TutorialWindow win;
        setupTutorialWindowForSquare(T_KEY_DEATH, dmap, mc, win);

        const MapCoord & home_location = player.getHomeLocation();
        int room = -1, dummy;
        const RoomMap * room_map = dmap.getRoomMap();
        if (room_map) room_map->getRoomAtPos(home_location, room, dummy);

        // set 'pickup' flag, this will make sure the message gets displayed
        update(home_location, room, T_KEY_DEATH, mc, win, true);
    }
}

void TutorialManager::setupTutorialWindowForSquare(int key, const DungeonMap &dmap, const MapCoord &mc, TutorialWindow &win)
{
    setupTutorialWindow(key, win);
    std::vector<boost::shared_ptr<Tile> > tiles;
    dmap.getTiles(mc, tiles);
    std::sort(tiles.begin(), tiles.end(), CompareDepth());
    boost::shared_ptr<Item> item(dmap.getItem(mc));
    setupTileGraphics(tiles, item.get(), win);
}

void TutorialManager::clearCurrentTrigger()
{
    current_window_room = -999;
    current_window_pos = MapCoord();
    curr_is_pickup = false;
}

void TutorialManager::setupTutorialWindow(int key, TutorialWindow &win)
{
    win.title_latin1 = titles[key];
    win.msg_latin1 = messages[key];
    win.popup = false;
    win.gfx.clear();
    win.cc.clear();
}

void TutorialManager::update(const MapCoord &knight_pos, int knight_room,
                             int trigger, const MapCoord &trigger_pos,
                             TutorialWindow window,
                             bool is_pickup)
{
    const int TUTORIAL_RANGE = 5;
    
    bool replace_current = false;  // Should we replace the current window?
    bool use_new = false;         // Can we use the new window?

    // Reset the current trigger if the knight has moved to a new room
    if (knight_room != current_window_room && knight_room >= 0) clearCurrentTrigger();
    
    // Calculate how far away the current trigger is
    int dist_to_curr_trigger = Distance(current_window_pos, knight_pos);

    // Replace window if the knight has gone out of range of the current trigger
    if (dist_to_curr_trigger > TUTORIAL_RANGE) replace_current = true;

    if (trigger > 0) {
        // There is a new trigger that we could use.
        const int dist_to_new_trigger = Distance(trigger_pos, knight_pos);

        // To accept a trigger, we must have either:
        // - this is a pickup (or similar e.g. death), or
        // - the trigger is in range and either:
        //   - the new trigger is closer than the old one, or
        //   - the old trigger was a 'pickup' and the knight has moved away from the trigger square.
        if (is_pickup ||
               (dist_to_new_trigger <= TUTORIAL_RANGE && (dist_to_new_trigger < dist_to_curr_trigger
                                                         || (curr_is_pickup && dist_to_curr_trigger > 0)))) {
            replace_current = true;
            use_new = true;
        }
    }
    
    if (replace_current) {
        if (use_new) {
            current_window_room = knight_room;
            current_window_pos = trigger_pos;
            curr_is_pickup = is_pickup;
            known_triggers.insert(trigger);
            Mediator::instance().getCallbacks().popUpWindow(
                std::vector<TutorialWindow>(1, window));
        } else {
            clearCurrentTrigger();
        }

        
    }
}

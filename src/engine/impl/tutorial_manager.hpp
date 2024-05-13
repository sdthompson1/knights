/*
 * tutorial_manager.hpp
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

/*
 * Manages the tutorial windows that are shown to the player.
 *
 */

#ifndef TUTORIAL_MANAGER_HPP
#define TUTORIAL_MANAGER_HPP

#include "map_support.hpp"
#include "tutorial_window.hpp"

#include <map>
#include <set>
#include <string>

class ItemType;
class Player;

class TutorialManager {
public:
    explicit TutorialManager() : current_window_room(-999), sent_first_msg(false),
                                 curr_is_pickup(false) { }

    void addTutorialKey(int key, const std::string &title, const std::string &msg);

    void onMoveKnight(const Player &player);
    void onPickup(const Player &player, const ItemType &itype);
    void onOpenLockable(const Player &player, const MapCoord &mc);
    void onKnightDeath(const Player &player, const DungeonMap &dmap, const MapCoord &mc);
    
private:
    void clearCurrentTrigger();
    void setupTutorialWindowForSquare(int key, const DungeonMap &dmap, const MapCoord &mc, TutorialWindow &win);
    void setupTutorialWindow(int key, TutorialWindow &win);
    void setupTileGraphics(const std::vector<boost::shared_ptr<Tile> > &tiles, const Item *item, TutorialWindow &win);
    void handleTutorial(const MapCoord &mc, const MapCoord &kt_pos,
                        int t_key, const std::vector<boost::shared_ptr<Tile> > &tiles,
                        const Item *item,
                        TutorialWindow &win, int &nearest_dist, int &nearest_key, MapCoord &nearest_pos);
    void update(const MapCoord &position, int room,  // knight location
                int trigger,                      // which tutorial code (or <= 0 for none)
                const MapCoord &trigger_pos,      // trigger location. ignored if trigger <= 0.
                TutorialWindow window,    // the window for this trigger. ignored if trigger <= 0.
                bool is_pickup);              // true if the trigger was picking up an item.

private:
    MapCoord current_window_pos;
    int current_window_room;
    std::set<int> known_triggers;
    std::map<int, std::string> titles, messages;   // latin1 (for now!)
    bool sent_first_msg;
    bool curr_is_pickup;
};

#endif

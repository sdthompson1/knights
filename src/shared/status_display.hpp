/*
 * status_display.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2025.
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
 * Interface for updates to a status display (the area that appears
 * below the main dungeon view, but not including the mini-map).
 *
 */

#ifndef STATUS_DISPLAY_HPP
#define STATUS_DISPLAY_HPP

#include "localization.hpp"
#include "potion_magic.hpp"

#include <string>
#include <vector>

class Graphic;

class StatusDisplay {
public:
    virtual ~StatusDisplay() { }

    //
    // Backpack
    // 
    // The game engine tells us a slot number, graphic to display in
    // that slot (from item_type->backpack_graphic), how many the
    // player is carrying, and the maximum that the player is allowed
    // to carry (or 0 for no maximum).
    // 
    // Slot conventions:
    // 11,12,... normal items.
    // 23 for the lock picks, and 20,21,22 for keys (one max of each).
    // 30 for gems.
    //
    virtual void setBackpack(int slot, const Graphic *gfx, const Graphic *overdraw,
                             int no_carried, int no_max) = 0;
    
    // Add a death (skull)
    virtual void addSkull() = 0;

    // Set health and potion-magic (controls colour of health bottle)
    virtual void setHealth(int h) = 0;
    virtual void setPotionMagic(PotionMagic pm, bool poison_immunity) = 0;
    
    virtual void setQuestHints(const std::vector<LocalMsg> &quest_hints) = 0;
};

#endif

/*
 * quest.hpp
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

#ifndef QUEST_HPP
#define QUEST_HPP

#include "quest_circumstance.hpp"
#include "status_display.hpp"

#include <map>
#include <string>
#include <vector>
using namespace std;

class ItemType;
class Knight;

class Quest {
public:
    virtual ~Quest() { }

    // Check is called in two circumstances:
    // (i)  The knight has just whacked the special pentagram square. (See A_CheckQuest.)
    // (ii) The knight has just approached his own exit point. (See Home::onApproach.)
    virtual bool check(Knight &kt) const = 0;

    // getHint() is called when we approach the exit point but check() returns false.
    // The hint text is flashed up onto the screen.
    virtual string getHint() const { return string(); }

    // determine whether this quest is interested in a certain item type
    // (used for "Sense Items").
    virtual bool isItemInteresting(const ItemType &) const = 0;

    // This should get all required quest items. Used by ItemCheckTask.
    // The map is from itemtype to number of those items required.
    // Note: If an itemtype is already present in the map then this class should take the MAX
    // of the existing number in the map and the number required for this quest.
    virtual void getRequiredItems(std::map<const ItemType *, int> &required_items) const = 0;
    
    // Get the QuestIcon (i.e. "quest requirements" display) for this quest
    virtual StatusDisplay::QuestIcon getQuestIcon(const Knight &kt, QuestCircumstance c) const = 0;
};

#endif

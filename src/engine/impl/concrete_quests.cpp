/*
 * quests.cpp
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
 * Definitions of the concrete Quest objects.
 *
 */

#include "misc.hpp"

#include "concrete_quests.hpp"
#include "dungeon_map.hpp"
#include "item.hpp"
#include "knight.hpp"
#include "mediator.hpp"
#include "quest.hpp"

#include <sstream>
using namespace std;


//
// QuestRetrieve
//

bool QuestRetrieve::check(Knight &kt) const
{
    if (find(itypes.begin(), itypes.end(), kt.getItemInHand()) != itypes.end()) return true;
    for (int i=0; i<kt.getBackpackCount(); ++i) {
        if (find(itypes.begin(), itypes.end(), &kt.getBackpackItem(i)) != itypes.end()
            && kt.getNumCarried(i) >= no) return true;
    }
    return false;
}

string QuestRetrieve::getHint() const
{
    if (no == 1) {
        return singular + " " + Mediator::instance().cfgString("required_msg");
    } else {
        ostringstream s;
        s << no;
        return s.str() + " " + plural + " " + Mediator::instance().cfgString("required_msg");
    }
}

bool QuestRetrieve::isItemInteresting(const ItemType &itype) const
{
    return (find(itypes.begin(), itypes.end(), &itype) != itypes.end());
}

void QuestRetrieve::getRequiredItems(std::map<const ItemType *, int> & required_items) const
{
    for (std::vector<const ItemType*>::const_iterator it = itypes.begin(); it != itypes.end(); ++it) {
        required_items[*it] = std::max(required_items[*it], no);
    }
}

StatusDisplay::QuestIcon QuestRetrieve::getQuestIcon(const Knight &kt, QuestCircumstance c) const
{
    StatusDisplay::QuestIcon qi;

    int num_held = 0;
    
    if (find(itypes.begin(), itypes.end(), kt.getItemInHand()) != itypes.end()) {
        num_held = 1;
    } else {
        for (int i = 0; i < kt.getBackpackCount(); ++i) {
            if (find(itypes.begin(), itypes.end(), &kt.getBackpackItem(i)) != itypes.end()) {
                num_held = kt.getNumCarried(i);
                break;
            }
        }
    }

    std::ostringstream str;
    str << "Retrieve ";

    // Parse the item name, it should be of form "A gem" or "The book".
    const std::string & name = itypes.front()->getName();
    const bool is_big = itypes.front()->isBig();
    if (name[0] == 'A') {
        if (is_big) {
            // TODO: In quests with multiple wands we should probably 
            // say "Retrieve a wand" rather than "Retrieve the wand"?
            str << "the " << name.substr(2);   // "the wand"
        } else {
            str << no << ' ' << name.substr(2);     // "<N> gem"
            if (no > 1) str << 's';                 // add "s" if plural
        }
    } else {
        str << 't';
        str << name.substr(1);   // "the book"
    }

    qi.sort = is_big ? 10 : 20;
    qi.msg = str.str();
    qi.complete = (num_held >= no);
    
    return qi;
}


//
// QuestDestroy
//

bool QuestDestroy::check(Knight &kt) const
{
    // check wand
    if (find(wand.begin(), wand.end(), kt.getItemInHand()) == wand.end()) return false;

    // check book
    if (!kt.getMap()) return false;    
    MapCoord mc = DisplaceCoord(kt.getPos(), kt.getFacing());
    shared_ptr<Item> it = kt.getMap()->getItem(mc);
    if (!it) return false;
    return (find(book.begin(), book.end(), &it->getType()) != book.end());
}

bool QuestDestroy::isItemInteresting(const ItemType &itype) const
{
    return find(book.begin(), book.end(), &itype) != book.end()
        || find(wand.begin(), wand.end(), &itype) != wand.end();
}

void QuestDestroy::getRequiredItems(std::map<const ItemType *, int> &required_items) const
{
    for (std::vector<const ItemType *>::const_iterator it = book.begin(); it != book.end(); ++it) {
        required_items[*it] = std::max(required_items[*it], 1);
    }
    for (std::vector<const ItemType *>::const_iterator it = wand.begin(); it != wand.end(); ++it) {
        required_items[*it] = std::max(required_items[*it], 1);
    }
}

StatusDisplay::QuestIcon QuestDestroy::getQuestIcon(const Knight &kt, QuestCircumstance c) const
{
    StatusDisplay::QuestIcon qi;
    qi.sort = 110;
    qi.msg = "Strike the book with the wand";
    qi.complete = (c == WIN_FROM_COMPLETE_QUEST);
    return qi;
}

StatusDisplay::QuestIcon QuestGeneric::getQuestIcon(const Knight &kt, QuestCircumstance c) const
{
    StatusDisplay::QuestIcon qi;
    qi.sort = sort;
    qi.msg = msg;
    qi.complete = (c == WIN_FROM_COMPLETE_QUEST);
    return qi;
}

StatusDisplay::QuestIcon QuestSecure::getQuestIcon(const Knight &kt, QuestCircumstance c) const
{
    StatusDisplay::QuestIcon qi;
    qi.sort = 1010;
    qi.msg = "Secure all entry points using the Wand of Securing";
    qi.complete = false; // TODO
    return qi;
}

StatusDisplay::QuestIcon QuestDestroyKnights::getQuestIcon(const Knight &kt, QuestCircumstance c) const
{
    StatusDisplay::QuestIcon qi;
    qi.sort = 1020;
    qi.msg = "Destroy all enemy knights";
    qi.complete = (c == WIN_FROM_KILL_KNIGHTS);
    return qi;
}

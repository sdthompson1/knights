/*
 * quests.hpp
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
 * The concrete Quest classes.
 *
 */

#ifndef QUESTS_HPP
#define QUESTS_HPP

#include "quest.hpp"

#include <vector>
using namespace std;

class QuestRetrieve : public Quest {
public:
    // This checks whether the knight is holding (at least) the given number of the
    // given item type. (Any one of the item types on the list are acceptable.)
    QuestRetrieve(int n, const vector<const ItemType*> &i,
                  const string &sing, const string &pl)
        : no(n), itypes(i), singular(sing), plural(pl) { }
    virtual bool check(Knight &) const;
    virtual string getHint() const;
    virtual bool isItemInteresting(const ItemType &itype) const;
    virtual void getRequiredItems(std::map<const ItemType *, int> & required_items) const;
    virtual StatusDisplay::QuestIcon getQuestIcon(const Knight &kt, QuestCircumstance c) const;
private:
    int no;
    const vector<const ItemType*> itypes;
    string singular, plural;
};

class QuestDestroy : public Quest {
public:
    // This checks whether the knight is holding one of the given wands, and one of the given
    // books is in the square ahead. (This is intended to be called from "on_hit" on the
    // special pentagram tile.)
    QuestDestroy(const vector<const ItemType*> &book_,
                 const vector<const ItemType*> &wand_)
        : book(book_), wand(wand_) { }
    virtual bool check(Knight &) const;
    virtual bool isItemInteresting(const ItemType &itype) const;
    virtual void getRequiredItems(std::map<const ItemType *, int> & required_items) const;    
    virtual StatusDisplay::QuestIcon getQuestIcon(const Knight &kt, QuestCircumstance c) const;
private:
    vector<const ItemType*> book;
    vector<const ItemType*> wand;
};

class QuestGeneric : public Quest {
public:
    // Displays a generic Quest Requirement message, that is only fulfilled when
    // the player wins the quest.
    QuestGeneric(const std::string &msg_, int sort_) : msg(msg_), sort(sort_) { }
    virtual bool check(Knight &) const { return true; }
    virtual bool isItemInteresting(const ItemType &itype) const { return false; }
    virtual void getRequiredItems(std::map<const ItemType *, int> &) const { }
    virtual StatusDisplay::QuestIcon getQuestIcon(const Knight &kt, QuestCircumstance c) const;
private:
    std::string msg;
    int sort;
};

class QuestSecure : public Quest {
public:
    // Displays the "Secure all entry points" message
    virtual bool check(Knight &) const { return true; }
    virtual bool isItemInteresting(const ItemType &itype) const { return false; }
    virtual void getRequiredItems(std::map<const ItemType *, int> &) const { }
    virtual StatusDisplay::QuestIcon getQuestIcon(const Knight &kt, QuestCircumstance c) const;
};

class QuestDestroyKnights : public Quest {
public:
    // Displays the "destroy all enemy knights" message
    virtual bool check(Knight &) const { return true; }
    virtual bool isItemInteresting(const ItemType &itype) const { return false; }
    virtual void getRequiredItems(std::map<const ItemType *, int> &) const { }
    virtual StatusDisplay::QuestIcon getQuestIcon(const Knight &kt, QuestCircumstance c) const;
};
    
#endif

/*
 * create_quest.cpp
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

#include "concrete_quests.hpp"
#include "create_quest.hpp"
#include "knights_config_impl.hpp"

#include "kfile.hpp"
using namespace KConfig;

using namespace boost;
using namespace std;

namespace {
    void PopItemList(KnightsConfigImpl &kc, vector<const ItemType *> &output)
    {
        output.clear();
        KFile::List lst(*kc.getKFile(), "list of items");
        for (int i=0; i<lst.getSize(); ++i) {
            lst.push(i);
            const ItemType *it = kc.popItemType();
            if (!it) kc.getKFile()->errExpected("item type");
            output.push_back(it);
        }
    }
}

void CreateQuests(const string &name, KnightsConfigImpl &kc,
                  std::vector<boost::shared_ptr<Quest> > &output)
{
    if (name == "QuestRetrieve") {
        KFile::List lst(*kc.getKFile(), "", 3, 4);
        lst.push(0);
        int n = kc.getKFile()->popInt();
        lst.push(1);
        vector<const ItemType *> items;
        PopItemList(kc, items);
        lst.push(2);
        string sing = kc.getKFile()->popString();
        lst.push(3);
        string pl = kc.getKFile()->popString(sing + "S");

        shared_ptr<Quest> q(new QuestRetrieve(n, items, sing, pl));
        output.push_back(q);

    } else if (name == "QuestDestroy") {
        vector<const ItemType *> book, wand;
        KFile::List lst(*kc.getKFile(), "", 2);
        lst.push(0);
        PopItemList(kc, book);
        lst.push(1);
        PopItemList(kc, wand);

        shared_ptr<Quest> q(new QuestGeneric("Place the book on the special pentagram", 100));
        output.push_back(q);
        q.reset(new QuestDestroy(book, wand));
        output.push_back(q);

    } else if (name == "QuestSecure") {
        
        shared_ptr<Quest> q(new QuestSecure);
        output.push_back(q);
        q.reset(new QuestDestroyKnights);
        output.push_back(q);

    } else {
        kc.getKFile()->errExpected("MenuDirective");
    }
}

void CreateEscapeQuest(ExitType e,
                       std::vector<boost::shared_ptr<Quest> > &output)
{
    boost::shared_ptr<Quest> escape_quest;
    switch (e) {
    case E_SELF:
        escape_quest.reset(new QuestGeneric("Escape via your entry point", 120));
        break;
    case E_OTHER:
        escape_quest.reset(new QuestGeneric("Escape via your opponent's entry point", 120));
        break;
    case E_RANDOM:
        escape_quest.reset(new QuestGeneric("Escape via the random exit point", 120));
        break;
    case E_SPECIAL:
        escape_quest.reset(new QuestGeneric("Escape via the guarded exit", 120));
        break;
    }
    if (escape_quest) output.push_back(escape_quest);
}

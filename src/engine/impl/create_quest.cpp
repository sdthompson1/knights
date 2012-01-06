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

shared_ptr<Quest> CreateQuest(const string &name, KnightsConfigImpl &kc)
{
    shared_ptr<Quest> result;
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
        result.reset(new QuestRetrieve(n, items, sing, pl));

    } else if (name == "QuestDestroy") {
        vector<const ItemType *> book, wand;
        KFile::List lst(*kc.getKFile(), "", 2);
        lst.push(0);
        PopItemList(kc, book);
        lst.push(1);
        PopItemList(kc, wand);
        result.reset(new QuestDestroy(book, wand));

    } else {
        kc.getKFile()->errExpected("MenuDirective");
    }

    return result;
}

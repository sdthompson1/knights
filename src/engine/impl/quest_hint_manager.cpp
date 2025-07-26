/*
 * quest_hint_manager.cpp
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

#include "misc.hpp"

#include "quest_hint_manager.hpp"
#include "status_display.hpp"

#include <algorithm>

bool QuestHintManager::QuestHint::operator<(const QuestHintManager::QuestHint &rhs) const
{
    return group < rhs.group ? true
        : group > rhs.group ? false
        : order < rhs.order ? true
        : order > rhs.order ? false
        : msg < rhs.msg;
}

void QuestHintManager::addHint(const std::string &msg, double order, double group)
{
    QuestHint i;
    i.msg = msg;
    i.order = order;
    i.group = group;
    quest_hints.push_back(i);
}

void QuestHintManager::clearHints()
{
    quest_hints.clear();
}

void QuestHintManager::sendHints(StatusDisplay &status_display)
{
    // Sort QuestHints into order
    std::sort(quest_hints.begin(), quest_hints.end());

    // Work out the "Quest Requirements" strings
    std::vector<std::string> quest_rqmts;
    for (std::vector<QuestHint>::const_iterator it = quest_hints.begin(); it != quest_hints.end(); ++it) {
        if (it != quest_hints.begin() && it->group != (it-1)->group) {
            quest_rqmts.push_back("");
            quest_rqmts.push_back("--- OR ---");
            quest_rqmts.push_back("");
        }
        quest_rqmts.push_back(it->msg);
    }

    // Now send them out
    status_display.setQuestHints(quest_rqmts);
}

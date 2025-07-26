/*
 * quest_hint_manager.hpp
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

#ifndef QUEST_HINT_MANAGER_HPP
#define QUEST_HINT_MANAGER_HPP

class StatusDisplay;

#include <string>
#include <vector>

class QuestHintManager {
public:
    void addHint(const std::string &msg, double order, double group);
    void clearHints();
    void sendHints(StatusDisplay &sd);

private:
    struct QuestHint {
        std::string msg;
        double order;
        double group;
        
        bool operator<(const QuestHint &other) const;
    };

    std::vector<QuestHint> quest_hints;
};

#endif

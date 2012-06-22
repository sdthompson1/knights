/*
 * quest_circumstance.hpp
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

#ifndef QUEST_CIRCUMSTANCE_HPP
#define QUEST_CIRCUMSTANCE_HPP

// this is passed as a parameter to Mediator::updateQuestIcons()

enum QuestCircumstance {
    WIN_FROM_KILL_KNIGHTS,   // win by killing all other knights (with all homes secured)
    WIN_FROM_COMPLETE_QUEST,     // win from completing the quest (escape from dgn, destroy book, etc)
    JUST_AN_UPDATE                 // just updating quest requirements mid-game, no-one has won yet
};

#endif

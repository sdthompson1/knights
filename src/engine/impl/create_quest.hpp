/*
 * create_quest.hpp
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

#ifndef CREATE_QUEST_HPP
#define CREATE_QUEST_HPP

#include "dungeon_generator.hpp"  // for ExitType

#include "boost/shared_ptr.hpp"
#include <string>

class Quest;
class KnightsConfigImpl;

// the following pops an argument list from the kfile.
// created quest(s) are appended to the vector.
void CreateQuests(const std::string &name,
                  KnightsConfigImpl &kc,
                  std::vector<boost::shared_ptr<Quest> > &output);

void CreateEscapeQuest(ExitType e,
                       std::vector<boost::shared_ptr<Quest> > &output);

#endif

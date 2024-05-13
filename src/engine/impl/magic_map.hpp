/*
 * magic_map.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2024.
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
 * Map-related magic effects
 *
 */

#ifndef MAGIC_MAP_HPP
#define MAGIC_MAP_HPP

#include "boost/shared_ptr.hpp"

class DungeonMap;
class Knight;
class Player;

void MagicMapping(boost::shared_ptr<Knight> kt);
void MagicMapping(Player &player, const DungeonMap &dmap);

void WipeMap(boost::shared_ptr<Knight> kt);

void SenseItems(boost::shared_ptr<Knight> kt, int stop_time);

#endif

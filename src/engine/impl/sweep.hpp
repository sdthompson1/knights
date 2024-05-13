/*
 * sweep.hpp
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

#ifndef SWEEP_HPP
#define SWEEP_HPP

#include "map_support.hpp"

// SweepCreatures moves any creatures that need to be moved, according to
// the new access level.
// If "use_height" is true then restrict to creatures at the given height only. (If
// "use_height" is false then pass in a dummy value for ht, eg H_MISSILES.)
void SweepCreatures(DungeonMap &dmap, const MapCoord &mc, bool use_height, MapHeight ht, const Originator &originator);

// SweepItems moves aside any item at mc (or destroys it if it can't
// be moved). Called when items-allowed becomes FALSE.
void SweepItems(DungeonMap &dmap, const MapCoord &mc);

#endif

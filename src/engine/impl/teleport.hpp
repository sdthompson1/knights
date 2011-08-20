/*
 * teleport.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
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
 * Routines to implement teleportation.
 *
 */

#ifndef TELEPORT_HPP
#define TELEPORT_HPP

#include "boost/shared_ptr.hpp"
using namespace boost;

class Entity;
class Knight;

// Teleport an entity directly to a given square
// If the square is occupied, try nearby alternative squares.
// Returns true if successful.
bool TeleportToSquare(shared_ptr<Entity> from, DungeonMap &dmap, const MapCoord &mc);

// Teleport an entity to a randomly selected square.
// Returns true if successful.
bool TeleportToRandomSquare(shared_ptr<Entity> ent);

// Teleport an entity into the same room as some target entity.
void TeleportToRoom(shared_ptr<Entity> from, shared_ptr<Entity> to);

// Find the nearest other knight. NOTE: This isn't used currently (at time of writing anyway)
shared_ptr<Knight> FindNearestOtherKnight(const DungeonMap &, const MapCoord &);

// Find a random other knight
shared_ptr<Knight> FindRandomOtherKnight(shared_ptr<Knight> me);

#endif

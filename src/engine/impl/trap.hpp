/*
 * trap.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2013.
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
 * Trap: Base class for trap implementations (ie poison or blade traps).
 * 
 */

#ifndef TRAP_HPP
#define TRAP_HPP

#include "boost/shared_ptr.hpp"
using namespace boost;

class Creature;
class DungeonMap;
class ItemType;
class MapCoord;
class Originator;

class Trap {
public:
    explicit Trap(ItemType *it) : trap_item(it) { }
    virtual ~Trap() { }
    ItemType *getTrapItem() const { return trap_item; }
    virtual void spring(DungeonMap &, const MapCoord &, shared_ptr<Creature>, const Originator &originator) = 0;
    virtual bool activateOnHit() const = 0;
private:
    ItemType *trap_item;
};

#endif

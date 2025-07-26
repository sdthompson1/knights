/*
 * concrete_traps.hpp
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

/*
 * Concrete Trap classes (poison darts and spring blade traps).
 *
 */

#ifndef TRAPS_HPP
#define TRAPS_HPP

#include "map_support.hpp"
#include "trap.hpp"

class BladeTrap : public Trap {
public:
    BladeTrap(ItemType *trap_item, ItemType &missile_type,
              MapDirection fire_direction)
        : Trap(trap_item), mtype(missile_type), fire_dirn(fire_direction) { }
    virtual void spring(DungeonMap &, const MapCoord &, shared_ptr<Creature>, const Originator &originator);
    virtual bool activateOnHit() const { return true; }
private:
    ItemType &mtype;
    MapDirection fire_dirn;
};

class PoisonTrap : public Trap {
public:
    explicit PoisonTrap(ItemType *trap_item) : Trap(trap_item) { }
    virtual void spring(DungeonMap &, const MapCoord &, shared_ptr<Creature>, const Originator &originator);
    virtual bool activateOnHit() const { return false; }
};

#endif

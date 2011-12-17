/*
 * missile.hpp
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

#ifndef MISSILE_HPP
#define MISSILE_HPP

#include "entity.hpp"
#include "map_support.hpp"
#include "originator.hpp"

class ItemType;


// Add a new missile to the map. "Drop_after" controls whether an item
// should be dropped after the missile hits (true for axes, daggers
// etc, but false for skulls). "With_strength" doubles the range.
// "owner" => kills will be attributed to this player.
// "allow_friendly_fire" => If false, missile cannot hit the owner or anyone on same team as owner (used for crossbows/daggers/etc).
//                       => If true, the missile can hit anyone (used for traps etc).
// Returns true if the missile was successfully created.
bool CreateMissile(DungeonMap &dmap, const MapCoord &mc, MapDirection dir, 
                   const ItemType &it, bool drop_after, bool with_strength,
                   const Originator &owner, bool allow_friendly_fire);

// The Missile class itself
class Missile : public Entity {
    friend class MissileTask;

public:
    Missile(const ItemType &it, bool da, const Originator &ownr, bool allow_friendly_fire_);
    virtual MapHeight getHeight() const;
    void doubleRange() { range_left *= 2; } // used for strength
    const Originator & getOwner() const { return owner; }
    
    enum HitResult { FRIENDLY_FIRE, IGNORE, CAN_HIT };
    HitResult canHitPlayer(const Player *target) const;

private:
    const ItemType &itype;
    int range_left;
    bool drop_after;
    Originator owner;
    bool allow_friendly_fire;
};

#endif

/*
 * monster.hpp
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
 * The Monster class includes code in the onDeath routine to place
 * a monster corpse into the map.
 *
 * A pointer to the MonsterType is also stored.
 *
 */

#ifndef MONSTER_HPP
#define MONSTER_HPP

#include "creature.hpp"

class MonsterType;

class Monster : public Creature {
public:
    Monster(const MonsterType &type_, int health, MapHeight ht,
            const ItemType *item_in_hand, const Anim *anim, int speed)
        : Creature(health, ht, item_in_hand, anim, speed),
          type(type_) { }

    virtual ~Monster();
    virtual void onDeath(DeathMode dmode, const Originator &);

    virtual Originator getOriginator() const { return Originator(OT_Monster()); }

    const MonsterType & getMonsterType() const { return type; }

private:
    const MonsterType &type;
};

#endif

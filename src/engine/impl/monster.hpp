/*
 * monster.hpp
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

class LuaFunc;
class MonsterType;

class Monster : public Creature {
public:
    Monster(const MonsterType &type_, int health, MapHeight ht,
            ItemType *item_in_hand, const Anim *anim, int speed)
        : Creature(health, ht, item_in_hand, anim, speed),
          type(type_) { }

    virtual ~Monster();

    virtual void damage(int amount, const Originator &originator, int stun_until, bool inhibit_squelch);
    virtual void onDeath(DeathMode dmode, const Originator &);
    virtual void onDownswing();

    virtual Originator getOriginator() const { return Originator(OT_Monster()); }

    const MonsterType & getMonsterType() const { return type; }

    // If the monster is currently moving, then run the "on_move" Lua function
    // (otherwise, do nothing)
    void runMovementAction();
    
protected:
    void runAction(const LuaFunc &);
    
private:
    const MonsterType &type;
};

#endif

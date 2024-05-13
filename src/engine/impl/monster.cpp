/*
 * monster.cpp
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

#include "misc.hpp"

#include "action_data.hpp"
#include "mediator.hpp"
#include "monster.hpp"
#include "monster_type.hpp"

Monster::~Monster()
{
    try {
        Mediator::instance().onMonsterDeath(type);
    } catch (...) {
        // ensure no exceptions escape from this dtor.
    }
}

void Monster::damage(int amount, const Originator &originator, int stun_until, bool inhibit_squelch)
{
    if (amount > 0 && amount < getHealth()) {
        // call "on_damage" when non-fatal damage is received
        runAction(type.getOnDamage());
    }

    // call base class
    Creature::damage(amount, originator, stun_until, inhibit_squelch);
}

void Monster::onDeath(DeathMode dmode, const Originator &)
{
    // Call the lua "on_death" action if required.
    runAction(type.getOnDeath());
    
    // Place the monster corpse.
    if (getMap()) {
        if (dmode != PIT_MODE && dmode != ZOMBIE_MODE) {
            Mediator::instance().placeMonsterCorpse(*getMap(), getNearestPos(), type);
        }
    }
}

void Monster::onDownswing()
{
    runAction(type.getOnAttack());
}

void Monster::runMovementAction()
{
    if (isMoving()) {
        runAction(type.getOnMove());
    }
}

void Monster::runAction(const LuaFunc &func)
{
    if (func.hasValue()) {
        ActionData ad;
        ad.setActor(static_pointer_cast<Creature>(shared_from_this()));
        ad.setGenericPos(getMap(), getPos());
        ad.setOriginator(Originator(OT_Monster()));
        func.execute(ad);
    }
}

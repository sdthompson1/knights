/*
 * monster.cpp
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

#include "misc.hpp"

#include "mediator.hpp"
#include "monster.hpp"

Monster::~Monster()
{
    try {
        Mediator::instance().onMonsterDeath(type);
    } catch (...) {
        // ensure no exceptions escape from this dtor.
    }
}

void Monster::onDeath(DeathMode dmode, const Originator &)
{
    // We use this routine to place the monster corpse.
    if (!getMap()) return;
    if (dmode != PIT_MODE && dmode != ZOMBIE_MODE) {
        Mediator::instance().placeMonsterCorpse(*getMap(), getNearestPos(), type);
    }
}

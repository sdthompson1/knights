/*
 * concrete_traps.cpp
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

#include "misc.hpp"

#include "concrete_traps.hpp"
#include "creature.hpp"
#include "dungeon_view.hpp"
#include "missile.hpp"
#include "player.hpp"

void BladeTrap::spring(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature>, Player *player)
{
    CreateMissile(dmap, mc, fire_dirn, mtype, false, false, player, true);
}

void PoisonTrap::spring(DungeonMap &, const MapCoord &, shared_ptr<Creature> cr, Player *player)
{
    if (cr) {
        Player * pl = cr->getPlayer();
        if (pl) {
            pl->getDungeonView().flashMessage("Poison", 4);
        }
        
        cr->poison(player);
    }
}

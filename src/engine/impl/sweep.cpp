/*
 * sweep.cpp
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

#include "creature.hpp"
#include "dungeon_map.hpp"
#include "item.hpp"
#include "mediator.hpp"
#include "sweep.hpp"
#include "tile.hpp"

void SweepItems(DungeonMap &dmap, const MapCoord &mc)
{
    shared_ptr<Item> it = dmap.getItem(mc);
    if (it) {
        // Remove existing item from the map
        dmap.rmItem(mc);

        // Try to add it back again
        // (We allow it to be placed non-locally if necessary.)
        DropItem(it, dmap, mc, true, false, D_NORTH, shared_ptr<Creature>());
    }
}


void SweepCreatures(DungeonMap &dmap, const MapCoord &mc, bool use_height, MapHeight ht, const Originator &originator)
{
    using std::vector;
    
    vector<shared_ptr<Entity> > ents;
    dmap.getEntities(mc, ents);

    for (vector<shared_ptr<Entity> >::iterator it = ents.begin(); it != ents.end(); ++it) {

        // Check height if required
        if (use_height && (*it)->getHeight() != ht) continue;

        // Entity is only considered for damage if it is at least 50% in the target square,
        // OR if it is walking towards the target square.
        if ((*it)->getNearestPos() == mc
        || ((*it)->getMotionType() == MT_MOVE && DisplaceCoord((*it)->getPos(), (*it)->getFacing()) == mc)) {

            // Check if the entity really is a creature, and that the tile really is blocked...
            shared_ptr<Creature> cr = dynamic_pointer_cast<Creature>(*it);
            MapAccess acc = dmap.getAccess(mc, (*it)->getHeight(), it->get());
            if (acc <= A_APPROACH && cr) {
                // Deal DOOR_CLOSED_DAMAGE to creatures finding themselves in A_BLOCKED squares.
                cr->damage(Mediator::instance().cfgInt("door_closed_damage"), originator);
            }
        }
    }

    dmap.getAllEntities(mc, ents);
    for (vector<shared_ptr<Entity> >::iterator it = ents.begin(); it != ents.end(); ++it) {
        if (use_height && (*it)->getHeight() != ht) continue;
        if ((*it)->isApproaching() && (*it)->getPos() != mc) {
            // An entity is approaching the square (from an adjacent square).
            MapAccess acc = dmap.getAccess(mc, (*it)->getHeight(), it->get());
            if (acc != A_APPROACH) {
                MotionType mt = (*it)->getMotionType();
                if (acc == A_CLEAR && (mt == MT_NOT_MOVING || mt == MT_APPROACH)) {
                    // The tile is clear.
                    // Also, the entity is either not moving, or is already moving forwards.
                    // Move it forwards.
                    (*it)->move(MT_MOVE);
                } else if (mt != MT_WITHDRAW) {
                    // If we can't move forward, then go back
                    (*it)->move(MT_WITHDRAW);
                }
            }
        }
    }
}


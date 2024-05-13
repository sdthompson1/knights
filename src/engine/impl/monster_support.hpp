/*
 * monster_support.hpp
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

/*
 * A couple of support routines for monster AI.
 *
 */

#ifndef MONSTER_SUPPORT_HPP
#define MONSTER_SUPPORT_HPP

#include "knight.hpp"
#include "mediator.hpp"
#include "player.hpp"
#include "rng.hpp"
#include "room_map.hpp"

#include "boost/shared_ptr.hpp"
using namespace boost;

#include <utility>


//
// Check if there is a knight at a given square
// The knight must not be holding one of the "fear" items (if any)
//
bool KnightAt(DungeonMap &dmap, const MapCoord &mc, const std::vector<ItemType *> &fear);

//
// Get direction from one square to another
//
MapDirection DirectionFromTo(const MapCoord &from, const MapCoord &to);


//
// Find the closest knight in the same room as a monster
// The knight also has to satisfy the given predicate.
//
template<class T>
shared_ptr<Knight> FindClosestKnight(shared_ptr<Entity> ent, T predicate)
{
    if (!ent->getMap()) return shared_ptr<Knight>();
    const std::vector<Player*> &players(Mediator::instance().getPlayers());

    std::vector<shared_ptr<Knight> > best_so_far;
    best_so_far.reserve(2);
    int dist = 99999;  // we use a simple Manhattan distance.
    
    for (int i=0; i<players.size(); ++i) {
        shared_ptr<Knight> kt = players[i]->getKnight();
        if (!kt) continue;
        if (kt->getMap() != ent->getMap()) continue;
        if (!predicate(kt)) continue;
        if (ent->getMap()->getRoomMap() &&
            !ent->getMap()->getRoomMap()->inSameRoom(ent->getPos(), kt->getPos())) continue;
        const int d = abs(kt->getPos().getX() - ent->getPos().getX())
            + abs(kt->getPos().getY() - ent->getPos().getY());
        if (d < dist) {
            dist = d;
            best_so_far.clear();
            best_so_far.push_back(kt);
        } else if (d == dist) {
            best_so_far.push_back(kt);
        }
    }

    if (best_so_far.empty()) {
        return shared_ptr<Knight>();
    } else {
        const int idx = g_rng.getInt(0, best_so_far.size());
        return best_so_far[idx];
    }
}


//
// Choose a direction for a monster to move in
//
template<class T>
std::pair<MapDirection,bool> ChooseDirection(shared_ptr<Entity> ent, const MapCoord &target_pos,
                                             bool afraid, T can_walk_into_predicate)
{
    if (!ent) return std::make_pair(D_NORTH,false);

    // Work out vector to target
    int d[2];
    MapDirection basedir[2];
    basedir[0] = D_EAST;
    basedir[1] = D_SOUTH;
    if (target_pos.isNull()) {
        d[0] = d[1] = 0;
    } else {
        d[0] = target_pos.getX() - ent->getPos().getX();
        d[1] = target_pos.getY() - ent->getPos().getY();
    }

    // If afraid, then reverse the target vector
    if (afraid) {
        d[0] = -d[0];
        d[1] = -d[1];
    }   

    // If 50% chance, then swap order of x and y
    if (g_rng.getBool(0.5f)) {
        std::swap(d[0],d[1]);
        std::swap(basedir[0],basedir[1]);
    }   
    
    // Build up a list of directions
    MapDirection dir[4];
    int next_dir = 0;
    for (int i=0; i<2; ++i) {
        if (d[i] > 0) {
            dir[next_dir++] = basedir[i];
        } else if (d[i] < 0) {
            dir[next_dir++] = Opposite(basedir[i]);
        }
    }

    // now include "reverse" directions as well
    for (int i=0; i<2; ++i) {
        if (d[i] == 0) {
            if (g_rng.getBool(0.5f)) {
                dir[next_dir++] = basedir[i];
                dir[next_dir++] = Opposite(basedir[i]);
            } else {
                dir[next_dir++] = Opposite(basedir[i]);
                dir[next_dir++] = basedir[i];
            }
        } else if (d[i] > 0) {
            dir[next_dir++] = Opposite(basedir[i]);
        } else {
            dir[next_dir++] = basedir[i];
        }
    }

    ASSERT(next_dir==4);

    // Try each direction in turn
    for (int i=0; i<4; ++i) {
        if (can_walk_into_predicate(*ent->getMap(), DisplaceCoord(ent->getPos(), dir[i]))) {
            return std::make_pair(dir[i],true);
        }
    }

    // Failed (looks like we can't walk in any direction at all!)
    return std::make_pair(D_NORTH,false);
}

#endif

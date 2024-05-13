/*
 * teleport.cpp
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

#include "dungeon_map.hpp"
#include "knight.hpp"
#include "mediator.hpp"
#include "rng.hpp"
#include "player.hpp"
#include "room_map.hpp"
#include "teleport.hpp"

namespace {
    void DoTeleportToSquare(shared_ptr<Entity> from, DungeonMap &dmap, const MapCoord &mc,
                            MapDirection new_facing)
    {
        if (!from) return;
        
        // Set the knight's "teleported recently" flag. This will prevent him seeing his
        // location immediately after the teleport (unless he teleports to a room he has
        // already mapped) -- see Trac #68.
        Knight * kt = dynamic_cast<Knight*>(from.get());
        if (kt) {
            kt->getPlayer()->setTeleportFlag(true);
        }

        // Now do the teleport.
        from->rmFromMap();
        from->setFacing(new_facing);
        from->addToMap(&dmap, mc);

    }

    bool TrySquare(shared_ptr<Entity> from, DungeonMap &dmap, const MapCoord &mc)
    {
        if (dmap.getAccess(mc, H_WALKING) == A_CLEAR) {
            DoTeleportToSquare(from, dmap, mc, from->getFacing());
            return true;
        }
        return false;
    }
}

bool TeleportToSquare(shared_ptr<Entity> from, DungeonMap &dmap, const MapCoord &mc)
{
    if (!from) return false;
    if (from->getMap() != &dmap) return false;  // just checking
    const MapDirection facing = from->getFacing();
        
    // Try to find either this square or one of the four surrounding squares (for now).
    return (TrySquare(from, dmap, mc)
            || TrySquare(from, dmap, DisplaceCoord(mc, facing))
            || TrySquare(from, dmap, DisplaceCoord(mc, Opposite(facing)))
            || TrySquare(from, dmap, DisplaceCoord(mc, Clockwise(facing)))
            || TrySquare(from, dmap, DisplaceCoord(mc, Anticlockwise(facing))));
}

bool TeleportToRandomSquare(shared_ptr<Entity> ent)
{
    if (!ent) return false;
    DungeonMap *dmap = ent->getMap();
    if (!dmap) return false;

    // 50 random attempts to find an empty square, after which we give up.
    for (int i = 0; i < 50; ++i) {
        MapCoord mc(g_rng.getInt(1, dmap->getWidth()-1), g_rng.getInt(1, dmap->getHeight()-1));
        if (TrySquare(ent, *dmap, mc)) return true;
    }

    return false;
}

void TeleportToRoom(shared_ptr<Entity> from, shared_ptr<Entity> to)
{
    if (!from || !to) return;
    DungeonMap *dmap = from->getMap();
    if (!dmap || to->getMap() != dmap) return;

    const RoomMap *rmap = dmap->getRoomMap();
    int r1, r2;
    rmap->getRoomAtPos(to->getPos(), r1, r2);
    if (r2 != -1) {
        // Two rooms were returned. We just choose one at random.
        if (g_rng.getBool()) r1 = r2;
        r2 = -1;
    }

    // r1 now contains the room we want to teleport into.
    // Find the room dimensions
    MapCoord top_left;
    int width, height;
    rmap->getRoomLocation(r1, top_left, width, height);
        
    // We must now find an unoccupied space within the room.
    MapDirection new_facing = MapDirection(g_rng.getInt(0,4));
    MapCoord new_mc;
    for (int i=0; i<100; ++i) {
        // (try 100 times to find a suitable square)
        const int x = g_rng.getInt(0, width);
        const int y = g_rng.getInt(0, height);
        MapCoord mc2 = MapCoord(top_left.getX() + x,
                                top_left.getY() + y);
        if (dmap->getAccess(mc2, H_WALKING) == A_CLEAR) {
            new_mc = mc2;
            break;
        }
    }
    if (new_mc.isNull()) return;  // failed to find an empty square.

    // Now we just have to move "from" to the new square.
    DoTeleportToSquare(from, *dmap, new_mc, new_facing);
}

shared_ptr<Knight> FindNearestOtherKnight(const DungeonMap &dmap, const MapCoord &mypos)
{
    using std::vector;

    // Search through all players, looking for the closest.
    const vector<Player*> & players(Mediator::instance().getPlayers());
    int dist = 99999;
    shared_ptr<Knight> target;
    for (vector<Player*>::const_iterator it = players.begin(); it != players.end(); ++it) {
        shared_ptr<Knight> kt = (*it)->getKnight();
        if (!kt) continue;
        if (kt->getMap() != &dmap) continue;
        const MapCoord kpos = kt->getPos();
                
        // Measure distance by Manhattan distance:
        const int d = abs(kpos.getX() - mypos.getX()) + abs(kpos.getY() - mypos.getY());

        if ((d > 0 && d < dist) || (d == dist && g_rng.getBool())) {
            // Closer target found
            // (or if equal distance, then choose target randomly)
            dist = d;
            target = kt;
        }
    }

    return target;
}

shared_ptr<Knight> FindRandomOtherKnight(shared_ptr<Knight> me)
{
    const std::vector<Player*> & players(Mediator::instance().getPlayers());
    
    std::vector<int> search_order(players.size());
    for (int i = 0; i < players.size(); ++i) search_order[i] = i;
    RNG_Wrapper myrng(g_rng);
    std::random_shuffle(search_order.begin(), search_order.end(), myrng);

    shared_ptr<Knight> result;

    for (int i = 0; i < search_order.size(); ++i) {
        shared_ptr<Knight> kt = players[search_order[i]]->getKnight();
        if (!kt) continue;
        if (kt == me) continue;
        result = kt;
        break;
    }

    return result;
}

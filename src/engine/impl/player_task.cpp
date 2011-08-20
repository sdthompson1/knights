/*
 * player_task.cpp
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

#include "creature.hpp"
#include "dungeon_map.hpp"
#include "dungeon_view.hpp"
#include "knight.hpp"
#include "mediator.hpp"
#include "player.hpp"
#include "player_task.hpp"
#include "room_map.hpp"
#include "task_manager.hpp"

void PlayerTask::execute(TaskManager &tm)
{
    // At the moment, all we do here is update the player's mini-map as necessary.

    // Find the room the player is currently in
    shared_ptr<Knight> kt = pl.getKnight();
    const int rm = pl.getCurrentRoom();

    // Find whether the current room is mapped already
    bool mapped = pl.isRoomMapped(rm);

    // Find whether the knight has teleported recently
    bool teleport_flag = pl.getTeleportFlag();

    // If room unmapped => map it if no creatures present
    // If teleport flag set => unset if no creatures present
    
    if (rm != -1 && kt && (!mapped || teleport_flag)) {
    
        MapCoord top_left;     // top left of current room
        int w, h;              // width, height of current room
    
        const DungeonMap *dmap = pl.getDungeonMap();
        const RoomMap *room_map = pl.getRoomMap();

        if (dmap && room_map) {
            room_map->getRoomLocation(rm, top_left, w, h);
            
            // Check whether there are creatures in the room
            // (TODO) This could be made faster, write a dungeonmap routine to find all
            // entities within a room.
            bool found_creature = false;
            vector<shared_ptr<Entity> > entities;
            for (int i=0; i<w; ++i) {
                for (int j=0; j<h; ++j) {
                    MapCoord mc(top_left.getX() + i, top_left.getY() + j);
                    dmap->getEntities(mc, entities);
                    for (vector<shared_ptr<Entity> >::iterator it = entities.begin();
                    it != entities.end(); ++it) {
                        if (dynamic_cast<Creature*>(it->get()) && (*it) != kt) {
                            found_creature = true;
                            break;
                        }
                    }
                    if (found_creature) break;
                }
                if (found_creature) break;
            }

            if (!found_creature) {
                if (!mapped) {
                    // Map the current room
                    pl.mapCurrentRoom(top_left);
                    mapped = true;
                }

                if (teleport_flag) {
                    // Clear the teleport flag
                    pl.setTeleportFlag(false);
                    teleport_flag = false;
                }
            }
        }
    }

    // The player's current location should be shown on the map if
    // !teleport_flag OR current room is mapped.
    if (kt && kt->getMap() && (mapped || !teleport_flag)) {
        pl.mapKnightLocation(pl.getPlayerNum(), kt->getPos().getX(), kt->getPos().getY());
    } else {
        pl.mapKnightLocation(pl.getPlayerNum(), -1, -1);
    }

    // Look to see if we can see other knights:

    Mediator &med(Mediator::instance());

    if (kt && kt->getMap()) {
        bool have_sense_knight = kt->getSenseKnight() || kt->getCrystalBall();
        for (int i=0; i<med.getPlayers().size(); ++i) {
            const Player &p = *(med.getPlayers()[i]);
            if (&p == &(this->pl)) continue;  // don't do myself!
            shared_ptr<Knight> other_kt = p.getKnight();
            // Can see knight if I have SenseKnight or CrystalBall, or he has
            // RevealLocation
            bool can_see = false;
            if (other_kt) {
                can_see = have_sense_knight || other_kt->getRevealLocation();
            }
            if (can_see && other_kt->getMap()==kt->getMap()) {
                pl.mapKnightLocation(p.getPlayerNum(), other_kt->getPos().getX(), other_kt->getPos().getY());
            } else {
                pl.mapKnightLocation(p.getPlayerNum(), -1, -1);
            }
        }
    }
        
    // Now reschedule the task
    shared_ptr<PlayerTask> pt(new PlayerTask(pl));
    tm.addTask(pt, TP_NORMAL, tm.getGVT() + med.cfgInt("player_task_interval"));
}

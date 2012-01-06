/*
 * view_manager.cpp
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

#include "dungeon_map.hpp"
#include "dungeon_view.hpp"
#include "entity.hpp"
#include "item.hpp"
#include "knight.hpp"
#include "knights_callbacks.hpp"
#include "mediator.hpp"
#include "my_exceptions.hpp"
#include "player.hpp"
#include "room_map.hpp"
#include "task_manager.hpp"
#include "tile.hpp"
#include "view_manager.hpp"

//
// ctor
//

ViewManager::ViewManager(int nplyrs)
    : next_id(nplyrs), active_ids(nplyrs), id_map(nplyrs)
{
    for (int i=0; i<nplyrs; ++i) {
        next_id[i] = 1;
    }
}


//
// onAddPlayer
//

void ViewManager::onAddPlayer(Player &p)
{
    // add to "players"
    players.push_back(&p);
}



//
// utility routines for rooms & visibility
//

void ViewManager::getCurrentRoomLocation(const Player &player, const DungeonMap &dmap,
                                         MapCoord &top_left, int &width, int &height)
{
    if (dmap.getRoomMap()) {
        dmap.getRoomMap()->getRoomLocation(player.getCurrentRoom(), top_left,
                                           width, height);
    } else {
        top_left = MapCoord();
        width = height = 0;
    }
}
        
bool ViewManager::squareVisibleToPlayer(const DungeonMap &dmap, const MapCoord &mc,
                                        const Player &p)
{
    MapCoord top_left;
    int width, height;
    getCurrentRoomLocation(p, dmap, top_left, width, height);
    return (mc.getX() >= top_left.getX() &&
            mc.getX() <  top_left.getX() + width &&
            mc.getY() >= top_left.getY() &&
            mc.getY() <  top_left.getY() + height);
}

bool ViewManager::entityVisibleToPlayer(const Entity &ent, const Player &p, bool trueview)
{
    shared_ptr<Knight> kt(p.getKnight());
    if (&ent == kt.get()) return true;    // players are always visible to themselves.
    if (!ent.getMap()) return false;
    if (!ent.isVisible() && !trueview) return false;
    if (squareVisibleToPlayer(*ent.getMap(), ent.getPos(), p)) return true;
    if (ent.isMoving() && !ent.isApproaching()) {
        MapCoord mc = DisplaceCoord(ent.getPos(), ent.getFacing());
        if (squareVisibleToPlayer(*ent.getMap(), mc, p)) return true;
    }
    return false;
}

void ViewManager::getRelativePos(const Player &p, const DungeonMap *dmap,
                                 const MapCoord &mc, int &x, int &y)
{
    // (dmap is passed by pointer for convenience.)
    if (!dmap) {
        x = y = 0;
    } else {
        MapCoord top_left;
        int width, height;
        getCurrentRoomLocation(p, *dmap, top_left, width, height);
        x = mc.getX() - top_left.getX();
        y = mc.getY() - top_left.getY();
    }
}


//
// routines for uploading rooms and handling "squares_invalid" cache
//

void ViewManager::invalidateSquare(const Player &p, const RoomMap &rm, const MapCoord &mc)
{
    // This marks square "mc" as invalid (for player p).

    // Go through each cache entry in turn -- look for rooms that contain "mc"
    SquaresInvalidMap & s(squares_invalid_cache[&p]);
    for (SquaresInvalidMap::iterator it = s.begin(); it != s.end(); ++it) {
        if (it->first == p.getCurrentRoom()) continue; // do not invalidate anything for current room.
        MapCoord tl;
        int w, h;
        rm.getRoomLocation(it->first, tl, w, h);
        if (mc.getX() >= tl.getX() && mc.getX() < tl.getX()+w
        && mc.getY() >= tl.getY() && mc.getY() < tl.getY()+h) {
            // The square "mc" is within this room. Invalidate the corresponding cache entry
            const int index = (mc.getX() - tl.getX()) + (mc.getY() - tl.getY())*w;
            it->second[index] = true;
        }
    }
}

void ViewManager::uploadRoomSquares(const Player &p)
{
    // This sends all necessary squares to the client. The current
    // room (on p.getDungeonView()) must have been set correctly
    // before calling this.
    
    int current_room;
    MapCoord top_left;
    int width, height;

    // work out the player's current room, and dmap.
    if (!p.getKnight()) return;
    if (!p.getKnight()->getMap()) return;
    const DungeonMap &dmap(*p.getKnight()->getMap());
    if (!dmap.getRoomMap()) return;
    current_room = p.getCurrentRoom();
    dmap.getRoomMap()->getRoomLocation(current_room, top_left, width, height);

    // create the cache entry if necessary
    SquaresInvalidMap &s(squares_invalid_cache[&p]);
    vector<bool> &cache(s[current_room]);
    if (cache.empty()) cache.resize(width*height);
    ASSERT(cache.size()==width*height);    
    
    // check every square in the room.
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = y*width + x;

            // upload the square. if the player's cached square is invalid then the square will be sent always,
            // otherwise it will only be sent if this is the first time this player/observer has ever seen this square.
            const bool square_invalid = cache[index];
            
            uploadSquare(p.getDungeonView(), dmap,
                         MapCoord(x + top_left.getX(), y + top_left.getY()),
                         x, y,
                         square_invalid);

            // mark it as valid in the cache, if necessary.
            if (square_invalid) cache[index] = false;
        }
    }
}

void ViewManager::uploadSquare(DungeonView &dview, const DungeonMap &dmap, const MapCoord &mc,
                               int relx, int rely, bool force)
{
    // This sends to 'dview' the item and tile information for square 'mc'.
    // (It does not mark the square as valid in the cache.)

    // Item
    shared_ptr<Item> it = dmap.getItem(mc);
    if (it) dview.setItem(relx, rely, it->getGraphic(), force);
    else dview.setItem(relx, rely, 0, force);
    
    // Tiles
    dview.clearTiles(relx, rely, force);
    vector<shared_ptr<Tile> > tiles;
    dmap.getTiles(mc, tiles);
    for (vector<shared_ptr<Tile> >::const_iterator t = tiles.begin(); t != tiles.end(); ++t) {
        dview.setTile(relx, rely, (*t)->getDepth(), (*t)->getGraphic(), (*t)->getColourChange(), force);
    }
}


//
// updateRoom
//
// This checks to see if the DungeonView's "current_room" value needs
// to be changed. If so, the DungeonView is flipped over to the new
// room, and all necessary tiles, items and entities in the new room
// are uploaded to the client.
//
// Return value is TRUE if current room was changed, otherwise FALSE.
//

bool ViewManager::updateRoom(int plyr)
{
    // First, work out which room the DungeonView should be set to.
    shared_ptr<Knight> kt(players[plyr]->getKnight());
    if (!kt) return false;
    if (!kt->getMap()) return false;
    DungeonMap &dmap(*kt->getMap());
    if (!dmap.getRoomMap()) return false;

    int r1, r2, r_chosen;
    dmap.getRoomMap()->getRoomAtPos(kt->getPos(), r1, r2);
    if (r2 == -1) {
        r_chosen = r1;
    } else {
        // Knight is on border between two rooms. Determine which one
        // to use by considering the knight's facing direction.
        MapCoord top_left_1, top_left_2;
        int dum1, dum2;
        dmap.getRoomMap()->getRoomLocation(r1, top_left_1, dum1, dum2);
        dmap.getRoomMap()->getRoomLocation(r2, top_left_2, dum1, dum2);
        if (kt->getPos().getX() == top_left_1.getX()) {
            // Western edge of room 1
            if (kt->getFacing() == D_WEST) r_chosen = r2;
            else if (kt->getFacing() == D_EAST) r_chosen = r1;
            else r_chosen = -1;
        } else if (kt->getPos().getX() == top_left_2.getX()) {
            // Western edge of room 2
            if (kt->getFacing() == D_WEST) r_chosen = r1;
            else if (kt->getFacing() == D_EAST) r_chosen = r2;
            else r_chosen = -1;
        } else if (kt->getPos().getY() == top_left_1.getY()) {
            // Northern edge of room 1
            if (kt->getFacing() == D_NORTH) r_chosen = r2;
            else if (kt->getFacing() == D_SOUTH) r_chosen = r1;
            else r_chosen = -1;
        } else {
            // Northern edge of room 2
            ASSERT(kt->getPos().getY() == top_left_2.getY());
            if (kt->getFacing() == D_NORTH) r_chosen = r1;
            else if (kt->getFacing() == D_SOUTH) r_chosen = r2;
            else r_chosen = -1;
        }
    }

    int current_room = players[plyr]->getCurrentRoom();
    if (r_chosen == -1) {
        // "r_chosen==-1" means that we want the chosen room to be
        // either r1 or r2 -- we do not mind which.
        if (current_room == r1 || current_room == r2) return false;
        else r_chosen = r1;
    }
    
    if (current_room != r_chosen) {
        // Select the new room
        MapCoord top_left;
        int width, height;
        dmap.getRoomMap()->getRoomLocation(r_chosen, top_left, width, height);
        players[plyr]->setCurrentRoom(r_chosen, width, height);

        // Wipe out all the entity IDs (since setCurrentRoom will clear out all
        // entities from the dungeonview)
        next_id[plyr] = 1;
        active_ids[plyr].clear();
        id_map[plyr].clear();
        
        // Upload tiles and items
        uploadRoomSquares(*players[plyr]);

        // Upload all entities.
        vector<shared_ptr<Entity> > ents;
        for (int x = top_left.getX(); x < top_left.getX() + width; ++x) {
            for (int y = top_left.getY(); y < top_left.getY() + height; ++y) {
                MapCoord mc(x,y);
                dmap.getEntities(mc, ents);
                for (vector<shared_ptr<Entity> >::iterator it = ents.begin();
                it != ents.end(); ++it) {
                    bool mine = (*it == kt);
                    if ((*it)->isVisible() || mine) {
                        doAddEntity(plyr, *it, mine);
                    }
                }
            }
        }
        return true;
    } else {
        return false;
    }
}


//
// doAddEntity (doRmEntity): this handles the details of adding (removing) an entity to a dview.
//

void ViewManager::doAddEntity(int plyr, shared_ptr<Entity> ent, bool mine)
{
    ASSERT(ent && ent->getMap());
    if (fetchID(plyr, ent).second) return;  // Entity already added -- nothing to do. 
    const unsigned short int id = fetchNewID(plyr, ent, mine);
    reallyAddEntity(plyr, ent, id, players[plyr]->getDungeonView());
}

void ViewManager::reallyAddEntity(int plyr, shared_ptr<Entity> ent, unsigned short int id, DungeonView &dungeon_view)
{
    const Anim *anim;
    const Overlay *ovr;
    int af, atz, x, y;
    bool ainv;
    getRelativePos(*players[plyr], ent->getMap(), ent->getPos(), x, y);
    ent->getAnimData(anim, ovr, af, atz, ainv);
    const int gvt = Mediator::instance().getGVT();
    int motion_time_remaining = ent->getArrivalTime() - gvt;
    if (motion_time_remaining < 0) motion_time_remaining = 0;

    std::string name;
    bool show_speech_bubble = false;
    Knight *kt = dynamic_cast<Knight*>(ent.get());
    if (kt) {
        name = kt->getPlayer()->getName();
        show_speech_bubble = kt->getPlayer()->getSpeechBubble();
    }
    dungeon_view.addEntity(id, x, y, ent->getHeight(), ent->getFacing(), anim, ovr, af,
                           atz ? atz - gvt : 0, ainv, ent->getOffset(), ent->getMotionType(), motion_time_remaining,
                           name);
    if (show_speech_bubble) dungeon_view.setSpeechBubble(id, true);
}

void ViewManager::doRmEntity(int plyr, weak_ptr<Entity> ent, unsigned short int id)
{
    // remove it from the dungeon view
    players[plyr]->getDungeonView().rmEntity(id);
    // remove it from our own ID lists
    active_ids[plyr].erase(id);
    id_map[plyr].erase(ent);
}


//
// event handler routines for entity addition/removal/motion/etc
//

void ViewManager::onAddEntity(shared_ptr<Entity> ent)
{
    ASSERT(ent && ent->getMap());
    for (int p=0; p<players.size(); ++p) {
        if (ent == players[p]->getKnight()) {
            // My own knight is being added to dungeon.

            // May need to flip rooms.
            bool update_done = updateRoom(p);
            if (!update_done) {
                doAddEntity(p, ent, true);
            }

            // Set knight indicator on mini map (but only not-teleported-recently OR if current room is mapped)
            const MapCoord &mc = ent->getPos();
            if (!players[p]->getTeleportFlag() || players[p]->isSquareMapped(mc)) {
                players[p]->mapKnightLocation(players[p]->getPlayerNum(), mc.getX(), mc.getY());
            } else {
                players[p]->mapKnightLocation(players[p]->getPlayerNum(), -1, -1);
            }
            
        } else if (entityVisibleToPlayer(*ent, *players[p], false)) {
            // 'Ent' is not my own knight, but I can see it being added to the dungeon.
            doAddEntity(p, ent, false);
        }
    }
}

void ViewManager::onRmEntity(shared_ptr<Entity> ent)
{
    ASSERT(ent);
    for (int p=0; p<players.size(); ++p) {
        // Check if player p can see entity ent leaving the dungeon
        pair<unsigned short int, bool> id = fetchID(p, ent);
        if (id.second) {
            doRmEntity(p, ent, id.first);
        }
        // If this is my knight then remove the position indicator
        if (ent == players[p]->getKnight()) {
            players[p]->mapKnightLocation(players[p]->getPlayerNum(), -1, -1);
        }
    }
}

void ViewManager::onRepositionEntity(shared_ptr<Entity> ent)
{
    ASSERT(ent && ent->getMap());
    for (int p=0; p<players.size(); ++p) {
        shared_ptr<Knight> kt = players[p]->getKnight();
        pair<unsigned short int, bool> id = fetchID(p, ent);
        if (id.second) {
            if (ent == kt) {
                // Change room if necessary
                const bool room_changed = updateRoom(p);

                // Move the knight indicator (if not-teleported-recently OR current room is mapped)
                const MapCoord &mc = ent->getPos();
                if (!players[p]->getTeleportFlag() || players[p]->isSquareMapped(mc)) {
                    players[p]->mapKnightLocation(players[p]->getPlayerNum(), mc.getX(), mc.getY());
                } else {
                    players[p]->mapKnightLocation(players[p]->getPlayerNum(), -1, -1);
                }

                // If room was changed then can go on to next player
                if (room_changed) continue;
            }

            if (entityVisibleToPlayer(*ent, *players[p], false)) {
                // reposition
                int new_x, new_y;
                getRelativePos(*players[p], ent->getMap(), ent->getPos(), new_x, new_y);
                players[p]->getDungeonView().repositionEntity(id.first, new_x, new_y);
            } else {
                // remove from old square
                doRmEntity(p, ent, id.first);
            }
        } else {
            ASSERT(ent != kt); // Kt is always visible, hence should never be repositioned from an invisible square
            if (entityVisibleToPlayer(*ent, *players[p], false)) {
                // add to new square
                doAddEntity(p, ent, false);
            }
        }
    }
}

void ViewManager::onChangeEntityMotion(shared_ptr<Entity> ent, bool missile_mode)
{
    // (This is called after an entity either starts or stops moving.
    // We are only interested in the former case.)
    ASSERT(ent && ent->getMap());
    const int gvt = Mediator::instance().getGVT();
    if (ent->isMoving()) {
        // Start entity motion
        for (int p=0; p<players.size(); ++p) {
            if (entityVisibleToPlayer(*ent, *players[p], false)) {
                // Entity is *currently* visible to p (in its new position).
                                
                // If the entity was previously invisible to player p (and hence has no
                // ID currently) then we need to call doAddEntity.
                // (Note entities can only come into view as a result of moves; they cannot go
                // out of view until the following reposition. So we never need to do a
                // rmEntity in this routine.)
                pair<unsigned short int, bool> id = fetchID(p, ent);
                if (!id.second) {
                    doAddEntity(p, ent, false);
                } else {
                    // Entity was previously visible, so we want a move command, as opposed
                    // to an add command
                    players[p]->getDungeonView().moveEntity(id.first,
                                                            ent->getMotionType(), ent->getArrivalTime() - gvt, missile_mode);
                }
            }
        }
    }
}

void ViewManager::onFlipEntityMotion(shared_ptr<Entity> ent)
{
    ASSERT(ent && ent->getMap() && ent->isMoving() && !ent->isApproaching());

    const int gvt = Mediator::instance().getGVT();
        
    for (int p=0; p<players.size(); ++p) {
        if (entityVisibleToPlayer(*ent, *players[p], false)) {
            // Entity is currently visible to p
            // For a flip-motion, it should therefore have previously
            // been visible to p as well.
            pair<unsigned short int, bool> id = fetchID(p, ent);
            ASSERT(id.second);
            if (id.second) {
                const int start_time = ent->getStartTime();
                players[p]->getDungeonView().flipEntityMotion(id.first,
                                                              start_time - gvt,
                                                              ent->getArrivalTime() - start_time);
            }
        }
    }
}
        

//
// event handler routines for entity updates (not involving motion)
//

void ViewManager::onChangeEntityAnim(shared_ptr<Entity> ent)
{
    for (int p=0; p<players.size(); ++p) {
        pair<unsigned short int, bool> id = fetchID(p, ent);
        if (id.second) {
            ASSERT(ent && ent->getMap());
            const Anim *anim;
            const Overlay *ovr;
            int af, atz;
            bool ainv;
            ent->getAnimData(anim, ovr, af, atz, ainv);
            const int gvt = Mediator::instance().getGVT();
            if (atz==gvt) atz++; // avoid setting atz_diff to zero in this special case ...
            players[p]->getDungeonView().setAnimData(id.first, anim, ovr,
                                                     af, atz ? atz - gvt : 0, ainv, ent->isMoving());
        }
    }
}

void ViewManager::onChangeSpeechBubble(shared_ptr<Knight> ent)
{
    for (int p = 0; p < players.size(); ++p) {
        pair<unsigned short int, bool> id = fetchID(p, ent);
        if (id.second) {
            ASSERT(ent && ent->getMap());
            players[p]->getDungeonView().setSpeechBubble(id.first, ent->getPlayer()->getSpeechBubble());
        }
    }
}

void ViewManager::onChangeEntityFacing(shared_ptr<Entity> ent)
{
    ASSERT(ent);
    for (int p=0; p<players.size(); ++p) {
        shared_ptr<Knight> kt = players[p]->getKnight();
        // Do a room change if necessary
        if (ent == kt) {
            bool update_done = updateRoom(p);
            if (update_done) continue;
        }
        // Otherwise, if the entity is on-screen, then change the facing
        pair<unsigned short int, bool> id = fetchID(p, ent);
        if (id.second) {
            players[p]->getDungeonView().setFacing(id.first, ent->getFacing());
        }
    }
}

void ViewManager::onChangeEntityVisible(shared_ptr<Entity> ent)
{
    // this one works slightly differently.
    for (int p=0; p<players.size(); ++p) {
        if (ent == players[p]->getKnight()) continue; // Our own kt is always visible...
        if (ent->isVisible()) {
            // Entity has become visible (was invisible);
            // add it if it is on a square that we can see.
            if (entityVisibleToPlayer(*ent, *players[p], false)) {
                doAddEntity(p, ent, false);
            }
        } else {
            // entity has become invisible (was visible);
            // remove it if it is on-screen currently.
            pair<unsigned short int, bool> id = fetchID(p, ent);
            if (id.second) {
                doRmEntity(p, ent, id.first);
            }
        }
    }
}

//
// Entity ID management
//

unsigned short int ViewManager::fetchNewID(int plyr, weak_ptr<Entity> ent, bool mine)
{
    // We should not already have the entity
    ASSERT(id_map[plyr].find(ent) == id_map[plyr].end());

    // Create a new ID.
    unsigned short int new_id_number;
    if (mine) {
        // My Knight has a fixed ID of zero
        ASSERT(active_ids[plyr].find(0) == active_ids[plyr].end());
        new_id_number = 0;
    } else {
        // Find next available free ID
        unsigned short int starting_point = next_id[plyr]; // Safety check
        while (active_ids[plyr].find(next_id[plyr]) != active_ids[plyr].end()) {
            // Next_id[plyr] is "in use" at present
            // Try another one.
            ++next_id[plyr];
            if (next_id[plyr] == 0) next_id[plyr] = 1; // skip over the special ID (in case of overflow)
            if (next_id[plyr] == starting_point) throw UnexpectedError("Ran out of entity IDs");
        }
        new_id_number = next_id[plyr];
        ++next_id[plyr];
        if (next_id[plyr] == 0) next_id[plyr] = 1;
    }

    // Add the new ID to the id_map and the active_ids
    id_map[plyr].insert(make_pair(ent, new_id_number));
    active_ids[plyr].insert(new_id_number);
    return new_id_number;
}

pair<unsigned short int, bool> ViewManager::fetchID(int plyr, weak_ptr<Entity> ent)
{
    map<weak_ptr<Entity>, unsigned short int>::const_iterator it = id_map[plyr].find(ent);
    if (it == id_map[plyr].end()) {
        return make_pair(0, false);
    } else {
        return make_pair(it->second, true);
    }
}
                
        

//
// item or tile updates
//

void ViewManager::setItemIfVisible(const DungeonMap &dmap, const MapCoord &mc,
                                   const Graphic * graphic)
{
    // All add/rm tile and item commands follow the same pattern:
    // For each player
    //   If player can see the given square
    //     Do a setTile or setItem command
    //   Else
    //     Mark the square as invalid on that player / dungeonview.

    if (!dmap.getRoomMap()) return;
    for (vector<Player*>::iterator p = players.begin(); p != players.end(); ++p) {
        invalidateSquare(**p, *dmap.getRoomMap(), mc);
        if (squareVisibleToPlayer(dmap, mc, **p)) {
            int x, y;
            getRelativePos(**p, &dmap, mc, x, y);
            (*p)->getDungeonView().setItem(x, y, graphic, true);
        }
    }
}

void ViewManager::setTileIfVisible(const DungeonMap &dmap, const MapCoord &mc, int depth,
                                   const Graphic * graphic, shared_ptr<const ColourChange> cc,
                                   MiniMapColour col)
{
    if (!dmap.getRoomMap()) return;
    for (vector<Player*>::iterator p = players.begin(); p != players.end(); ++p) {
        invalidateSquare(**p, *dmap.getRoomMap(), mc);
        if (squareVisibleToPlayer(dmap, mc, **p)) {
            int x, y;
            getRelativePos(**p, &dmap, mc, x, y);
            (*p)->getDungeonView().setTile(x, y, depth, graphic, cc, true);
        }
    }
}


//
// event handler routines for items and tiles
// (these just call setItemIfVisible or setTileIfVisible)
//

void ViewManager::onAddItem(const DungeonMap &dm, const MapCoord &mc, const Item & i)
{
    setItemIfVisible(dm, mc, i.getGraphic());
}

void ViewManager::onRmItem(const DungeonMap &dm, const MapCoord &mc, const Item & i)
{
    setItemIfVisible(dm, mc, 0);
}

void ViewManager::onChangeItemGraphic(const DungeonMap &dm, const MapCoord &mc,
                                      const Item & i)
{
    setItemIfVisible(dm, mc, i.getGraphic());
}

void ViewManager::onAddTile(const DungeonMap &dm, const MapCoord &mc, const Tile & t)
{
    setTileIfVisible(dm, mc, t.getDepth(), t.getGraphic(), t.getColourChange(),
                     t.getColour());
}

void ViewManager::onRmTile(const DungeonMap &dm, const MapCoord &mc, const Tile & t)
{
    setTileIfVisible(dm, mc, t.getDepth(), 0, shared_ptr<const ColourChange>(),
                     t.getColour());
}

void ViewManager::onChangeTile(const DungeonMap &dm, const MapCoord &mc, const Tile & t)
{
    setTileIfVisible(dm, mc, t.getDepth(), t.getGraphic(), t.getColourChange(),
                     t.getColour());
}

void ViewManager::placeIcon(const DungeonMap &dm, const MapCoord &mc, const Graphic *g,
                            int dur)
{
    for (vector<Player*>::iterator p = players.begin(); p != players.end(); ++p) {
        if (squareVisibleToPlayer(dm, mc, **p)) {
            int x, y;
            getRelativePos(**p, &dm, mc, x, y);
            (*p)->getDungeonView().placeIcon(x, y, g, dur);
        }
    }
}

void ViewManager::flashScreen(shared_ptr<Entity> ent, int delay)
{
    for (vector<Player*>::iterator p = players.begin(); p != players.end(); ++p) {
        if (entityVisibleToPlayer(*ent, **p, true)) {
            // NB using trueview in the above, because even invisible knights
            // in the same room should cause the screen to flash!
            Mediator::instance().getCallbacks().flashScreen((*p)->getPlayerNum(), delay);
        }
    }
}

void ViewManager::playSound(DungeonMap &dm, const MapCoord &mc, const Sound &sound, int frequency, bool all)
{
    for (vector<Player*>::iterator p = players.begin(); p != players.end(); ++p) {
        if (all || squareVisibleToPlayer(dm, mc, **p)) {
            Mediator::instance().getCallbacks().playSound((*p)->getPlayerNum(), sound, frequency);
        }
    }
}

void ViewManager::sendCurrentRoom(int player, DungeonView &dview)
{
    if (player < 0 || player >= players.size()) return;

    const Player &p = *players[player];

    if (p.getKnight() && p.getKnight()->getMap()) {
        const DungeonMap &dmap = *p.getKnight()->getMap();
        if (dmap.getRoomMap()) {

            // Current Room
            const int current_room = p.getCurrentRoom();

            MapCoord top_left;
            int width, height;
            dmap.getRoomMap()->getRoomLocation(current_room, top_left, width, height);

            dview.setCurrentRoom(current_room, width, height);

            // Entities
            for (map<weak_ptr<Entity>, unsigned short int>::const_iterator it = id_map[player].begin(); 
            it != id_map[player].end(); ++it) {
                shared_ptr<Entity> ent = it->first.lock();
                if (ent) {
                    reallyAddEntity(player, ent, it->second, dview);
                }
            }

            // Item/Tile information
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    uploadSquare(dview, dmap, MapCoord(x + top_left.getX(), y + top_left.getY()), x, y, false);
                }
            }
        }
    }
}

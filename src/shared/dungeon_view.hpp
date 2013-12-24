/*
 * dungeon_view.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2013.
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
 * Interface to a view of the dungeon as seen by a given player.
 *
 */

#ifndef DUNGEON_VIEW_HPP
#define DUNGEON_VIEW_HPP

#include "map_support.hpp"
#include "mini_map_colour.hpp"

#include <string>

class Anim;
class ColourChange;
class Graphic;
class Overlay;

class DungeonView {
public:
    virtual ~DungeonView() { }
    
    //
    // Room Control
    //
    // The DungeonView remembers tiles and items from different rooms,
    // so when the player revisits a previous room, the tiles and
    // items don't have to be downloaded again. Entities are never
    // remembered though -- the server will resend all entities (using
    // addEntity) after calling setCurrentRoom.
    //
    // NB: setCurrentRoom(-1) means display a blank screen (eg if the
    // player has died).
    //

    virtual void setCurrentRoom(int r, int width, int height) = 0;


    //
    // Entities
    //

    // All entities are assigned an "id" number by the server.
    // id==0 is reserved for "my knight".
    
    // NOTE for setAnimData and addEntity: if atz_diff==0 this means
    // anim_tzero == 0 and if atz_diff>0 this means anim_tzero ==
    // atz_diff + current time.
    
    virtual void addEntity(unsigned short int id, int x, int y, MapHeight ht, MapDirection facing,
                           const Anim * anim, const Overlay *ovr, int af, int atz_diff,
                           bool ainvis, bool ainvuln, // (anim data)
                           int cur_ofs, MotionType motion_type, int motion_time_remaining,
                           const std::string &name) = 0;
    virtual void rmEntity(unsigned short int id) = 0;

    // move entity to a new square (instantaneously)
    virtual void repositionEntity(unsigned short int id, int new_x, int new_y) = 0;

    // start an entity moving (continuously) between two squares.
    virtual void moveEntity(unsigned short int id, MotionType motion_type,
                            int motion_duration, bool missile_mode) = 0;

    // reverse the direction of a move-in-progress.
    virtual void flipEntityMotion(unsigned short int id, int initial_delay, int motion_duration) = 0;
    
    virtual void setAnimData(unsigned short int id, const Anim *, const Overlay *, int af,
                             int atz_diff, bool ainvis, bool ainvuln, bool currently_moving) = 0;
    virtual void setFacing(unsigned short int id, MapDirection new_facing) = 0;
    virtual void setSpeechBubble(unsigned short int id, bool show) = 0;

    //
    // Tiles
    //
    
    virtual void clearTiles(int x, int y, bool force) = 0;  // wipe all tiles at a coord.
    virtual void setTile(int x, int y, int depth, const Graphic * gfx, boost::shared_ptr<const ColourChange> cc, bool force) = 0;

    
    //
    // Items
    //
    
    virtual void setItem(int x, int y, const Graphic * gfx, bool force) = 0;


    //
    // "Icons" (used for blood splats)
    //
    
    virtual void placeIcon(int x, int y, const Graphic *g, int dur) = 0;


    //
    // Messages
    //

    virtual void flashMessage(const std::string &msg, int ntimes) = 0;
    virtual void cancelContinuousMessages() = 0;
    virtual void addContinuousMessage(const std::string &msg) = 0;
};

#endif

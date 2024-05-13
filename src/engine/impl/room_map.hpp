/*
 * room_map.hpp
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
 * "Rooms" are stored as rectangles with a top-left corner position
 * and a width and height. The definition of the room includes its
 * border (but NOT the four corner squares), therefore rooms overlap
 * each other by one square around the edges. Each square in the
 * dungeon is part of either zero, one or two rooms.
 * 
 */

#ifndef ROOM_MAP_HPP
#define ROOM_MAP_HPP

#include "map_support.hpp"

#include <vector>

class RoomMap {
public:
    // construction
    RoomMap();
    void addRoom(const MapCoord &top_left, int w, int h);
    void doneAddingRooms();  // call once all rooms are added.

    // "getRoomAtPos" returns the room(s) associated with a square. If
    // it's a border square, 2 room numbers are returned. If it's an
    // interior square, one room number will be returned, and r2 will
    // be set to -1. (Otherwise both r1 and r2 will be set to -1.)
    void getRoomAtPos(const MapCoord &mc, int &r1, int &r2) const;

    // "isCorner" checks whether the square is one of the four corners
    // of some room.
    bool isCorner(const MapCoord &mc) const;
    
    // "inSameRoom" checks whether two given mapcoords are in the same room.
    bool inSameRoom(const MapCoord &mc1, const MapCoord &mc2) const;

    // "getRoomLocation" looks up the pos & size of a numbered room.
    void getRoomLocation(int r, MapCoord &top_left, int &w, int &h) const;
        
private:
    struct RoomInfo {
        MapCoord pos;
        int w, h;
    };
    std::vector<RoomInfo> rooms;
    bool ready; // set once "doneAddingRooms" has been called.
};

#endif

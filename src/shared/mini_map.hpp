/*
 * mini_map.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2025.
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
 * Interface for updates to a mini-map.
 *
 */

#ifndef MINI_MAP_HPP
#define MINI_MAP_HPP

#include "mini_map_colour.hpp"

class MiniMap {
public:
    virtual ~MiniMap() { }

    // Set the size of the mini-map. Should be called first.
    virtual void setSize(int width, int height) = 0;
    
    // Set colour of individual squares.
    // 
    // NOTE: When mapping an entire room, it is best to map in
    // horizontal 'runs' (ie loop over y first, then x). This is for
    // efficiency reasons (see ServerMiniMap).
    virtual void setColour(int x, int y, MiniMapColour col) = 0;
    
    // Set all square-colours to COL_UNMAPPED
    virtual void wipeMap() = 0;
    
    // Show a knight on the mini-map.
    // n is the player number (from 0 to NPLYRS-1); x,y is the
    // position, or set to (-1,-1) if knight is not to be shown.
    virtual void mapKnightLocation(int n, int x, int y) = 0;
    
    // Switch on or off an "item indicator" on the mini-map at
    // a given location. (used for sense items.)
    virtual void mapItemLocation(int x, int y, bool on) = 0;
};

#endif

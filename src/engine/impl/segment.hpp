/*
 * segment.hpp
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

/*
 * "Segment" represents a rectangular dungeon section, used by the dungeon
 * generator.
 *
 */

#ifndef SEGMENT_HPP
#define SEGMENT_HPP

#include "map_support.hpp"

#include "boost/shared_ptr.hpp"
using namespace boost;

#include <list>
#include <vector>
using namespace std;

class DungeonMap;
class ItemType;
class Tile;

struct lua_State;

struct HomeInfo {
    int x;
    int y; 
    MapDirection facing; // points inwards, towards the home
};


class Segment {
public:
    Segment(int w, int h) : data(w*h), width(w), height(h) { }

    // get width and height
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // add tile or item (can be called more than once per square)
    // x,y range from 0 to w-1, h-1, and increase rightwards and upwards as usual.
    void addTile(int x, int y, shared_ptr<Tile> t);
    void addItem(int x, int y, const ItemType * i);
        
    // set a room (coords are relative to bottom left of segment -- note that the
    // outer boundary wall is not counted as part of the segment).
    void addRoom(int blx, int bly, int w, int h);

    // get/set "bat placement tile" (used by the dungeon generator)
    void setBatPlacementTile(shared_ptr<Tile> t) { bat_placement_tile = t; }
    shared_ptr<Tile> getBatPlacementTile() const { return bat_placement_tile; }
        
    // copy the tiles and items into a dungeon map
    // The tiles replace any that were already there.
    // This routine also adds appropriate rooms to the RoomMap.
    //
    // #41: Reflections and rotations are now supported.
    // Transformation applied is R^nrot * X
    // where R=clockwise rotation of 90 degrees
    // X=x reflection (only applied if x_reflect==true).
    //
    void copyToMap(DungeonMap &dmap, const MapCoord &bottom_left,
                   bool x_reflect, int nrot) const;

    // access to homes
    vector<HomeInfo> getHomes(bool xreflect, int nrot) const;
    int getNumHomes() const { return int(homes.size()); }
    

    // see if a given square is approachable. (based on the addTile calls so far.)
    bool isApproachable(int x, int y) const;
    
private:
    // (TODO) might be better for segments to store tile data in a standard pointer -- would
    // halve memory requirements. (ServerConfig could look after the pointers.) However there
    // might be complications with switch tiles?
    vector<list<shared_ptr<Tile> > > data;
    vector<HomeInfo> homes;
    int width;
    int height;
    struct RoomInfo {
        int tlx, tly, w, h;
    };
    vector<RoomInfo> rooms;
    struct ItemInfo {
        int x;
        int y;
        const ItemType * itype;
    };
    vector<ItemInfo> items;
    shared_ptr<Tile> bat_placement_tile;
};

#endif

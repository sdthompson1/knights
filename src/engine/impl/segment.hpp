/*
 * segment.hpp
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
 * "Segment" represents a rectangular dungeon section, used by the dungeon
 * generator.
 *
 */

#ifndef SEGMENT_HPP
#define SEGMENT_HPP

#include "map_support.hpp"

#include "boost/shared_ptr.hpp"

#include <iosfwd>
#include <list>
#include <string>
#include <vector>

class DungeonMap;
class ItemType;
class MonsterManager;
class MonsterType;
class Tile;

struct lua_State;


struct HomeInfo {
    int x;    // The x,y of the home itself
    int y; 
    MapDirection facing; // points inwards, towards the home
    bool special_exit;
};


class Segment {
public:
    // This creates an empty segment of a given size.
    Segment(int w, int h) : data(w*h), width(w), height(h) { }

    // This loads segment data from a text file.
    // The lua state should have a "tile table" on top of stack. (The stack is unchanged on exit.)
    // NOTE: It's assumed this is called inside a lua pcall, so it may raise lua errors.
    Segment(std::istream &str, lua_State *lua);
    
    // get width and height
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // add tile or item (can be called more than once per square)
    // x,y range from 0 to w-1, h-1, and increase rightwards and upwards as usual.
    void addTile(int x, int y, boost::shared_ptr<Tile> t);
    void addItem(int x, int y, ItemType * i, int num);
    void addMonster(int x, int y, const MonsterType * m);
        
    // set a room (coords are relative to bottom left of segment -- note that the
    // outer boundary wall is not counted as part of the segment).
    void addRoom(int blx, int bly, int w, int h);

    // copy the tiles and items into a dungeon map
    // The tiles replace any that were already there.
    // This routine also adds appropriate rooms to the RoomMap.
    //
    // #41: Reflections and rotations are now supported.
    // Transformation applied is R^nrot * X
    // where R=clockwise rotation of 90 degrees
    // X=x reflection (only applied if x_reflect==true).
    //
    void copyToMap(DungeonMap &dmap, MonsterManager &monster_manager,
                   const MapCoord &bottom_left,
                   bool x_reflect, int nrot) const;

    // access to homes
    std::vector<HomeInfo> getHomes(bool xreflect, int nrot) const;
    int getNumHomes(bool include_special) const;
    

    // see if a given square is approachable. (based on the addTile calls so far.)
    bool isApproachable(int x, int y) const;

private:
    void readTable(lua_State *lua, int x, int y);
    void readTile(lua_State *lua, int x, int y, bool top_level);
    void readSquare(lua_State *lua, int x, int y, int n);
    void loadData(std::istream &str, lua_State *lua);
    void loadRooms(std::istream &str, lua_State *lua);
    void loadSwitches(std::istream &str, lua_State *lua);
    bool readLine(std::istream &str, lua_State *lua, std::string &key, std::string &value);
    
private:
    // (TODO) might be better for segments to store tile data in a
    // standard pointer -- would halve memory requirements.
    // (KnightsConfigImpl could look after the pointers.) However
    // there might be complications with switch tiles?
    std::vector<std::list<boost::shared_ptr<Tile> > > data;
    std::vector<HomeInfo> homes;
    int width;
    int height;

    struct RoomInfo {
        int tlx, tly, w, h;
    };
    std::vector<RoomInfo> rooms;

    struct ItemInfo {
        int x;
        int y;
        ItemType * itype;
        int num;
    };
    std::vector<ItemInfo> items;

    struct MonsterInfo {
        int x;
        int y;
        const MonsterType *mtype;
    };
    std::vector<MonsterInfo> monsters;
};

#endif

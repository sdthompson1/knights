/*
 * segment.cpp
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
#include "item.hpp"
#include "lua_userdata.hpp"
#include "monster_manager.hpp"
#include "room_map.hpp"
#include "segment.hpp"
#include "special_tiles.hpp"  // for Home
#include "tile.hpp"

#include "lua.hpp"

#include <cctype>
#include <iostream>

namespace {
    std::string Trim(const std::string &s)
    {
        size_t beg = 0;
        while (beg < s.size()) {
            if (!std::isspace(s[beg])) break;
            ++beg;
        }
        // 'beg' is now the first non-space character
        // (or size() if none such exists)

        size_t end = s.size();
        while (end != beg) {
            if (!std::isspace(s[end-1])) break;
            --end;
        }
        // 'end' is now one plus the last non-space character,
        // or equal to beg, whichever is greater

        return s.substr(beg, end - beg);
    }        
    
    MapCoord TransformMapCoord(int size, bool x_reflect, int nrot, const MapCoord &top_left, int x, int y)
    {
        // apply reflection
        if (x_reflect) {
            x = size - 1 - x;
        }

        // apply rotations
        for (int i = 0; i < nrot; ++i) {
            const int x_old = x;
            x = size - 1 - y;
            y = x_old;
        }

        // return final coordinates
        return MapCoord(x + top_left.getX(), y + top_left.getY());
    }    
}

void Segment::readTable(lua_State *lua, int x, int y)
{
    // [tbl]
    lua_len(lua, -1);  // [tbl len]
    const int sz = lua_tointeger(lua, -1);  // [tbl]
    lua_pop(lua, 1);  // [tbl]

    for (int i = 1; i <= sz; ++i) {
        lua_pushinteger(lua, i);  // [tbl i]
        lua_gettable(lua, -2);    // [tbl value]

        if (!lua_isnil(lua, -1)) {

            if (IsLuaPtr<Tile>(lua, -1)) {
                addTile(x, y, ReadLuaSharedPtr<Tile>(lua, -1));

            } else if (IsLuaPtr<const ItemType>(lua, -1)) {
                addItem(x, y, ReadLuaPtr<const ItemType>(lua, -1));

            } else if (IsLuaPtr<MonsterType>(lua, -1)) {
                addMonster(x, y, ReadLuaPtr<MonsterType>(lua, -1));

            } else {
                // try reading it as a table
                readTable(lua, x, y);
            }
        }

        lua_pop(lua, 1);  // [tbl]
    }
}

void Segment::readSquare(lua_State *lua, int x, int y, int n)
{
    // [t]
    lua_pushinteger(lua, n);  // [t n]
    lua_gettable(lua, -2);    // [t value]

    if (lua_isnil(lua, -1)) {
        luaL_error(lua, "Error in segment file, tile %d does not exist", n);
    } else {
        readTable(lua, x, y);
        lua_pop(lua, 1);  // [t]
    }
}
        
void Segment::loadData(std::istream &str, lua_State *lua)
{
    if (width <= 0 || height <= 0) {
        luaL_error(lua, "Error in segment file, 'width' and 'height' must come before 'data'");
    }

    data.resize(width * height);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int n = -1;
            str >> n;
            if (!str || n<0) luaL_error(lua, "Error in segment file, bad 'data'");
            readSquare(lua, x, y, n);
        }
    }
}

void Segment::loadRooms(std::istream &str, lua_State *lua)
{
    int num = -1;
    str >> num;
    if (!str || num < 0) luaL_error(lua, "Error while loading segment: 'rooms' invalid");

    rooms.reserve(num);
    
    for (int i = 0; i < num; ++i) {
        int tlx, tly, w, h;
        str >> tlx >> tly >> w >> h;
        if (!str) luaL_error(lua, "Error while loading segment: 'rooms' invalid");
        addRoom(tlx, tly, w, h);
    }
}    

bool Segment::readLine(std::istream &str, lua_State *lua, std::string &key, std::string &value)
{
    std::string s;
    std::getline(str, s);

    s = Trim(s);
    if (s == "end") return true;

    size_t pos = s.find(':');
    if (pos == string::npos) luaL_error(lua, "Error while loading segment: ':' expected");
    key = s.substr(0,pos);
    value = s.substr(pos+1);

    key = Trim(key);
    value = Trim(value);
    return false;
}
    
Segment::Segment(std::istream &str, lua_State *lua)
        : width(0), height(0)
{
    while (1) {
        std::string key, value;
        if (readLine(str, lua, key, value)) {
            break;
        } else {

            if (key == "data") {
                loadData(str, lua);

            } else if (key == "width") {
                width = std::atoi(value.c_str());
                if (width <= 0) luaL_error(lua, "Invalid segment width: %d", width);

            } else if (key == "height") {
                height = std::atoi(value.c_str());
                if (height <= 0) luaL_error(lua, "Invalid segment height: %d", height);

            } else if (key == "rooms") {
                loadRooms(str, lua);

            } else {
                luaL_error(lua, "Error while loading segment: '%s' unrecognized", key.c_str());
            }
            
        }
    }

    if (width <= 0 || height <= 0 || rooms.empty() || data.empty()) {
        luaL_error(lua, "Invalid segment, not all values have been set");
    }
}

void Segment::addTile(int x, int y, shared_ptr<Tile> t)
{
    if (!t) return;
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    data[y*width+x].push_front( t );

    shared_ptr<Home> h = dynamic_pointer_cast<Home>(t);
    if (h) {
        HomeInfo hi;
        hi.x = x;
        hi.y = y;
        hi.facing = h->getFacing();
        hi.special_exit = h->isSpecialExit();
        homes.push_back(hi);
    }
}

void Segment::addItem(int x, int y, const ItemType *itype)
{
    if (!itype) return;
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    ItemInfo ii;
    ii.x = x;
    ii.y = y;
    ii.itype = itype;
    items.push_back(ii);
}

void Segment::addMonster(int x, int y, const MonsterType *mtype)
{
    if (!mtype) return;
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    MonsterInfo mi;
    mi.x = x;
    mi.y = y;
    mi.mtype = mtype;
    monsters.push_back(mi);
}

void Segment::addRoom(int tlx, int tly, int w, int h)
{
    RoomInfo ri = { tlx, tly, w, h };
    rooms.push_back(ri);
}

void Segment::copyToMap(DungeonMap &dmap, MonsterManager &monster_manager,
                        const MapCoord &top_left,
                        bool x_reflect, int nrot) const
{
    using boost::shared_ptr;
    using std::vector;
    
    // If transformations are applied then the segment must be square
    ASSERT(width == height || (!x_reflect && nrot == 0));

    // nrot is supposed to be between 0 and 3
    ASSERT(nrot >= 0 && nrot <= 3);
    
    for (int i=0; i<width; ++i) {
        for (int j=0; j<height; ++j) {
            MapCoord mc = TransformMapCoord(width, x_reflect, nrot, top_left, i, j);
            dmap.clearTiles(mc);
            const list<shared_ptr<Tile> > &tiles( data[j*width+i] );
            for (list<shared_ptr<Tile> >::const_iterator it = tiles.begin();
            it != tiles.end(); ++it) {

                shared_ptr<Tile> t = *it;
                if (x_reflect) t = t->getReflect();
                for (int k = 0; k < nrot; ++k) t = t->getRotate();
                
                shared_ptr<Tile> tile(t->clone(false));
                dmap.addTile(mc, tile, Originator(OT_None()));
            }
        }
    }

    for (vector<ItemInfo>::const_iterator it = items.begin(); it != items.end(); ++it) {
        shared_ptr<Item> item(new Item(*it->itype));
        MapCoord mc = TransformMapCoord(width, x_reflect, nrot, top_left, it->x, it->y);
        dmap.addItem(mc, item);
    }

    for (vector<MonsterInfo>::const_iterator it = monsters.begin(); it != monsters.end(); ++it) {
        MapCoord mc = TransformMapCoord(width, x_reflect, nrot, top_left, it->x, it->y);
        monster_manager.placeMonster(*it->mtype, dmap, mc, D_NORTH);
    }
    
    if (dmap.getRoomMap()) {
        for (vector<RoomInfo>::const_iterator it = rooms.begin(); it != rooms.end(); ++it) {
            // get the two opposite room corners (inclusive)
            const MapCoord corner1 = TransformMapCoord(width, x_reflect, nrot, top_left, it->tlx, it->tly);
            const MapCoord corner2 = TransformMapCoord(width, x_reflect, nrot, top_left, it->tlx + it->w - 1, it->tly + it->h - 1);

            // if rotations or reflections were applied then we need to work out where the new top left is, and
            // what the new width and height are.
            const int min_x = std::min(corner1.getX(), corner2.getX());
            const int max_x = std::max(corner1.getX(), corner2.getX());
            const int min_y = std::min(corner1.getY(), corner2.getY());
            const int max_y = std::max(corner1.getY(), corner2.getY());
            
            dmap.getRoomMap()->addRoom(MapCoord(min_x, min_y), max_x - min_x + 1, max_y - min_y + 1);
        }
    }
}

bool Segment::isApproachable(int x, int y) const
{
    using boost::shared_ptr;
    using std::list;
    
    const list<shared_ptr<Tile> > & tiles = data[y*width+x];
    MapAccess acc = A_CLEAR;
    for (list<shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
        const MapAccess a = (*it)->getAccess(H_WALKING);
        if (a < acc) acc = a;
    }
    return (acc == A_APPROACH);
}

std::vector<HomeInfo> Segment::getHomes(bool x_reflect, int nrot) const
{
    using std::vector;
    
    vector<HomeInfo> result = homes;
    for (vector<HomeInfo>::iterator it = result.begin(); it != result.end(); ++it) {

        if (x_reflect) {
            if (it->facing == D_WEST) it->facing = D_EAST;
            else if (it->facing == D_EAST) it->facing = D_WEST;
            it->x = width - 1 - it->x;
        }

        for (int i = 0; i < nrot; ++i) {
            it->facing = Clockwise(it->facing);
            const int x_old = it->x;
            it->x = width - 1 - it->y;
            it->y = x_old;
        }
    }

    return result;
}

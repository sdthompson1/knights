/*
 * segment.cpp
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

#include "dungeon_map.hpp"
#include "item.hpp"
#include "room_map.hpp"
#include "segment.hpp"
#include "special_tiles.hpp"  // for Home
#include "tile.hpp"

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

void Segment::addRoom(int tlx, int tly, int w, int h)
{
    RoomInfo ri = { tlx, tly, w, h };
    rooms.push_back(ri);
}

void Segment::copyToMap(DungeonMap &dmap, const MapCoord &top_left) const
{
    for (int i=0; i<width; ++i) {
        for (int j=0; j<height; ++j) {
            MapCoord mc(i+top_left.getX(), j+top_left.getY());
            dmap.clearTiles(mc);
            const list<shared_ptr<Tile> > &tiles( data[j*width+i] );
            for (list<shared_ptr<Tile> >::const_iterator it = tiles.begin();
                 it != tiles.end(); ++it) {
                shared_ptr<Tile> tile((*it)->clone(false));
                dmap.addTile(mc, tile, 0);
            }
        }
    }

    for (vector<ItemInfo>::const_iterator it = items.begin(); it != items.end(); ++it) {
        shared_ptr<Item> item(new Item(*it->itype));
        MapCoord mc(it->x + top_left.getX(), it->y + top_left.getY());
        dmap.addItem(mc, item);
    }
        
    if (dmap.getRoomMap()) {
        for (vector<RoomInfo>::const_iterator it = rooms.begin(); it != rooms.end(); ++it) {
            dmap.getRoomMap()->addRoom(MapCoord(it->tlx + top_left.getX(),
                                                it->tly + top_left.getY()),
                                       it->w, it->h);
        }
    }
}

bool Segment::isApproachable(int x, int y) const
{
    const list<shared_ptr<Tile> > & tiles = data[y*width+x];
    MapAccess acc = A_CLEAR;
    for (list<shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
        const MapAccess a = (*it)->getAccess(H_WALKING);
        if (a < acc) acc = a;
    }
    return (acc == A_APPROACH);
}

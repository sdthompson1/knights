/*
 * item.cpp
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
 * Some useful functions for dropping items into the map
 *
 */

#include "misc.hpp"

#include "dungeon_map.hpp"
#include "item.hpp"
#include "mediator.hpp"
#include "tile.hpp"

#include <deque>
#include <set>
using namespace std;

namespace {

    // AddMapTile - helper function for GetDropSquare.
    void AddMapTile(const DungeonMap &dmap, const ItemType &drop_item, shared_ptr<Item> curr_item,
                    deque<MapCoord> &open, const MapCoord &mc)
    {
        if (!dmap.valid(mc)) return;

        // Note slight inefficiency - CheckDropSquare ends up getting called twice on many squares. Never mind.
        const bool can_drop = CheckDropSquare(dmap, mc, drop_item, curr_item);
        
        // If CheckDropSquare returns false then it could be because:
        // (i) item already present. In which case we can add to open list
        // (ii) there is a wall or other blocked tile present. In which case we don't add to open list
        if (can_drop || dmap.getItem(mc)) {
            open.push_back(mc);
        }
    }


    // GetDropSquare -- find a square (either "base", or one of the
    // four adjacent squares) on which an item of type "drop_item" can
    // be dropped. (Also returns the item currently on the chosen
    // square.)
    // Added 31-Aug-2007: If "allow_nonlocal" is true then additional
    // nearby squares will be searched as well.
    // Added 31-Dec-2010: If "shift_forward" is set to true then we 
    // will prefer to drop on the square in front (if it is approachable).
    // Otherwise we will prefer the current square.

    MapCoord GetDropSquare(const DungeonMap &dmap, const MapCoord &base,
                           bool allow_nonlocal, bool shift_forward,
                           MapDirection preferred_direction,
                           const ItemType &drop_item, shared_ptr<Item> &curr_item)
    {
        if (shift_forward) {
            // In this case prefer dropping on the square ahead if possible (if it is approachable).
            // Just do this as an extra check right at the beginning.
            const MapCoord ahead = DisplaceCoord(base, preferred_direction);
            if (dmap.getAccessTilesOnly(ahead, H_WALKING) == A_APPROACH
            && CheckDropSquare(dmap, ahead, drop_item, curr_item)) {
                return ahead;
            }
        }

        deque<MapCoord> open;
        set<MapCoord> closed;

        open.push_back(base);

        bool add_more_squares = true;

        while (!open.empty()) {
            // get next tile
            const MapCoord mc = open.front();
            open.pop_front();

            // ignore it if it's already been looked at
            if (closed.find(mc) != closed.end()) continue;

            // try this square
            if (CheckDropSquare(dmap, mc, drop_item, curr_item)) return mc;

            // failed here, so go on to next square
            closed.insert(mc);

            if (add_more_squares) {
                AddMapTile(dmap, drop_item, curr_item, open, DisplaceCoord(mc, preferred_direction));
                AddMapTile(dmap, drop_item, curr_item, open, DisplaceCoord(mc, Clockwise(preferred_direction)));
                AddMapTile(dmap, drop_item, curr_item, open, DisplaceCoord(mc, Anticlockwise(preferred_direction)));
                AddMapTile(dmap, drop_item, curr_item, open, DisplaceCoord(mc, Opposite(preferred_direction)));
                if (!allow_nonlocal) add_more_squares = false;  // Don't add anything beyond the first four
            }
        }

        // No drop squares are available!
        return MapCoord();
    }

}


bool CheckDropSquare(const DungeonMap &dmap, const MapCoord &mc, 
                     const ItemType &drop_item, shared_ptr<Item> &curr_item)
{
    curr_item = dmap.getItem(mc); // item currently at the square
    if (!dmap.valid(mc)) return false;   // can't drop outside the map!
    if (curr_item) {
        // check if we can stack, otherwise return false.
        if (&curr_item->getType() != &drop_item) return false; 
        if (curr_item->getNumber() == curr_item->getType().getMaxStack()) return false;
        return true;
    } else {
        // No item present -- check if a Tile forbids items from being dropped here.
        vector<shared_ptr<Tile> > tiles;
        dmap.getTiles(mc, tiles);
        return (find_if(tiles.begin(), tiles.end(), BlocksItems()) == tiles.end());
    }
}


bool CanDropItem(const ItemType &drop_item, DungeonMap &dmap, const MapCoord &mc,
                 bool shift_forward, MapDirection preferred_direction)
{
    shared_ptr<Item> dummy;
    MapCoord dest = GetDropSquare(dmap, mc, false, shift_forward, preferred_direction, drop_item, dummy);
    return (!dest.isNull());
}

bool DropItem(shared_ptr<Item> drop_item, DungeonMap &dmap, const MapCoord &mc,
              bool allow_nonlocal, bool shift_forward,
              MapDirection preferred_direction,
              shared_ptr<Creature> actor, MapCoord *drop_mc)
{
    if (!drop_item) return false;           // no item was given
    
    shared_ptr<Item> curr_item;
    Mediator &mediator(Mediator::instance());
    
    while (1) {
        MapCoord dest = GetDropSquare(dmap, mc, allow_nonlocal, shift_forward,
                                      preferred_direction, drop_item->getType(), curr_item);
        if (dest.isNull()) {
            // Can't drop at all
            return false;
        } else if (curr_item) {
            // Found an existing stack
            const Graphic * old_graphic = curr_item->getGraphic();
            int new_number = curr_item->getNumber() + drop_item->getNumber();
            int max_stack = drop_item->getType().getMaxStack();
            bool drop_all;
            if (new_number > max_stack) {
                // We can drop some, but not all, into the existing stack
                curr_item->setNumber(max_stack);
                drop_item->setNumber(new_number - max_stack);
                drop_all = false;
            } else {
                // The whole lot can be dropped into an existing stack
                curr_item->setNumber(new_number);
                drop_item->setNumber(0);
                drop_all = true;
            }
            if (curr_item->getGraphic() != old_graphic) {
                mediator.onChangeItemGraphic(dmap, dest, *curr_item);
            }
            if (actor) {
                // do the "onDrop" event
                curr_item->getType().onDrop(dmap, dest, actor);
            }
            if (drop_all) {
                if (drop_mc) *drop_mc = dest;
                return false;
            } else {
                continue;
            }
        } else {
            // The square is empty -- we can just drop outright.
            dmap.addItem(dest, drop_item);
            if (actor) {
                // do the "onDrop" event
                drop_item->getType().onDrop(dmap, dest, actor);
            }
            if (drop_mc) *drop_mc = dest;
            return true;
        }
    }
    ASSERT(0);
    return false;
}

Item::Item(const ItemType &t, int no)
    : type(t), number(no)
{
    if (number > type.getMaxStack()) number = type.getMaxStack();
    if (number < 0) number = 0;
}

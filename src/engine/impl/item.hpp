/*
 * item.hpp
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

/*
 * Items are things that can be picked up, eg crossbows, hammers,
 * potions, scrolls, traps, daggers, keys.
 *
 * Each item has an ItemType describing what sort of item it is, a
 * graphic, and a number (some items can come in piles of more than
 * one at a time, eg daggers, crossbow bolts). Since items do not
 * carry any state information (eg. all hammers are identical) this is
 * not a problem, and it saves a bit of memory in some cases.
 *
 */

#ifndef ITEM_HPP
#define ITEM_HPP

#include "item_type.hpp"
#include "originator.hpp"

//
// Item
//

class Item {
public:
    // The ItemType is assumed not to be deleted until the end of the
    // game. Also, an individual item never changes its ItemType.
    explicit Item(const ItemType &t, int no = 1);

    // ItemTypes can be compared by taking their addresses (eg &i1.getType() == &i2.getType())
    const ItemType & getType() const { return type; }

    // get/set the number in the stack.
    int getNumber() const { return number; }
    void setNumber(int n) { if (n >= 0 && n <= type.getMaxStack()) number = n; }

    // get graphic.
    const Graphic * getGraphic() const { 
        return number > 1 ? type.getStackGraphic() : type.getSingleGraphic(); 
    }

    // get/set player who "owns" this item. Used for bear traps.
    const Originator &getOwner() const { return owner; }
    void setOwner(const Originator &o) { owner = o; }
    
private:
    const ItemType &type;
    int number;
    Originator owner;
};


//
// Free functions
//


// CheckDropSquare -- check if a given itemtype can be dropped onto a
// given square. Returns true if it can. Also returns a ptr to the
// item currently at the square if any.

bool CheckDropSquare(const DungeonMap &dmap, const MapCoord &mc,
                     const ItemType &drop_item, shared_ptr<Item> &curr_item);



// CanDropItem: can an item of the given type be dropped into the
// given square or one of the four surrounding squares?

// preferred_direction: indicates which of the four surrounding squares
// you would prefer the item to be dropped into (if it can't be dropped in the
// given square) -- set to anything (e.g. D_NORTH) if you don't care.

// shift_forward: If true, we will PREFER to drop in the preferred_direction, over the base
// square, IF the tile in the preferred direction is approachable. (This is used when a player
// drops an item into a chest for example.)
// Otherwise, we will prefer the base square, THEN the square in the preferred direction.

bool CanDropItem(const ItemType &drop_item, DungeonMap &dmap, const MapCoord &mc,
                 bool shift_forward, MapDirection preferred_direction);
    

// DropItem: adds the given item to the map at the given square or one
// of the four surrounding squares. If "allow_nonlocal" is true then
// additional nearby squares will be considered as well. If "actor" is
// non-null then the "onDrop" event is run.

// Returns true if the item was added to the map.

// Returns false if the item was not added to the map, but instead we
// decreased the number of the item, and increased the number of some
// item already in the map. (This is used when dropping onto an
// existing stack of items.) NB The "drop_item" could come back with
// number==0 (if the whole stack was added to an existing stack).

// Also returns false if the drop could not succeed at all. (In this
// case drop_item->getNumber() will be unchanged, and no changes are
// made to the map either.)

// preferred_direction: see above.

// If drop_mc != 0 then *drop_mc will be set to the actual map square
// that the item was dropped onto. (If item was not dropped then it
// will be left unchanged.) (If the item was dropped into more than
// one square then only the last square is returned, this can happen
// with stackable items.)

bool DropItem(shared_ptr<Item> drop_item, DungeonMap &dmap, const MapCoord &mc,
              bool allow_nonlocal, bool shift_forward, MapDirection preferred_direction,
              shared_ptr<Creature> actor, MapCoord *drop_mc = 0);

#endif

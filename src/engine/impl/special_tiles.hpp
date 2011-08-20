/*
 * special_tiles.hpp
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
 * This file contains special Tile types (eg doors, chests, homes).
 *
 */

#ifndef SPECIAL_TILES_HPP
#define SPECIAL_TILES_HPP

#include "lockable.hpp"

#include <vector>
using namespace std;

class Door : public Lockable {
public:
    Door();
    void doorConstruct(const Graphic *og, const Graphic *cg, const MapAccess acc[]);

    virtual void damage(DungeonMap &, const MapCoord &, int amt, shared_ptr<Creature> actor);
    virtual void onHit(DungeonMap &, const MapCoord &, shared_ptr<Creature> actor, Player *);
    virtual bool targettable() const;
    
protected:
    virtual shared_ptr<Tile> doClone(bool);
    virtual void openImpl(DungeonMap &, const MapCoord &, Player *player);
    virtual void closeImpl(DungeonMap &, const MapCoord &, Player *player);
    
private:
    const Graphic *open_graphic;
    const Graphic *closed_graphic;
    MapAccess closed_access[H_MISSILES+1];
};


class Chest : public Lockable {
public:
    struct TrapInfo {
        TrapInfo(const ItemType *it, const Action *ac)
            : itype(it), action(ac) { }
        const ItemType *itype;
        const Action *action;  // action to place the trap ...
    };

    Chest() : open_graphic(0), closed_graphic(0), facing(D_NORTH), trap_chance(0) { }
    void chestConstruct(const Graphic *og, const Graphic *cg,
                        MapDirection f, int trap_chance_,
                        const vector<TrapInfo> &t)
    { open_graphic = og; closed_graphic = cg; facing = f; trap_chance = trap_chance_; traps = t; }

    virtual bool cannotActivateFrom(MapDirection &dir) const;
    
    virtual bool canPlaceItem() const;
    virtual shared_ptr<Item> getPlacedItem() const;
    virtual void placeItem(shared_ptr<Item>);
    virtual void onDestroy(DungeonMap &, const MapCoord &, shared_ptr<Creature>, Player *);

    virtual bool generateTrap(DungeonMap &, const MapCoord &);
    
protected:
    virtual shared_ptr<Tile> doClone(bool);
    virtual void openImpl(DungeonMap &, const MapCoord &, Player *);
    virtual void closeImpl(DungeonMap &, const MapCoord &, Player *);

private:
    const Graphic *open_graphic;
    const Graphic *closed_graphic;
    shared_ptr<Item> stored_item;

    MapDirection facing;
    int trap_chance; // as a percentage
    vector<TrapInfo> traps;
};


class Barrel : public Tile {
public:
    virtual bool canPlaceItem() const;
    virtual shared_ptr<Item> getPlacedItem() const;
    virtual void placeItem(shared_ptr<Item>);
    virtual void onDestroy(DungeonMap &, const MapCoord &, shared_ptr<Creature>, Player *);

protected:
    virtual shared_ptr<Tile> doClone(bool);

private:
    shared_ptr<Item> stored_item;
};

    
class Home : public Tile {
public:
    void homeConstruct(MapDirection fcg, shared_ptr<const ColourChange> cc_unsecured);
    
    // onApproach/onWithdraw are overridden to handle healing and quest-checking.
    virtual void onApproach(DungeonMap &, const MapCoord &, shared_ptr<Creature>, Player *);
    virtual void onWithdraw(DungeonMap &, const MapCoord &, shared_ptr<Creature>, Player *);

    // get facing -- the facing direction points inwards, towards the home.
    MapDirection getFacing() const { return facing; }

    // "secure" (this resets the colour-change).
    void secure(DungeonMap &, const MapCoord &, shared_ptr<const ColourChange> new_cc);
    
protected:
    virtual shared_ptr<Tile> doClone(bool);

private:
    MapDirection facing;
};

#endif

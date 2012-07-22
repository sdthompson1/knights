/*
 * special_tiles.hpp
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
 * This file contains special Tile types (eg doors, chests, homes).
 *
 */

#ifndef SPECIAL_TILES_HPP
#define SPECIAL_TILES_HPP

#include "lockable.hpp"
#include "lua_func.hpp"

#include <vector>
using namespace std;

class ItemType;

class Door : public Lockable {
public:
    Door(lua_State *lua, KnightsConfigImpl *kc);

    virtual void damage(DungeonMap &, const MapCoord &, int amt, shared_ptr<Creature> actor);
    virtual void onHit(DungeonMap &, const MapCoord &, shared_ptr<Creature> actor, const Originator &);
    virtual bool targettable() const;
    
protected:
    virtual shared_ptr<Tile> doClone(bool);
    virtual void openImpl(DungeonMap &, const MapCoord &, const Originator &originator);
    virtual void closeImpl(DungeonMap &, const MapCoord &, const Originator &originator);
    
private:
    const Graphic *open_graphic;
    const Graphic *closed_graphic;
    MapAccess closed_access[H_MISSILES+1];
};


class Chest : public Lockable {
public:
    Chest(lua_State *lua, KnightsConfigImpl *kc);

    virtual bool cannotActivateFrom(MapDirection &dir) const;
    
    virtual bool canPlaceItem() const;
    virtual shared_ptr<Item> getPlacedItem() const;
    virtual void placeItem(shared_ptr<Item>);
    virtual void onDestroy(DungeonMap &, const MapCoord &, shared_ptr<Creature>, const Originator &);

    virtual bool generateTrap(DungeonMap &, const MapCoord &);
    
protected:
    virtual shared_ptr<Tile> doClone(bool);
    virtual void openImpl(DungeonMap &, const MapCoord &, const Originator &);
    virtual void closeImpl(DungeonMap &, const MapCoord &, const Originator &);

private:
    struct TrapInfo {
        const ItemType *itype;
        LuaFunc action;  // action to place the trap ...
    };
    static TrapInfo popTrapInfo(lua_State *lua, KnightsConfigImpl *kc);
    
private:
    const Graphic *open_graphic;
    const Graphic *closed_graphic;
    shared_ptr<Item> stored_item;

    MapDirection facing;
    float trap_chance;
    vector<TrapInfo> traps;
};


class Barrel : public Tile {
public:
    Barrel(lua_State *lua, KnightsConfigImpl *kc)
        : Tile(lua, kc) { }
    
    virtual bool canPlaceItem() const;
    virtual shared_ptr<Item> getPlacedItem() const;
    virtual void placeItem(shared_ptr<Item>);
    virtual void onDestroy(DungeonMap &, const MapCoord &, shared_ptr<Creature>, const Originator &);

protected:
    virtual shared_ptr<Tile> doClone(bool);

private:
    shared_ptr<Item> stored_item;
};

    
class Home : public Tile {
public:
    Home(lua_State *lua, KnightsConfigImpl *kc);
    
    // onApproach/onWithdraw are overridden to handle healing and quest-checking.
    virtual void onApproach(DungeonMap &, const MapCoord &, shared_ptr<Creature>, const Originator &);
    virtual void onWithdraw(DungeonMap &, const MapCoord &, shared_ptr<Creature>, const Originator &);

    // get facing -- the facing direction points inwards, towards the home.
    MapDirection getFacing() const { return facing; }

    // "secure" (this resets the colour-change).
    void secure(DungeonMap &, const MapCoord &, shared_ptr<const ColourChange> new_cc);

    // This is used during map generation to decide which home should be assigned as the special exit point.
    // It is not used in-game at all.
    bool isSpecialExit() const { return special_exit; }
    
protected:
    virtual shared_ptr<Tile> doClone(bool);

private:
    MapDirection facing;
    bool special_exit;
};

#endif

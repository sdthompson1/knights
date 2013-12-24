/*
 * lockable.hpp
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
 * Lockable: Base class for openable/trappable/lockable tiles (ie doors and chests).
 *
 */

#ifndef LOCKABLE_HPP
#define LOCKABLE_HPP

#include "tile.hpp"

#include "boost/shared_ptr.hpp"
using namespace boost;

class Creature;
class DungeonMap;
class MapCoord;
class Trap;

class Lockable : public Tile {
    enum { PICK_ONLY_LOCK_NUM=99998, SPECIAL_LOCK_NUM = 99999 };
    
public:
    explicit Lockable(lua_State *lua);

    virtual void newIndex(lua_State *lua);

    // Override canActivateHere, onActivate to deal with opening and closing.
    virtual bool canActivateHere() const { return false; }
    virtual void onActivate(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature>, const Originator &,
                            ActivateType);

    // Methods for opening and closing Lockable tiles. Called by the Lua "open" and "close" commands.
    void close(DungeonMap &, const MapCoord &, const Originator &);
    void open(DungeonMap &, const MapCoord &, const Originator &);

    // Check whether the tile is currently closed
    bool isClosed() const { return closed; }
    
    // Locks
    bool isLocked() const { return lock > 0; }
    bool isSpecialLocked() const { return lock == SPECIAL_LOCK_NUM; }
    void generateLock(int nkeys);  // If "lock" is negative then this generates a lock.
    void setLock(int n);           // Set lock (0=unlocked, 1-3=locked with key)
    void setLockPickOnly();        // Set lock to PICK_ONLY_LOCK_NUM
    void setLockSpecial();         // Set lock to SPECIAL_LOCK_NUM

    // return lock num (1,2,3 or PICK_ONLY_LOCK_NUM), or 0 (if not
    // locked, or special-locked). used by the dungeon generator for
    // connectivity checking.
    int getLockNum() const { return isLocked() && !isSpecialLocked() ? lock : 0; }

    
    // Traps
    //
    // setTrap: Set a trap which will be triggered when the door/chest is opened. 
    //   -- If a trap is already present, then the existing trap will be set off (it will affect the given
    //      creature; if "cr" is null, then poison traps won't be set off, but I think bolt traps will).
    //   -- Note: "mc" must be the position of the door/chest, not the position of the knight setting the trap! (#167)
    //
    // generateTrap: like setTrap but called during initial "pretrapped chests" generation.
    //   (returns true if a trap was generated.)
    //
    // onHit: Override this to deal with setting off traps when a door/chest is hit.
    //
    void setTrap(DungeonMap &dmap, const MapCoord &mc, 
                 shared_ptr<Creature> cr,
                 const Originator &originator,
                 shared_ptr<Trap> newtrap);
    virtual bool generateTrap(DungeonMap &, const MapCoord &) { return false; }
    virtual void onHit(DungeonMap &, const MapCoord &, shared_ptr<Creature>, const Originator &);


    // Mini map colour of anything lockable should always be COL_FLOOR.
    // (This prevents bug where iron doors would otherwise appear as COL_WALL.)
    virtual MiniMapColour getColour() const { return COL_FLOOR; }

protected:
    // "openImpl", "closeImpl" are implemented by subclasses, and handle the actual opening
    // and closing.
    virtual void openImpl(DungeonMap &, const MapCoord &, const Originator &) = 0;
    virtual void closeImpl(DungeonMap &, const MapCoord &, const Originator &) = 0; 

private:
    bool doOpen(DungeonMap &, const MapCoord &, shared_ptr<Creature>, const Originator &, ActivateType);
    bool doClose(DungeonMap &, const MapCoord &, shared_ptr<Creature>, const Originator &, ActivateType);
    bool checkUnlock(shared_ptr<Creature>) const;
    void activateTraps(DungeonMap &, const MapCoord &, shared_ptr<Creature>);
    void disarmTraps(DungeonMap &, const MapCoord &);
    
private:
    bool closed;
    int lock; // key number, or zero for unlocked
    // NOTE: a negative value for lock means that lock has not been generated yet.
    // See lock_chance, pick_only_chance, keymax for generation parameters.
    // ALSO: if lock is set to SPECIAL_LOCK_NUM then the door cannot be unlocked by keys
    // or lock picks; only switches will work.

    float lock_chance, pick_only_chance;
    int keymax;
    shared_ptr<Trap> trap;
    Originator trap_owner;

    LuaFunc on_open_or_close;
    LuaFunc on_unlock_fail;
};

#endif

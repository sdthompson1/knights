/*
 * lockable.cpp
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

#include "action_data.hpp"
#include "dungeon_map.hpp"
#include "dungeon_view.hpp"
#include "item.hpp"
#include "item_type.hpp"
#include "knight.hpp"
#include "lockable.hpp"
#include "lua_setup.hpp"
#include "mediator.hpp"
#include "rng.hpp"
#include "trap.hpp"

//
// constructor
//

Lockable::Lockable(lua_State *lua)
    : Tile(lua),
      trap_owner(OT_None())
{
    // [t]
    closed = !LuaGetBool(lua, -1, "open");   // default closed (i.e. open=false)

    lock = -1;  // -1 ==> Lock has not been generated yet
    if (LuaGetBool(lua, -1, "special_lock")) {  // default false
        // Always generate the same lock num for this lock
        lock = SPECIAL_LOCK_NUM;
    }

    lock_chance = LuaGetProbability(lua, -1, "lock_chance");  // default 0
    pick_only_chance = LuaGetProbability(lua, -1, "lock_pick_only_chance");  // default 0
    keymax = LuaGetInt(lua, -1, "keymax", 1);   // default 1

    on_open_or_close.reset(lua, -1, "on_open_or_close");  // default null
    on_unlock_fail.reset(lua, -1, "on_unlock_fail");      // default null
}


//
// opening/closing: private functions
//

// returns true if the door was opened, or false if it was locked & we couldn't unlock
bool Lockable::doOpen(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> cr, const Originator &originator,
                      ActivateType act_type)
{
    if (isLocked() && act_type != ACT_UNLOCK_ALL) {
        bool ok = checkUnlock(cr);
        if (!ok) {
            // Failed to unlock
            if (on_unlock_fail.hasValue()) {
                ActionData ad;
                ad.setActor(cr);
                ad.setOriginator(originator);
                ad.setTile(&dmap, mc, shared_from_this());
                ad.setGenericPos(&dmap, mc);
                on_unlock_fail.execute(ad);
            }
            return false;
        }
    }
    openImpl(dmap, mc, originator);
    if (lock != SPECIAL_LOCK_NUM) lock = 0;  // special lock is always left; but other locks always unlock when opened.
    closed = false;
    if (act_type == ACT_NORMAL) activateTraps(dmap, mc, cr);
    else disarmTraps(dmap, mc);
    if (on_open_or_close.hasValue()) {
        ActionData ad;
        ad.setActor(cr);
        ad.setOriginator(originator);
        ad.setTile(&dmap, mc, shared_from_this());
        ad.setGenericPos(&dmap, mc);
        on_open_or_close.execute(ad);
    }
    return true;
}

bool Lockable::doClose(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> cr, const Originator &originator,
                       ActivateType act_type)
{
    // Doors can be closed UNLESS they are "special-locked". The latter can
    // only be closed by switches or traps (which we identify by the act_type,
    // which should be ACT_UNLOCK_ALL for switches/traps).
    
    if (lock == SPECIAL_LOCK_NUM && act_type != ACT_UNLOCK_ALL) return false;
    
    closeImpl(dmap, mc, originator);
    closed = true;

    if (on_open_or_close.hasValue()) {
        ActionData ad;
        ad.setActor(cr);
        ad.setOriginator(originator);
        ad.setTile(&dmap, mc, shared_from_this());
        ad.setGenericPos(&dmap, mc);
        on_open_or_close.execute(ad);
    }

    return true;
}

bool Lockable::checkUnlock(shared_ptr<Creature> cr) const
{
    if (!isLocked()) return true;
    
    // Only Knights have the ability to unlock doors
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(cr);
    if (kt) {
        for (int i=0; i<kt->getBackpackCount(); ++i) {
            int key = kt->getBackpackItem(i).getKey();
            if (key == this->lock) return true;
        }
    }
    return false;
}

//
// opening/closing: public functions
//

void Lockable::onActivate(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> cr,
                          const Originator &originator,
                          ActivateType act_type)
{
    // An "activate" only succeeds if the relevant doOpen/doClose routine says it does
    bool success = false, doing_open = false;
    if (closed) {
        success = doOpen(dmap, mc, cr, originator, act_type);
        doing_open = true;
    } else {
        success = doClose(dmap, mc, cr, originator, act_type);
    }
    Tile::onActivate(dmap, mc, cr, originator, act_type);
    if (doing_open) Mediator::instance().onOpenLockable(mc); // handles tutorial message (only) currently
}

void Lockable::close(DungeonMap &dmap, const MapCoord &mc, const Originator &originator)
{
    // An explicit "close" always succeeds
    if (!closed) {
        closeImpl(dmap, mc, originator);
        closed = true;
    }
}

void Lockable::open(DungeonMap &dmap, const MapCoord &mc, const Originator &originator)
{
    // An explicit "open" always succeeds
    if (closed) {
        // note: if activate_type is ACT_UNLOCK_ALL then the creature is not used, so we can pass null
        doOpen(dmap, mc, shared_ptr<Creature>(), originator, ACT_UNLOCK_ALL);
    }
}


//
// lock- and trap-related stuff
//

void Lockable::generateLock(int nkeys)
{
    if (lock < 0) {
        if (nkeys <= 0) {
            lock = 0;    // unlocked
        } else {
            if (g_rng.getBool(lock_chance)) {
                if (g_rng.getBool(pick_only_chance)) {
                    lock = PICK_ONLY_LOCK_NUM;    // locked, and can be opened by lockpicks only
                } else {
                    lock = g_rng.getInt(1, max(keymax,nkeys)+1); // normal lock
                    if (lock > nkeys) lock = 0; // unlocked (keymax has come into effect)
                }
            } else {
                lock = 0;   // unlocked
            }
        }
    }
}

void Lockable::disarmTraps(DungeonMap &dmap, const MapCoord &mc)
{
    if (trap) {
        const ItemType *itype = trap->getTrapItem();
        if (itype) {
            shared_ptr<Item> dummy;
            const bool can_drop = CheckDropSquare(dmap, mc, *itype, dummy);
            if (can_drop) {
                dummy.reset(new Item(*trap->getTrapItem()));
                dmap.addItem(mc, dummy);
            }
        }
        trap = shared_ptr<Trap>();
        trap_owner = Originator(OT_None());
    }
}

void Lockable::activateTraps(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> cr)
{
    if (trap) {
        trap->spring(dmap, mc, cr, trap_owner);
        trap = shared_ptr<Trap>();
        trap_owner = Originator(OT_None());
    }
}

void Lockable::setTrap(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> cr,
                       shared_ptr<Trap> newtrap)
{
    if (trap) {
        activateTraps(dmap, mc, cr);
    }
    trap = newtrap;
    trap_owner = cr ? cr->getOriginator() : Originator(OT_None());
}

void Lockable::onHit(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> cr, const Originator &originator)
{
    if (trap && trap->activateOnHit()) {
        activateTraps(dmap, mc, cr);
    }
    Tile::onHit(dmap, mc, cr, originator);
}

/*
 * control_actions.cpp
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


// NOTE -- When writing new actions, don't forget to set stun_time if
// required (see DoStun(creature)).


#include "misc.hpp"

#include "action_data.hpp"
#include "concrete_traps.hpp"
#include "control.hpp"
#include "control_actions.hpp"
#include "creature.hpp"
#include "dungeon_map.hpp"
#include "item.hpp"
#include "knight.hpp"
#include "knights_config_impl.hpp"
#include "lockable.hpp"
#include "lua_func_wrapper.hpp"
#include "mediator.hpp"
#include "missile.hpp"
#include "player.hpp"
#include "rng.hpp"
#include "tile.hpp"

#include "lua.hpp"

#include "boost/scoped_ptr.hpp"
#include "boost/thread/mutex.hpp"

// Undefine annoying windows macro that conflicts with my SC_MOVE constant
#ifdef SC_MOVE
#undef SC_MOVE
#endif

// Make sure the global ctors get linked (needed for MSVC)
#ifdef _MSC_VER
extern "C" void InitControls() { }
#endif

namespace {
    void DoStun(shared_ptr<Creature> actor, int gvt) 
    {
        actor->stunUntil(gvt + Mediator::instance().cfgInt("action_delay"));
    }

    shared_ptr<Knight> ExtractKnight(const ActionData &ad)
    {
        // Return actor (as knight), or NULL if no knight found or the knight is
        // not in a map.
        shared_ptr<Knight> actor = dynamic_pointer_cast<Knight>(ad.getActor());
        if (!actor || !actor->getMap()) return shared_ptr<Knight>();
        else return actor;
    }

    shared_ptr<Creature> ExtractCreature(const ActionData &ad)
    {
        // Same for Creature
        shared_ptr<Creature> actor = ad.getActor();
        if (!actor || !actor->getMap()) return shared_ptr<Creature>();
        else return actor;
    }

    ItemType * ExtractItemType(const ActionData &ad)
    {
        DungeonMap *dummy;
        MapCoord dummy2;
        ItemType *result;
        ad.getItem(dummy, dummy2, result);
        return result;
    }

    void DoSwing(shared_ptr<Creature> actor)
    {
        if (!actor) return;
        if (actor->canSwing()) {
            actor->swing(); // no need to stun
        }
    }

    void DoThrow(shared_ptr<Creature> actor)
    {
        if (!actor) return;
        if (actor->canThrowItemInHand(true)) {
            actor->throwItemInHand();
        }
    }

    void DoShoot(shared_ptr<Creature> actor, int gvt)
    {
        // Shooting of crossbows is (somewhat anomalously) handled here rather than
        // as a Knight function.
        if (!actor) return;
        if (actor->isApproaching()) return;
        const ItemType *itype = actor->getItemInHand();
        if (itype && itype->canShoot() && itype->getAmmo()) {
            // create the missile
            DungeonMap *dmap = actor->getMap();
            if (!dmap) return;

            // note: we don't care if CreateMissile was successful or not...
            CreateMissile(*dmap, actor->getPos(), actor->getFacing(),
                          *itype->getAmmo(), false, actor->hasStrength(),
                          actor->getOriginator(), false);

            // unload the crossbow, and stun
            actor->setItemInHand(itype->getUnloaded());
            actor->stunUntil(gvt + Mediator::instance().cfgInt("crossbow_delay"));
            
            // run event hook
            Mediator::instance().runHook("HOOK_SHOOT", actor);
        }
    }

    bool CanOpenTraps(shared_ptr<Creature> cr)
    {
        // This checks if the creature is holding a staff or other item that allows
        // it to open trapped doors/chests safely.
        if (!cr) return false;
        const ItemType *item = cr->getItemInHand();
        if (!item) return false;
        return item->canOpenTraps();
    }
}

//
// GetStandardControls
//

namespace {    
    A_Attack g_attack[4] = { A_Attack(D_NORTH), A_Attack(D_EAST), A_Attack(D_SOUTH), A_Attack(D_WEST) };
    A_Move g_move[4] = { A_Move(D_NORTH), A_Move(D_EAST), A_Move(D_SOUTH), A_Move(D_WEST) };
    A_Withdraw g_withdraw;
    A_AttackNoDir g_attack_no_dir;
    
    int StdActionExec(lua_State *lua)
    {
        LegacyAction * ac = static_cast<LegacyAction*>(lua_touserdata(lua, lua_upvalueindex(2)));
        ActionData ad(lua);
        bool result = ac->executeWithResult(ad);
        lua_pushboolean(lua, result);
        return 1;
    }

    int StdActionPossible(lua_State *lua)
    {
        LegacyAction * ac = static_cast<LegacyAction*>(lua_touserdata(lua, lua_upvalueindex(2)));
        ActionData ad(lua);
        bool result = ac->possible(ad);
        lua_pushboolean(lua, result);
        return 1;
    }

    void MakeControl(lua_State *lua, KnightsConfigImpl *kc,
                     bool cts, LegacyAction *ac, bool wm, bool ws)
    {
        lua_pushlightuserdata(lua, ac);
        PushCClosure(lua, &StdActionExec, 1);
        LuaFunc exec_func(lua);

        lua_pushlightuserdata(lua, ac);
        PushCClosure(lua, &StdActionPossible, 1);
        LuaFunc poss_func(lua);

        std::auto_ptr<Control> p(new Control(cts, exec_func, poss_func, wm, ws));
        kc->addLuaControl(p);
    }
}

void AddStandardControls(lua_State *lua, KnightsConfigImpl *kc)
{
    // create the standard controls
    // NOTE: These must be added in the order of the SC_ identifiers in user_control.hpp.

    for (int i = 0; i < 4; ++i) {
        MakeControl(lua, kc, true, &g_attack[i], true, false);
    }
    for (int i = 0; i < 4; ++i) {
        MakeControl(lua, kc, true, &g_move[i], true, false);
    }
        
    MakeControl(lua, kc, true, &g_withdraw, false, false);
    MakeControl(lua, kc, true, &g_attack_no_dir, true, false);
}



//
// A_Activate
//
// "Activate" is typically used as part of a tile-specific control.
// (eg opening doors or chests.) It runs the tile's "on_activate"
// function.
//
//

bool A_Activate::possible(const ActionData &ad) const
{
    // Most of the "possible" checks are taken care of by
    // Player::computeAvailableControls, which would not add the
    // control if the knight was not in a position to activate the
    // tile.
    // Here we only do one last check which is that you cannot
    // activate treasure chests from behind.

    shared_ptr<Creature> actor = ad.getActor();
    if (actor) {
        // Get the tile
        DungeonMap *dm;
        MapCoord mc;
        shared_ptr<Tile> tile;
        ad.getTile(dm, mc, tile);
        if (!tile) return false; // shouldn't happen (Activate should always be targetted at a tile)

        // See if it has a direction from which it cannot be activated
        MapDirection dir;
        const bool result = tile->cannotActivateFrom(dir);
        if (result && DisplaceCoord(mc, dir) == actor->getPos()) return false;
    }

    return true;
}

void A_Activate::execute(const ActionData &ad) const
{
    shared_ptr<Creature> actor = ad.getActor();

    // For "primary" activate commands, see what the abilities of the actor are.
    // For "secondary" activate commands (this is things like switches), assume that
    // all locks and traps can always be opened.
    const ActivateType act_type = 
        (CanOpenTraps(actor) ? ACT_DISARM_TRAPS : ACT_NORMAL);
    
    // We get mc from the Tile.
    DungeonMap *dm;
    MapCoord mc;
    shared_ptr<Tile> t;
    ad.getTile(dm, mc, t);
    if (!dm || !t) return;

    mc.setX(mc.getX());
    mc.setY(mc.getY());
    vector<shared_ptr<Tile> > tiles;
    dm->getTiles(mc, tiles);
    for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end(); ++it) {
        (*it)->onActivate(*dm, mc, actor, ad.getOriginator(), act_type);
    }

    // Check if there was a creature. If so, stun him for "action_delay".
    if (actor) {
        Mediator &mediator(Mediator::instance());
        actor->stunUntil(mediator.getGVT() + mediator.cfgInt("action_delay"));
    }
}

A_Activate::Maker A_Activate::Maker::register_me;

LegacyAction * A_Activate::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_Activate;
}


//
// A_Attack
//

void A_Attack::execute(const ActionData &ad) const
{
    shared_ptr<Creature> actor = ExtractCreature(ad);
    const int gvt = Mediator::instance().getGVT();

    if (!actor) return;
    if (actor->isApproaching()) return;

    if (actor->getFacing() != dir) {
        actor->setFacing(dir);
        DoStun(actor, gvt);
    } else {
        bool can_swing = actor->canSwing();
        bool can_throw = actor->canThrowItemInHand(true);
        bool can_shoot = actor->getItemInHand() && actor->getItemInHand()->canShoot();
        bool missile_attack = can_throw || can_shoot;

        if (can_swing && missile_attack) {
            // Both melee and missile attacks are possible.
            // If a creature is on my square or the square ahead,
            // or a "blocked" tile is on square ahead (e.g. a wall),
            // then do former, else do latter.
            // ("Open" means A_CLEAR at H_WALKING)

            // Find creature on my square or square_ahead:
            shared_ptr<Creature> target_cr = actor->getMap()->getTargetCreature(*actor, true);
            if (target_cr) {
                // Melee attack possible -- Cancel the missile option
                can_throw = can_shoot = missile_attack = false;
            }
            if (missile_attack) {
                // Now check tile-access on square ahead:
                // (use getDestinationPos because we can do this while moving & we want sq at the end of the move)
                const MapCoord sq_ahead = DisplaceCoord(actor->getDestinationPos(), actor->getFacing());
                if (actor->getMap()->getAccessTilesOnly(sq_ahead, H_WALKING) != A_CLEAR) {
                    // The square ahead is blocked (or "semi blocked", like a closed gate)
                    // Cancel the missile option
                    can_throw = can_shoot = missile_attack = false;
                }

                // If we would not be able to place a missile on the sq ahead either, then we
                // shouldn't allow the missile option. (This has the effect that if you are
                // two squares away from a wall - ie there is one open square between you and
                // the wall - then you will swing rather than throw.)
                if (!actor->getMap()->canPlaceMissile(sq_ahead, actor->getFacing())) {
                    can_throw = can_shoot = missile_attack = false;
                }

                if (missile_attack) {
                    // If we get here, then melee attack is not possible.
                    // Cancel the melee option.
                    can_swing = false;
                }
            }
        }

        if (can_swing) {
            DoSwing(actor); // no need to stun
        } else if (can_throw) {
            DoThrow(actor); // no need to stun
        } else if (can_shoot) {
            DoShoot(actor, gvt); // no need to stun
        } else if (actor->getMap() && actor->getItemInHand()
                   && !actor->getItemInHand()->canSwing()      // .. don't drop melee weapons
                   && !actor->getItemInHand()->canShoot()) {   // .. don't drop loaded xbows (but drop unloaded ones)
            // The reason we can't swing is because we are holding a non-weapon item.
            // Try dropping it. (E.g. if a player holding a book tries to attack, then we 
            // want to automatically drop the book.)
            ActionData data;
            data.setActor(actor);
            data.setGenericPos(actor->getMap(), actor->getPos());
            data.setOriginator(actor->getOriginator());
            A_DropHeld action;
            action.execute(data);
        }
    }
}

//
// A_AttackNoDir
//

void A_AttackNoDir::execute(const ActionData &ad) const
{
    shared_ptr<Creature> actor = ExtractCreature(ad);
    A_Attack att(actor->getFacing());
    att.execute(ad);
}


//
// A_Move
//

void A_Move::execute(const ActionData &ad) const
{
    shared_ptr<Creature> actor = ExtractCreature(ad);
    Mediator &mediator = Mediator::instance();
    const int gvt = mediator.getGVT();
    
    if (!actor) return;
    if (actor->isApproaching()) return;  // can't move if already approached (or approaching)

    if (!actor->isMoving()) {
        // Normal Motion Cmd
        if (actor->getFacing() != dir) {
            actor->setFacing(dir);
            int turn_delay = mediator.cfgInt("turn_delay");
            if (actor->hasQuickness()) turn_delay = (turn_delay * 2) / 3;
            actor->stunUntil(gvt + turn_delay);
        } else {
            MapCoord dest = DisplaceCoord(actor->getPos(), dir, 1);
            MapAccess dest_access = actor->getMap()->getAccess(dest, actor->getHeight());
            if (dest_access == A_CLEAR) {
                actor->move(MT_MOVE);
            } else if (dest_access == A_APPROACH && actor->getPlayer()->getApproachBasedControls()) {
                actor->move(MT_APPROACH);
            }
        }
    } else if (actor->getOffset() <= mediator.cfgInt("walk_limit")) {
        // In this case we are in the middle of moving, and we are allowed to turn back,
        // but no other motion actions are allowed.
        if (dir == Opposite(actor->getFacing())) {
            actor->flipMotion();
        }
    }
}



//
// A_Withdraw
//

void A_Withdraw::execute(const ActionData &ad) const
{
    shared_ptr<Creature> actor = ExtractCreature(ad);
    if (!actor) return;
    if (!actor->isApproaching()) return;  // can only withdraw if approached!
    actor->move(MT_WITHDRAW);
    DoStun(actor, Mediator::instance().getGVT());
}



//
// A_Suicide
//

void A_Suicide::execute(const ActionData &ad) const
{
    shared_ptr<Creature> actor = ExtractCreature(ad);
    if (!actor) return;
    Mediator::instance().runHook("HOOK_KNIGHT_DAMAGE", actor);
    actor->onDeath(Creature::NORMAL_MODE, ad.getOriginator());
    actor->rmFromMap();
}

A_Suicide::Maker A_Suicide::Maker::register_me;

LegacyAction * A_Suicide::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_Suicide;
}


//
// Drop
//

bool A_Drop::possible(const ActionData &ad) const
{
    shared_ptr<Knight> actor = ExtractKnight(ad);
    if (!actor) return false;

    if (actor->getNumCarried(it) == 0) return false;

    // If we are approaching
    // (or are using approach-less controls)
    // then try to drop onto the square ahead (assuming it is approachable).
    // Otherwise drop onto the current square.
    const bool drop_ahead =
        !actor->getPlayer()->getApproachBasedControls()
        || actor->isApproaching();
    return CanDropItem(it, *actor->getMap(), actor->getPos(),
                       drop_ahead, actor->getFacing());
}

void A_Drop::execute(const ActionData &ad) const
{
    const int gvt = Mediator::instance().getGVT();
    shared_ptr<Knight> actor = ExtractKnight(ad);
    if (!actor) return;
    
    int no_to_drop = actor->getNumCarried(this->it);
    if (no_to_drop == 0) return;
    if (no_to_drop > this->it.getMaxStack()) no_to_drop = this->it.getMaxStack();

    const bool drop_ahead =
        !actor->getPlayer()->getApproachBasedControls()
        || actor->isApproaching();

    shared_ptr<Item> item(new Item(this->it, no_to_drop));
    bool added_to_map = DropItem(item, *actor->getMap(), actor->getPos(), false,
                                 drop_ahead, actor->getFacing(),
                                 actor);

    if (item->getNumber() < no_to_drop) added_to_map = true;

    if (added_to_map) {
        // drop was successful
        DoStun(actor, gvt);
    }
}

A_Drop::Maker A_Drop::Maker::register_me;

LegacyAction * A_Drop::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    ItemType *it = pars.getItemType(0);
    if (it) return new A_Drop(*it);
    else return 0;
}


//
// DropHeld
//

bool A_DropHeld::possible(const ActionData &ad) const
{
    shared_ptr<Knight> actor = ExtractKnight(ad);
    if (!actor) return false;
    if (!actor->canDropHeld()) return false;

    // Rule: If actor is approaching then he can only drop the item into the
    // approached square. Otherwise, he can drop it onto any adjacent square.
    // (This is to prevent "accidental" drops when when e.g. approaching a
    // closed chest from behind.)
    if (actor->isApproaching()) {
        shared_ptr<Item> dummy;
        return CheckDropSquare(*actor->getMap(), actor->getTargettedPos(), *actor->getItemInHand(), dummy);
    } else {

        const bool drop_ahead =
            !actor->getPlayer()->getApproachBasedControls()
            || actor->isApproaching();

        return CanDropItem(*actor->getItemInHand(), *actor->getMap(), actor->getPos(),
                           drop_ahead, actor->getFacing());
    }
}

void A_DropHeld::execute(const ActionData &ad) const
{
    const int gvt = Mediator::instance().getGVT();
    shared_ptr<Knight> actor = ExtractKnight(ad);
    if (!actor) return;
    if (!actor->canDropHeld()) return;

    const bool drop_ahead =
        !actor->getPlayer()->getApproachBasedControls()
        || actor->isApproaching();

    shared_ptr<Item> item(new Item(*actor->getItemInHand()));
    bool added_to_map = DropItem(item, *actor->getMap(), actor->getPos(), false,
                                 drop_ahead, actor->getFacing(),
                                 actor);
    if (added_to_map) {
        DoStun(actor, gvt);
    }
}

A_DropHeld::Maker A_DropHeld::Maker::register_me;

LegacyAction * A_DropHeld::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_DropHeld;
}


//
// PickLock
//

namespace {
    shared_ptr<Lockable> GetLockPickTarget(const ActionData &ad)
    {
        shared_ptr<Creature> actor = ExtractCreature(ad);
        if (!actor || !actor->getMap()) return shared_ptr<Lockable>();
        MapCoord mc = DisplaceCoord(actor->getPos(), actor->getFacing());
        vector<shared_ptr<Tile> > tiles;
        actor->getMap()->getTiles(mc, tiles);
        for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end();
        ++it) {
            shared_ptr<Lockable> t = dynamic_pointer_cast<Lockable>(*it);
            if (t && t->isClosed() && t->isLocked() && !t->isSpecialLocked()) {
                return t;
            }
        }
        return shared_ptr<Lockable>();
    }
}

bool A_PickLock::possible(const ActionData &ad) const
{
    return GetLockPickTarget(ad);
}

void A_PickLock::execute(const ActionData &ad) const
{
    Mediator &mediator = Mediator::instance();
    const int gvt = mediator.getGVT();
    shared_ptr<Creature> cr = ExtractCreature(ad);
    if (!cr) return;
    shared_ptr<Lockable> t = GetLockPickTarget(ad);
    if (!t) return;
    if (g_rng.getBool(prob)) {
        // lock pick was successful
        // We can call "open" directly to make the tile open (even though it is locked)
        t->open(*cr->getMap(), DisplaceCoord(cr->getPos(), cr->getFacing()), ad.getOriginator());
    }
    
    // compute stun time (lock picking is faster with quickness)
    int time = waiting_time;
    if (cr->hasQuickness()) time = time * 100 / mediator.cfgInt("quickness_factor"); 
    cr->stunUntil(gvt + time);
}

A_PickLock::Maker A_PickLock::Maker::register_me;

LegacyAction * A_PickLock::Maker::make(ActionPars &pars) const
{
    pars.require(2);
    float p = pars.getProbability(0);
    int wt = pars.getInt(1);
    return new A_PickLock(p, wt);
}

//
// PickUp
//

namespace {
    bool CanPickupFrom(const Knight &kt, const MapCoord &mc)
    {
        shared_ptr<Item> item = kt.getMap()->getItem(mc);
        if (!item) return false;
        if (!item->getType().canPickUp()) return false;
        if (item->getType().isBig()) return true;  // can always pick up a "big" item (by
                                                   // dropping existing one if necessary)
        return kt.canAddToBackpack(item->getType());
    }
}

bool A_PickUp::possible(const ActionData &ad) const
{
    shared_ptr<Knight> actor = ExtractKnight(ad);
    if (!actor || !actor->getMap()) return false;

    // find mc.
    MapCoord mc = actor->getTargettedPos();

    if (CanPickupFrom(*actor, mc)) {
        return true;
    }

    if (actor->getPlayer()->getApproachBasedControls()) {
        return false;  // they have to approach if they want to pick up from the square ahead
    }

    // non approach based controls; can pick up from the square ahead as well (if it is approachable)
    const MapCoord mc_ahead = DisplaceCoord(mc, actor->getFacing());
    return actor->getMap()->getAccessTilesOnly(mc_ahead, H_WALKING) == A_APPROACH
        && CanPickupFrom(*actor, mc_ahead);
}

namespace {
    // returns true if pickup was successful
    bool DoPickup(shared_ptr<Knight> actor, const MapCoord &mc,
                  Mediator &mediator, int gvt)
    {
        if (!actor->getMap()) return false;

        shared_ptr<Item> item = actor->getMap()->getItem(mc);

        if (!item) return false;
        if (!item->getType().canPickUp()) return false;

        bool picked_up = false;
        
        if (item->getType().isBig()) {

            // Big item -- may need to drop held item first
            shared_ptr<Item> drop;
            if (actor->canDropHeld()) {
                drop.reset(new Item( *actor->getItemInHand(), 1 ));
            }

            // Pick up item from the map
            actor->getMap()->rmItem(mc);
            actor->setItemInHand(&item->getType());

            // Drop replacement item into the map
            if (drop) {
                actor->getMap()->addItem(mc, drop);
                // Run the "onDrop" event manually (since we're not using DropItem):
                drop->getType().onDrop(*actor->getMap(), mc, actor);
            }

            picked_up = true;

        } else {
            // Small item -- add to backpack
            int amt;
            if (item->getType().isMagic()) {
                // Magic items should be "fully picked up" and not added to backpack
                amt = item->getNumber();
            } else {
                // All other items should be added to the backpack.
                amt = actor->addToBackpack(item->getType(), item->getNumber());
            }
            if (amt > 0) {
                // Remove item from the ground
                if (amt < item->getNumber()) {
                    item->setNumber(item->getNumber() - amt);
                } else {
                    actor->getMap()->rmItem(mc);
                }
                picked_up = true;
            }
        }

        if (picked_up) {
            // Pickup event and Stun
            item->getType().onPickUp(*actor->getMap(), mc, actor);
            DoStun(actor, gvt);

            // Tell mediator (needed for the tutorial hints)
            if (actor->getPlayer()) {
                mediator.onPickup(*actor->getPlayer(), item->getType());
            }
        }

        return picked_up;
    }
}

void A_PickUp::execute(const ActionData &ad) const
{
    Mediator & mediator = Mediator::instance();
    const int gvt = mediator.getGVT();
    
    // find knight
    shared_ptr<Knight> actor = ExtractKnight(ad);
    if (!actor) return;

    // try to pick up
    const MapCoord mc_here = actor->getTargettedPos();
    const MapCoord mc_ahead = DisplaceCoord(mc_here, actor->getFacing());
    
    if (actor->getPlayer()->getApproachBasedControls()) {
        // can only do the selected tile
        DoPickup(actor, mc_here, mediator, gvt);
    } else {
        // try the tile ahead
        bool picked_up = false;
        if (actor->getMap()->getAccessTilesOnly(mc_ahead, H_WALKING) == A_APPROACH) {
            picked_up = DoPickup(actor, mc_ahead, mediator, gvt);
        }
        // if that failed, try the tile here
        if (!picked_up) {
            DoPickup(actor, mc_here, mediator, gvt);
        }
    }
}

A_PickUp::Maker A_PickUp::Maker::register_me;

LegacyAction * A_PickUp::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_PickUp;
}

//
// SetBearTrap
//

bool A_SetBearTrap::possible(const ActionData &ad) const
{
    shared_ptr<Knight> actor = ExtractKnight(ad);
    if (!actor || !actor->getMap()) return false;

    // Knight must not be approaching.
    if (actor->isApproaching()) return false;

    // We must be able to drop the trap-item in the square ahead.
    MapCoord mc = DisplaceCoord(actor->getPos(), actor->getFacing(), 1);    
    shared_ptr<Item> dummy;
    if (!CheckDropSquare(*actor->getMap(), mc, trap_itype, dummy)) return false;

    // There must be no (walking) creature in the square ahead. (Trac #8)
    std::vector<boost::shared_ptr<Entity> > entities;
    actor->getMap()->getEntities(mc, entities);
    for (std::vector<boost::shared_ptr<Entity> >::const_iterator it = entities.begin(); it != entities.end(); ++it) {
        Creature *cr = dynamic_cast<Creature*>(it->get());
        if (cr && cr->getHeight() == H_WALKING) {
            return false;
        }
    }
    
    return true;
}

void A_SetBearTrap::execute(const ActionData &ad) const
{
    const int gvt = Mediator::instance().getGVT();
    
    shared_ptr<Knight> actor = ExtractKnight(ad);
    if (!actor) return;
    if (actor->isApproaching()) return;
    MapCoord mc = DisplaceCoord(actor->getPos(), actor->getFacing(), 1);

    shared_ptr<Item> curr_item;
    if (!CheckDropSquare(*actor->getMap(), mc, trap_itype, curr_item)) return;
    if (curr_item) return; // bear traps shouldn't be able to stack.

    shared_ptr<Item> trap_item(new Item(trap_itype, 1));
    trap_item->setOwner(ad.getOriginator());
    actor->getMap()->addItem(mc, trap_item);
    actor->setItemInHand(0);
    DoStun(actor, gvt);
}

A_SetBearTrap::Maker A_SetBearTrap::Maker::register_me;

LegacyAction * A_SetBearTrap::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    ItemType *it = pars.getItemType(0);
    if (it) return new A_SetBearTrap(*it);
    else return 0;
}


// NB there is a little bit of shared code between SetPoisonTrapOld and SetBladeTrapOld.
// If more trap types are added it may be worth factoring this out.

namespace {
    shared_ptr<Lockable> CheckSetTrap(const ActionData &ad, MapCoord &mc)
    {
        shared_ptr<Creature> actor = ExtractCreature(ad);

        // To set a trap the actor must be approaching the chest
        bool approach_check = actor->isApproaching();
        
        // Also: if the player has non-approach based controls then we don't actually need
        // to approach to set a trap
        if (!approach_check) {
            Knight * kt = dynamic_cast<Knight*>(actor.get());
            if (kt && ! kt->getPlayer()->getApproachBasedControls()) approach_check = true;
        }

        if (!actor || !approach_check) return shared_ptr<Lockable>();
        mc = DisplaceCoord(actor->getPos(), actor->getFacing());
        vector<shared_ptr<Tile> > tiles;
        actor->getMap()->getTiles(mc, tiles);
        for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end();
        ++it) {
            shared_ptr<Lockable> lck = dynamic_pointer_cast<Lockable>(*it);
            if (lck && lck->isClosed() && !lck->isLocked()) {
                return lck;
            }
        }
        return shared_ptr<Lockable>();
    }

    void RemoveItem(shared_ptr<Creature> cr, const ItemType *it)
    {
        shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(cr);
        if (kt && it) kt->rmFromBackpack(*it, 1);
    }
}

bool A_SetBladeTrapOld::possible(const ActionData &ad) const
{
    MapCoord dummy;
    return bool(CheckSetTrap(ad, dummy));
}

void A_SetBladeTrapOld::execute(const ActionData &ad) const
{
    MapCoord target_pos;
    shared_ptr<Lockable> target = CheckSetTrap(ad, target_pos);
    shared_ptr<Creature> actor = ExtractCreature(ad);
    if (target && actor) {
        ItemType *it = ExtractItemType(ad);
        shared_ptr<Trap> trap(new BladeTrap(it, missile_type, Opposite(actor->getFacing())));
        target->setTrap(*actor->getMap(), target_pos, actor, actor->getOriginator(), trap);
        RemoveItem(actor, it);
    }
}

A_SetBladeTrapOld::Maker A_SetBladeTrapOld::Maker::register_me;

LegacyAction * A_SetBladeTrapOld::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    ItemType *it = pars.getItemType(0);
    if (it) return new A_SetBladeTrapOld(*it);
    else return 0;
}


bool A_SetPoisonTrapOld::possible(const ActionData &ad) const
{
    MapCoord dummy;
    return bool(CheckSetTrap(ad, dummy));
}

void A_SetPoisonTrapOld::execute(const ActionData &ad) const
{
    MapCoord target_pos;
    shared_ptr<Lockable> target = CheckSetTrap(ad, target_pos);
    shared_ptr<Creature> actor = ExtractCreature(ad);
    if (target && actor) {
        ItemType *it = ExtractItemType(ad);
        shared_ptr<Trap> trap(new PoisonTrap(it));
        target->setTrap(*actor->getMap(), target_pos, actor, actor->getOriginator(), trap);
        RemoveItem(actor, it);
    }
}

A_SetPoisonTrapOld::Maker A_SetPoisonTrapOld::Maker::register_me;

LegacyAction * A_SetPoisonTrapOld::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_SetPoisonTrapOld;
}


//
// Swing
//

bool A_Swing::possible(const ActionData &ad) const
{
    // This action is only possible while not approaching
    shared_ptr<Creature> actor = ExtractCreature(ad);
    if (!actor || !actor->getMap()) return false;
    if (actor->isApproaching()) return false;
    if (!actor->canSwing()) return false;
    return true;
}

void A_Swing::execute(const ActionData &ad) const
{
    shared_ptr<Creature> actor = ExtractCreature(ad);
    if (!actor || !actor->canSwing()) return;
    DoSwing(actor);  // no need to stun
}

A_Swing::Maker A_Swing::Maker::register_me;

LegacyAction * A_Swing::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_Swing;
}

//
// SwingOrDrop
//

bool A_SwingOrDrop::possible(const ActionData &ad) const
{
    shared_ptr<Creature> actor = ExtractCreature(ad);
    if (!actor || !actor->getMap()) return false;
    if (actor->isApproaching()) return false;
    return true;
}

void A_SwingOrDrop::execute(const ActionData &ad) const
{
    shared_ptr<Creature> actor = ExtractCreature(ad);
    if (!actor || !actor->getMap()) return;
    if (actor->canSwing()) {
        DoSwing(actor);  // no need to stun
    } else {
        A_DropHeld action;
        action.execute(ad);
    }
}

A_SwingOrDrop::Maker A_SwingOrDrop::Maker::register_me;

LegacyAction * A_SwingOrDrop::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_SwingOrDrop;
}


//
// Throw
//

bool A_Throw::possible(const ActionData &ad) const
{
    // This can only be used on Knights because it requires throwItem()
    // instead of just throww().
    shared_ptr<Knight> actor = ExtractKnight(ad);
    if (!actor || !actor->getMap()) return false;

    DungeonMap *dum;
    MapCoord dum2;
    ItemType *itype;
    ad.getItem(dum, dum2, itype);
    if (!itype) return false;

    // Strict set to false - see comments in creature.hpp
    return actor->canThrowItem(*itype, false);
}

void A_Throw::execute(const ActionData &ad) const
{
    shared_ptr<Knight> actor = ExtractKnight(ad);
    if (!actor) return;

    // cannot throw if you are carrying something (Trac #59)
    // In this case we automatically drop the held item first (this makes the new control
    // system, with a separate "throw" key, a bit smoother).
    if (actor->canDropHeld()) {
        A_DropHeld action;
        action.execute(ad);
    } else {

        // do the throw.
        DungeonMap *dum;
        MapCoord dum2;
        ItemType *itype;
        ad.getItem(dum, dum2, itype);
        if (!itype) return;
        
        actor->throwItem(*itype);
    }
}

A_Throw::Maker A_Throw::Maker::register_me;

LegacyAction * A_Throw::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_Throw;
}


//
// A_ThrowOrShoot
//

bool A_ThrowOrShoot::possible(const ActionData &ad) const
{
    shared_ptr<Creature> actor = ExtractCreature(ad);

    if (!actor || !actor->getMap()) return false;
    if (actor->isApproaching()) return false;
    if (!actor->canThrow(false)) return false;
    
    return actor->canThrowItemInHand(true)
           || (actor->getItemInHand() && actor->getItemInHand()->canShoot());
}

void A_ThrowOrShoot::execute(const ActionData &ad) const
{
    shared_ptr<Creature> actor = ExtractCreature(ad);

    if (!actor) return;
    if (actor->isApproaching()) return;

    if (actor->canThrowItemInHand(true)) {
        DoThrow(actor);
    } else {
        DoShoot(actor, Mediator::instance().getGVT());
    }
}

A_ThrowOrShoot::Maker A_ThrowOrShoot::Maker::register_me;

LegacyAction * A_ThrowOrShoot::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_ThrowOrShoot;
}

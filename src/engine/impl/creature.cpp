/*
 * creature.cpp
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

#include "anim.hpp"
#include "creature.hpp"
#include "dungeon_map.hpp"
#include "item_type.hpp"
#include "item.hpp"
#include "knight.hpp"
#include "mediator.hpp"
#include "missile.hpp"
#include "player.hpp"
#include "rng.hpp"
#include "task.hpp"
#include "task_manager.hpp"

//
// SetAnimTask -- set the anim frame of a creature at a certain time
//

class SetAnimTask : public Task {
public:
    SetAnimTask(weak_ptr<Creature> c_, int fr_, int st) : c(c_), fr(fr_), stop_time(st) { }
    virtual void execute(TaskManager &) {
        shared_ptr<Creature> cr(c.lock());
        if (cr) cr->setAnimFrame(fr, stop_time);
    }
private:
    weak_ptr<Creature> c;
    int fr;
    int stop_time;
};


//
// ImpactTask -- this processes melee weapon impacts.
//

class ImpactTask : public Task {
public:
    ImpactTask(weak_ptr<Creature> c_) : c(c_) { }
    virtual void execute(TaskManager &);
private:
    enum ImpactResult { NO_TARGET, HIT, VETO };
    ImpactResult tryCreatureImpact(Creature &me);
private:
    weak_ptr<Creature> c;
};

ImpactTask::ImpactResult ImpactTask::tryCreatureImpact(Creature &me)
{
    // Find target
    if (!me.getMap()) return NO_TARGET;
    shared_ptr<Creature> target_creature = me.getMap()->getTargetCreature(me, true);
    if (!target_creature) return NO_TARGET;
    
    // If the target creature is on the same team as me, then disallow the impact (Trac #128)
    if (target_creature->getPlayer() && me.getPlayer()
        && me.getPlayer()->getTeamNum() >= 0
        && target_creature->getPlayer()->getTeamNum() == me.getPlayer()->getTeamNum()) {
            return NO_TARGET;
    }

    // The "impactVeto" process is designed to ensure that
    // there is a fair result when two creatures strike each
    // other simultaneously (or near simultaneously).
    Mediator &mediator(Mediator::instance());
    const int gvt = mediator.getGVT();
    if (target_creature->getPos() != me.getPos() && target_creature->getFacing()
    == Opposite(me.getFacing()) && target_creature->impactVeto(gvt, me)) {
        // enemy is going to hit me first -- abort my attack and wait.
        me.setAnimFrame(0, 0);
        me.stunUntil(gvt + mediator.cfgInt("attack_threshold") + 1);
        return VETO;
    }

    // Parry Check:
    if (target_creature->getFacing() == Opposite(me.getFacing())
    && !target_creature->isApproaching()
    && !target_creature->isMoving()
    && !target_creature->isStunned()) {

        // if the target is currently "doing something" (they have a control set)
        // then we parrying is not allowed
        const Player * target_player = target_creature->getPlayer();
        const bool doing_something = target_player && target_player->peekControl();
        if (!doing_something) {

            // Now check that the target is carrying a weapon that allows parrying
            const ItemType *parry_weapon = target_creature->getItemInHand();
            if (parry_weapon) {
                const float parry_chance = parry_weapon->getParryChance() / 100.0f;
                if (g_rng.getBool(parry_chance)) {
                    // Successful parry
                    target_creature->parry();
                    return HIT;
                }
            }
        }
    }
                
    // If we get to here then we may proceed to hit the target.
    if (me.getItemInHand())
        me.getItemInHand()->doCreatureImpact(gvt,
                static_pointer_cast<Creature>(me.shared_from_this()),
                target_creature, me.hasStrength());
    return HIT;
}   

void ImpactTask::execute(TaskManager &tm)
{
    shared_ptr<Creature> me(c.lock());
    if (!me) return;
    if (!me->item_in_hand) return;  // attacker doesn't seem to have a weapon
    if (me->isMoving()) return;     // we should not be able to swing weapon while moving...

    const int gvt = tm.getGVT();
    if (me->impact_time == 0) {   // impact was cancelled.
        me->setFrameToZeroImmediately();  // Make sure we cancel the backswing animation.
        return;
    }
    if (me->impact_time != gvt) return;  // this is the wrong impact task.
    me->impact_time = 0;

    DungeonMap * dmap = me->getMap();
    if (!dmap) return;

    // Run the downswing hook.
    Mediator &mediator = Mediator::instance();
    const char *hook = me->getWeaponDownswingHook();
    if (hook) {
        mediator.runHook(hook, me);
    }
    
    // Check impacts against creatures -- first in the tile ahead, then in my current tile
    // (the latter makes it possible to hit vampire bats while they are flying over you,
    // which makes them a little easier to kill).
    ImpactResult result = tryCreatureImpact(*me);

    if (result == NO_TARGET) {
        // Couldn't find any creatures to hit; hit the tile itself.
        me->item_in_hand->doTileImpact(me, *dmap, DisplaceCoord(me->getPos(),me->getFacing()),
                                       me->hasStrength());
    }

    // Clean up attacker's anim state and stun time (assuming impact was not vetoed).
    if (result != VETO) {
        const int qf = me->hasQuickness() ? mediator.cfgInt("quickness_factor") : 100;
        const int ds_time = me->item_in_hand->getMeleeDownswingTime();
        const int wt_time = ds_time + mediator.cfgInt("melee_delay_time");
        me->setAnimFrame(AF_IMPACT, gvt + ds_time * 100 / qf);
        me->stunUntil(gvt + wt_time * 100 / qf);
    }
}

//
// Creature::impactVeto
//

bool Creature::impactVeto(int gvt, const Creature &attacker)
{
    Mediator &mediator(Mediator::instance());
    if (impact_time >= gvt && impact_time <= gvt + mediator.cfgInt("attack_threshold")) {
        // I am about to hit the attacker, just as he hits me, ie the
        // impact times are simultaneous (to within attack_threshold).
        // In this case we decide 50/50 who wins.

        // SPECIAL RULE: if the attacker is a monster (non-knight creature) and the defender
        // is a knight, then the impact is always vetoed. (This makes the zombies slightly
        // easier to kill, and it matches the original Knights behaviour better.)
        // Conversely, if the attacker is a knight and the defender is a monster, then the
        // impact can never be vetoed.

        const bool attacker_is_knight = dynamic_cast<const Knight *>(&attacker) != 0;
        const bool defender_is_knight = dynamic_cast<const Knight *>(this) != 0;
        const bool must_veto = !attacker_is_knight && defender_is_knight;
        const bool must_not_veto = attacker_is_knight && !defender_is_knight;
        if (must_veto || (!must_not_veto && g_rng.getBool(0.5f))) {
            // attacker loses (veto his impact)
            return true;
        } else {
            // I lose -- cancel my own impact.
            setAnimFrame(0, 0);
            stunUntil(gvt + mediator.cfgInt("attack_threshold"));
            return false;
        }
    } else {
        // No simultaneous impact ("normal" case)
        return false; // no veto
    }
}


//
// ThrowingTask -- throws a missile weapon (after backswing has completed).
//

class ThrowingTask : public Task {
public:
    ThrowingTask(weak_ptr<Creature> c_, const ItemType &i) : c(c_), itype(i) { }
    virtual void execute(TaskManager &);
private:
    weak_ptr<Creature> c;
    const ItemType &itype;
};

void ThrowingTask::execute(TaskManager &tm)
{
    shared_ptr<Creature> me(c.lock());
    if (!me) return;
    if (me->isMoving()) return;

    const int gvt = tm.getGVT();
    if (me->impact_time != gvt) return;
    me->impact_time = 0;

    DungeonMap *dmap = me->getMap();
    if (!dmap) return;

    // run the throwing hook (actually, same as downswing hook for now :)
    Mediator &mediator = Mediator::instance();
    const char *hook = me->getWeaponDownswingHook();
    if (hook) {
        Mediator::instance().runHook(hook, me);
    }
    
    // create the missile
    bool success = CreateMissile(*dmap, me->getPos(), me->getFacing(),
                                 itype, true, me->hasStrength(), me->getOriginator(), false);

    // reset anim / stun time
    const int qf = me->hasQuickness() ? mediator.cfgInt("quickness_factor") : 100;
    const int ds_time = itype.getMissileDownswingTime();
    const int wt_time = ds_time + mediator.cfgInt("melee_delay_time");
    me->setAnimFrame(AF_THROW_DOWN, gvt + ds_time * 100 / qf);
    me->stunUntil(gvt + wt_time * 100 / qf);

    // get rid of the item - if throw was unsuccessful then drop at feet
    // (Note: we don't check if the drop succeeded or not. If there is nowhere
    // to drop the item then that's just tough - the item is lost.)
    me->throwAwayItem(&itype);
    if (!success) {
        shared_ptr<Item> item(new Item(itype));
        DropItem(item, *dmap, me->getPos(), true, false, me->getFacing(), me);
    }
}


//
// Creature
//

Creature::Creature(int h, MapHeight ht, const ItemType * i, const Anim * a, int s)
    : height(ht), health(h), max_health(h), speed(s), stun_until(0), item_in_hand(i),
      impact_time(0)
{
    // Hmm, perhaps "speed" should be in Knight rather than Creature.
    setAnim(a);
    setSpeed(speed);
    if (i) setOverlay(i->getOverlay());
}


bool Creature::isStunned() const
{
    if (!getMap()) return false;
    return stun_until > Mediator::instance().getGVT() || isParalyzed();
}

void Creature::stunUntil(int new_stun_time)
{
    if (new_stun_time > stun_until) stun_until = new_stun_time;
    if (impact_time && isStunned()) impact_time = 0; // cancel impact-in-progress if stunned.
}

void Creature::stunnedUntil(bool &known, int &s_until) const
{
    if (!getMap()) {
        // Creature isn't in map -- we cannot access GVT.
        known = false;
    } else if (isParalyzed()) {
        // If we are paralyzed, then no lower bound can be given, since there could
        // be a DISPEL MAGIC at any time.
        known = false;
    } else if (stun_until > Mediator::instance().getGVT()) {
        // We know that definitely we will be stunned until stun_until
        known = true;
        s_until = stun_until;
    } else {
        // Hmm, it doesn't look like we are stunned after all.
        // Set known to false in this case
        known = false;
    }
}


//
// sets the item in hand, & updates overlay graphic
//

void Creature::doSetItemInHand(const ItemType * i)
{
    item_in_hand = i;
    if (i) setOverlay(i->getOverlay());
    else setOverlay(0);
}

//
// melee
//

bool Creature::canSwing() const
{
    if (!item_in_hand) return false;
    if (!item_in_hand->canSwing()) return false;
    if (isApproaching()) return false;  // can only swing while standing in centre of a sq. 
                                        // or moving a full sq. - not while approaching.

    return true;
}

void Creature::swing()
{
    if (!getMap()) return;
    if (!canSwing()) return;
    TaskManager &tm(Mediator::instance().getTaskManager());
    
    const int gvt = tm.getGVT();
    const int qf = hasQuickness() ? Mediator::instance().cfgInt("quickness_factor") : 100;
    const int time_to_impact = item_in_hand->getMeleeBackswingTime() * 100 / qf;
    this->impact_time = gvt + time_to_impact;

    adjustImpactTimeIfMoving();

    // set anim and stun
    if (impact_time > stun_until) stun_until = this->impact_time;
    delayedSetAnimFrame(AF_BACKSWING);

    // set the impact task.
    shared_ptr<ImpactTask> imp_tsk(new ImpactTask(static_pointer_cast<Creature>(
        shared_from_this())));
    tm.addTask(imp_tsk, TP_NORMAL, impact_time);
}

void Creature::adjustImpactTimeIfMoving()
{
    if (isMoving()) {
        // If we are moving then adjust the impact time
        // -- you can't gain more than "att_mov_time" off the standard attack time 
        // (compared to waiting until arrival before swinging).
        const int gvt = Mediator::instance().getTaskManager().getGVT();
        const int base_time_to_impact = this->impact_time - gvt;
        const int earliest_impact = getArrivalTime() + 
            max(0, base_time_to_impact - Mediator::instance().cfgInt("att_mov_delay_time"));
        if (this->impact_time < earliest_impact) this->impact_time = earliest_impact;
    }
}

void Creature::delayedSetAnimFrame(int fr)
{
    Mediator &mediator = Mediator::instance();
    TaskManager &tm(mediator.getTaskManager());
    const int gvt = tm.getGVT();
        
    if (isMoving()) {
        // If we are moving, then we might delay the start of the
        // backswing animation, for aesthetic reasons.

        const int earliest_anim_time = getArrivalTime() - mediator.cfgInt("att_mov_anim_time");

        if (earliest_anim_time > gvt) {
            // We require a delay, add a task to set the anim frame
            // when ready.
            shared_ptr<SetAnimTask> tsk(new SetAnimTask(
                static_pointer_cast<Creature>(shared_from_this()),
                fr,
                gvt + 5000));
            tm.addTask(tsk, TP_NORMAL, earliest_anim_time);
            return;
        }
    }

    // No delay, just set the anim frame right now.
    // NB the 5-second timeout is a safety feature, this puts the anim frame back to AF_NORMAL
    // if it somehow never gets reset by anything else (but normally it will be explicitly
    // reset by the ImpactTask or ThrowingTask long before the 5 seconds).
    setAnimFrame(fr, gvt + 5000);
}


//
// throwing
//

bool Creature::canThrow(bool strict) const
{
    // Check if creature is in a map
    if (!getMap()) return false;

    // Can never throw while approaching
    if (isApproaching()) return false;
    
    // Check if the square ahead allows missiles
    // Note: If non-strict then we check tiles only - not actual entities.
    const MapDirection facing = getFacing();
    const MapCoord pos = DisplaceCoord(getDestinationPos(), facing);
    const MapHeight height = MapHeight(H_MISSILES + facing);
    MapAccess access;
    if (strict) {
        access = getMap()->getAccess(pos, height);
    } else {
        access = getMap()->getAccessTilesOnly(pos, height);
    }

    return (access != A_BLOCKED);
}

bool Creature::canThrowItemInHand(bool strict) const
{
    return canThrow(strict) && getItemInHand() && getItemInHand()->canThrow();
}

void Creature::throwItemInHand()
{
    if (!canThrowItemInHand(true)) return;
    doThrow(*getItemInHand());
}

void Creature::doThrow(const ItemType &itype)
{
    // Check if can throw 
    if (!canThrow(true)) return;

    Mediator &mediator(Mediator::instance());
    TaskManager &tm(mediator.getTaskManager());
    
    // Work out timings.
    const int gvt = tm.getGVT();
    const int qf = hasQuickness() ? mediator.cfgInt("quickness_factor") : 100;
    this->impact_time = gvt + itype.getMissileBackswingTime() * 100 / qf;
    adjustImpactTimeIfMoving();

    // Set overlay
    setOverlay(itype.getOverlay());

    // Set anim and stun (for backswing).
    if (impact_time > stun_until) stun_until = impact_time;
    delayedSetAnimFrame(AF_THROW_BACK);

    // The actual throw is done by a ThrowingTask (once the backswing completes).
    shared_ptr<ThrowingTask> thr_tsk(new
        ThrowingTask(static_pointer_cast<Creature>(shared_from_this()), itype));
    tm.addTask(thr_tsk, TP_NORMAL, impact_time);
}

void Creature::throwAwayItem(const ItemType *itype)
{
    // This routine gets rid of an item that was just thrown.
    // Default implementation assumes that the item is currently held.
    // (Knight overrides this routine to look in the backpack as well.)
    if (itype == getItemInHand()) setItemInHand(0);
}

//
// parrying
//

void Creature::parry()
{
    if (!getMap()) return;
    Mediator &mediator(Mediator::instance());
    const int finish_time = mediator.getGVT() + mediator.cfgInt("parry_delay");
    setAnimFrame(AF_PARRY, finish_time);
    stunUntil(finish_time);

    // run hook
    Mediator::instance().runHook("HOOK_WEAPON_PARRY", static_pointer_cast<Creature>(shared_from_this()));
}


//
// damage and hitpoints
//

void Creature::damage(int amount, const Originator &attacker,
                      int su /* = -1 */, bool inhibit_squelch /* = false */)
{
    if (amount > 0) {
        health -= amount;

        if (getMap()) {
            bool need_blood = true;
            const int blood_level = bloodLevel();
            if (blood_level==0) need_blood = false;
            if (blood_level==1 && health > 0) need_blood = false;
            if (need_blood) Mediator::instance().placeBlood(*getMap(), getNearestPos());
            if (health <= 0) {
                // Death!
                // call onDeath() (this causes knights to drop items, also does corpses);
                // remove myself from the map (this may also trigger on_withdraw events).
                onDeath(NORMAL_MODE, attacker);
                rmFromMap();
                return;  // no point continuing if creature is dead :)
            }
        }
    }
    if (su != -1) stunUntil(su);
}

void Creature::poison(const Originator &attacker)
{
    // death by poison.
    if (getMap()) {
        onDeath(POISON_MODE, attacker);
        rmFromMap();
    }
}

void Creature::addToHealth(int amount)
{
    if (amount > 0) {
        health += amount;
        if (health > max_health) health = max_health;
    }
}

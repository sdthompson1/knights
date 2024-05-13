/*
 * creature.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2024.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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
 * Base class for 'creatures' which currently includes knights,
 * vampire bats and zombies. (Derived from Entity.)
 *
 * This class implements combat including item-in-hand support,
 * swinging and parrying, a hitpoints system, and a 'stun' timer.
 *
 */

#ifndef CREATURE_HPP
#define CREATURE_HPP

#include "entity.hpp"
#include "originator.hpp"

class ItemType;
class Player;

class Creature : public Entity {
    friend class ImpactTask;
    friend class ThrowingTask;
public:
    // ctor, dtor
    Creature(int health, MapHeight height, ItemType * item_in_hand,
             const Anim * lower_anim, int base_speed);

    // various accessors
    int getHealth() const { return health; }
    int getMaxHealth() const { return max_health; }
    virtual MapHeight getHeight() const { return height; }
    virtual Player * getPlayer() const { return 0; }

    // getOriginator() returns what the "originator" should be for any action carried out by this creature.
    // (For a knight it will be OT_Player, for a monster it should be OT_Monster.)
    virtual Originator getOriginator() const { return Originator(OT_None()); } // default -- unknown originator.

    // item in hand
    ItemType * getItemInHand() const { return item_in_hand; }
    virtual void setItemInHand(ItemType * i)  // pass 0 to mean 'no item in hand'
        { doSetItemInHand(i); }

    // remove given no. of a given itemtype from inventory.
    // for creatures, this removes from item-in-hand (*if* the given itemtype is currently held).
    // for knights, this is overridden to remove items from the backpack as well.
    virtual void removeItem(const ItemType &itype, int number);


    //
    // Stunning a creature:
    //
    // A creature is stunned by (a) being paralyzed, or (b) by an
    // explicit call to stunUntil (which stuns the creature for a
    // predetermined length of time).
    //
    // In general, a stunned creature cannot perform any actions
    // (currently the only exception to this is A_Suicide). He will
    // however continue moving until he reaches his destination
    // square.
    //

    void stunUntil(int new_stun_time);

    // Check if the creature is currently stunned
    bool isStunned() const;

    // stunnedUntil: Calculates a lower bound for the time when the
    // creature will become un-stunned. If it is possible to calculate
    // a lower bound (greater than current gvt), then 'known' will be
    // set to true and 'stunned_until' will be set to that bound.
    // Otherwise, 'known' will be set to false and 'stunned_until'
    // will be left unchanged.
    void stunnedUntil(bool &known, int &stunned_until) const;


    
    //
    // combat / hitpoints stuff 
    //
    // damage(): this is virtual, and can be overridden although
    // you'll probably have to call Creature::damage as part of your
    // implementation. Eg Knight does this to filter out damage for
    // invulnerable knights, and to update the dungeonview.
    //
    // addToHealth(): likewise.
    //
    // poison(): poison damage. Default implementation kills the
    // creature outright. Knight overrides in order to implement
    // poison immunity and invulnerability.
    //
    // impactVeto(): This is called just before a weapon impacts
    // against this creature. If it returns TRUE then the impact is
    // cancelled ("vetoed"). 
    //

    bool canSwing() const;
    void swing();  // start a melee attack, setting anim and stun_time.

    // canThrow:
    //   - Could we throw an item, if we were carrying a suitable throwable item?
    //   - *Doesn't* check whether you have an item to throw or not.
    //   - Also doesn't check whether the creature is stunned.
    //   - "strict": Usually set to true. If false, this skips the missile-in-square-ahead check.
    //   This is used by A_Throw::possible(), and it allows Throw to be used as a continuous action.
    //   (Otherwise, holding FIRE+RIGHT to throw multiple daggers in succession wouldn't
    //   work, because the presence of the first dagger in the square ahead would make the 2nd
    //   throw impossible.)
    bool canThrow(bool strict) const;

    // canThrowItemInHand: equiv. to canThrow() && item_in_hand && item_in_hand->canThrow().
    bool canThrowItemInHand(bool strict) const;

    // throwItemInHand: start a throwing attack, setting anim and stun_time.
    void throwItemInHand();

    // Note: inhibit_squelch is a slight hack, it's used by bear traps to prevent the squelching sound
    // when you step on a bear trap. (All other damage to a creature should usually play the squelching sound.)
    virtual void damage(int amount, const Originator &originator, int stun_until = -1, bool inhibit_squelch = false);
    virtual int bloodLevel() const { return 2; }  // 1=only blood when die 0=no blood at all
    virtual void addToHealth(int amount);
    virtual void poison(const Originator &originator);
    bool impactVeto(int gvt, const Creature &attacker);

    void parry();
    
    // strength and quickness: these are overridden by Knights.
    // onDeath: is called by "damage" just before removing the dead creature from the map.
    // Used by Knights, to drop items before leaving. Also onDeath is responsible for placing
    // the corpse.
    // -- onDeath can also be called directly (eg this is done by PitKill). However note that
    // this won't take invulnerability into account.
    enum DeathMode { PIT_MODE,  // items to be dropped one square back; no corpse to be placed
                     ZOMBIE_MODE,  // no corpse to be placed
                     POISON_MODE,  // corpse to be placed but no blood
                     NORMAL_MODE,  // corpse to be placed, with blood
    };
    virtual void onDeath(DeathMode dmode, const Originator &originator) { }
    virtual bool hasStrength() const { return false; }
    virtual bool hasQuickness() const { return false; }

    // this gets called when this creature downswings
    // (which in practice, means what sound the creature makes when attacking)
    // Note: same hook is used for axe-throwing
    virtual void onDownswing() { }
    
    int getBaseSpeed() const { return speed; }

    // Stuff used only by EventManager
    // (The purpose of walkOverFlag is to prevent a creature from triggering the
    // walk-over event of a tile more than once, without actually walking off the
    // tile and back on again. E.g. some gnome books rely on this.)
    bool queryWalkOverFlag() const { return walk_over_pos != getPos(); }
    void resetWalkOverFlag() { walk_over_pos = getPos(); }
    
protected:
    void doSetItemInHand(ItemType *i);
    virtual bool isParalyzed() const { return false; } // if true, then makes the creature stunned (for an unknown time).

    // routines for throwing daggers, axes etc
    void doThrow(ItemType &);
    virtual void throwAwayItem(const ItemType *);
    
private:
    void doWithdrawEvents();

    void adjustImpactTimeIfMoving();
    void delayedSetAnimFrame(int fr);

private:
    MapHeight height;
    int health, max_health;
    int speed; // base speed, before quickness is applied
    int stun_until;  // 0 if not stunned. NB stun_until is guaranteed to be non-decreasing.
    ItemType * item_in_hand;
    int impact_time;  // if nonzero, the creature is attacking and will hit at this time.
    MapCoord walk_over_pos;
};

#endif

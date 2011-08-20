/*
 * knight.cpp
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

#include "dispel_magic.hpp"
#include "dungeon_map.hpp"
#include "dungeon_view.hpp"
#include "healing_task.hpp"
#include "item.hpp"
#include "knight.hpp"
#include "mediator.hpp"
#include "player.hpp"
#include "status_display.hpp"
#include "stuff_bag.hpp"
#include "task_manager.hpp"

Knight::Knight(Player &pl, const map<const ItemType *, int> * b,
               int health, MapHeight height, const ItemType * di, const Anim * anim,
               int speed)
    : Creature(health, height, di, anim, speed),
      default_item(di), b_capacity(b), player(pl), potion_magic(NO_POTION),
      potion_stop_time(0), invuln_stop_time(0), poison_immun_stop_time(0),
      sense_kt_stop_time(0), reveal_locn_stop_time(0), reveal_2(0), crystal_ball_count(0),
      dagger_time(0)
{
    pl.getStatusDisplay().setHealth(getHealth());
    resetMagic();
}

Knight::~Knight()
{
    // Tell Player that Knight has died (so that a new Knight can be respawned).
    player.onDeath();

    // Kill continuous messages.
    player.getDungeonView().cancelContinuousMessages();
}


void Knight::onDeath(DeathMode dmode, Player *attacker)
{
    if (!getMap()) return;

    // onDeath is called by Creature::damage when the knight's hitpoints fall to zero, and
    // also directly by PitKill. We use this to drop all items before dying.
    // (We can't do this from the dtor, because by that stage the knight has been removed from
    // the map and we don't know the knight's position any more.)
    dropAllItems(dmode == PIT_MODE);

    // Also: place a Knight corpse.
    if (dmode != PIT_MODE && dmode != ZOMBIE_MODE) {
        Mediator::instance().placeKnightCorpse(*getMap(), getNearestPos(), player,
                                               dmode == NORMAL_MODE);
    }

    // Also: do a "dispel magic" before we go (e.g. this may be needed for Sense Items)
    dispelMagic();

    // Credit a kill to the attacker
    if (attacker && attacker != getPlayer()) {
        attacker->addKill();
    }

    // Call onKnightDeath, for the tutorial
    if (getPlayer()) Mediator::instance().onKnightDeath(*getPlayer(), *getMap(), getNearestPos());
}


//
// magic
//

class ResetMagicTask : public Task {
public:
    ResetMagicTask(weak_ptr<Knight> k) : kt(k) { }
    virtual void execute(TaskManager &) {
        shared_ptr<Knight> k(kt.lock());
        if (k) k->resetMagic();
    }
private:
    weak_ptr<Knight> kt;
};

void Knight::resetMagic()
{
    TaskManager &tm(Mediator::instance().getTaskManager());
    PotionMagic current_potion = getPotionMagic();
    
    // visibility
    const bool currently_visible = isVisible();
    const bool should_be_visible = current_potion != INVISIBILITY;
    if (currently_visible != should_be_visible) {
        setVisible(should_be_visible);
    }

    // invulnerability (need to change anim data)
    setAnimInvulnerability(getInvulnerability());

    // quickness (entity speed)
    resetSpeed();

    // regeneration (normal or SUPER)
    if (regeneration_task) {
        tm.rmTask(regeneration_task);
        regeneration_task = shared_ptr<Task>();
    }
    if (current_potion == REGENERATION || current_potion == SUPER) {
        Mediator &mediator = Mediator::instance();
        const int dt = (current_potion==REGENERATION? mediator.cfgInt("regen_time")
            : mediator.cfgInt("super_regen_time"));
        const int amt = (current_potion==REGENERATION? mediator.cfgInt("regen_amount")
            : mediator.cfgInt("super_regen_amount"));
        regeneration_task.reset(new HealingTask(static_pointer_cast<Creature>
                                                (shared_from_this()), dt, amt));
        tm.addTask(regeneration_task, TP_NORMAL, tm.getGVT() + dt);
    }

    // Make sure that the dungeonview is kept up to date
    player.getStatusDisplay().setPotionMagic(current_potion, getPoisonImmunity());

    // Set the cts message, if paralyzed.
    getPlayer()->getDungeonView().cancelContinuousMessages();
    if (current_potion == PARALYZATION) {
        getPlayer()->getDungeonView().addContinuousMessage(Mediator::instance().cfgString("paralyzation_msg"));
    }
}

void Knight::resetSpeed()
{
    if (hasQuickness()) {
        setSpeed(getBaseSpeed() * Mediator::instance().cfgInt("quickness_factor") / 100);
    } else {
        setSpeed(getBaseSpeed());
    }
}   

void Knight::setPotionMagic(PotionMagic p, int stop_time) 
{
    // Filter out paralyzation if we have poison immunity.
    if (getPoisonImmunity() && p == PARALYZATION) p = NO_POTION;
    
    // Update the potion_magic fields
    potion_magic = p;
    potion_stop_time = stop_time;

    // Reset visibility, speed, etc, to reflect the new magic settings.
    resetMagic();

    // Set a task to reset the magic again when the effect runs out.
    setResetMagicTask(potion_stop_time);
}

void Knight::setResetMagicTask(int stop_time)
{
    shared_ptr<Task> t(new ResetMagicTask(static_pointer_cast<Knight>(shared_from_this())));
    Mediator::instance().getTaskManager().addTask(t, TP_NORMAL, stop_time+1);
}   

void Knight::setInvulnerability(bool i, int stop_time)
{
    invuln_stop_time = i ? stop_time : 0;
    resetMagic();
    if (i) setResetMagicTask(invuln_stop_time);
}

void Knight::setPoisonImmunity(bool pi, int stop_time)
{
    poison_immun_stop_time = pi ? stop_time : 0;
    resetMagic();
    if (pi) setResetMagicTask(poison_immun_stop_time);
}

void Knight::setSenseKnight(bool sk, int stop_time)
{
    sense_kt_stop_time = sk ? stop_time : 0;
}

void Knight::setRevealLocation(bool rl, int stop_time)
{
    reveal_locn_stop_time = rl ? stop_time : 0;
}

void Knight::setReveal2(bool rl)
{
    // Used for book of knowledge
    reveal_2 += (rl ? 1 : -1);
}

void Knight::setCrystalBall(bool cb)
{
    crystal_ball_count += (cb ? 1 : -1);
}

void Knight::dispelMagic()
{
    potion_magic = NO_POTION;
    invuln_stop_time = 0;
    poison_immun_stop_time = 0;
    sense_kt_stop_time = 0;
    reveal_locn_stop_time = 0;
    resetMagic();

    // notify all our observers
    shared_ptr<Knight> kt = static_pointer_cast<Knight>(shared_from_this());
    list<weak_ptr<DispelObserver> >::iterator it = dispel_list.begin();
    while (it != dispel_list.end()) {
        shared_ptr<DispelObserver> p = it->lock();
        if (p) {
            p->onDispel(kt);
            ++it;
        } else {
            dispel_list.erase(it++);
        }
    }
}

void Knight::startHomeHealing()
{
    Mediator &mediator = Mediator::instance();
    TaskManager &tm(mediator.getTaskManager());
    stopHomeHealing();
    const int dt = mediator.cfgInt("healing_time");
    home_healing_task.reset(new HealingTask(static_pointer_cast<Creature>(shared_from_this()),
                                            dt, mediator.cfgInt("healing_amount")));
    tm.addTask(home_healing_task, TP_NORMAL, tm.getGVT() + dt);
}

void Knight::stopHomeHealing()
{
    if (home_healing_task) {
        Mediator::instance().getTaskManager().rmTask(home_healing_task);
        home_healing_task = shared_ptr<Task>();
    }
}

PotionMagic Knight::getPotionMagic() const
{
    return Mediator::instance().getGVT() >= potion_stop_time ? NO_POTION : potion_magic;
}

bool Knight::getInvulnerability() const
{
    return (Mediator::instance().getGVT() < invuln_stop_time);
}

bool Knight::getPoisonImmunity() const
{
    return (Mediator::instance().getGVT() < poison_immun_stop_time);
}

bool Knight::getSenseKnight() const
{
    return (Mediator::instance().getGVT() < sense_kt_stop_time);
}

bool Knight::getRevealLocation() const
{
    return (reveal_2 > 0) || (Mediator::instance().getGVT() < reveal_locn_stop_time);
}

bool Knight::getCrystalBall() const
{
    return crystal_ball_count > 0;
}

bool Knight::isParalyzed() const
{
    return (getPotionMagic() == PARALYZATION);
}

bool Knight::hasStrength() const
{
    PotionMagic pm = getPotionMagic();
    return pm == STRENGTH || pm == SUPER;
}

bool Knight::hasQuickness() const
{
    PotionMagic pm = getPotionMagic();
    return pm == QUICKNESS || (pm == SUPER && getHealth() == getMaxHealth());
}


//
// inventory
//

void Knight::setItemInHand(const ItemType *i)
{
    if (i) {
        doSetItemInHand(i);
    } else {
        doSetItemInHand(default_item);
    }
}

int Knight::backpackFind(const ItemType &itype) const
{
    for (int i=0; i<backpack.size(); ++i) {
        if (backpack[i].first == &itype) return i;
    }
    return -1;
}

int Knight::getNumCarried(const ItemType &itype) const
{
    int idx = backpackFind(itype);
    if (idx == -1) return 0;
    else return backpack[idx].second;
}

bool Knight::canAddToBackpack(const ItemType &itype) const
{
    // true if the item can be at least partially picked up
    int cur_no = 0;
    int idx = backpackFind(itype);
    if (idx != -1) cur_no = backpack[idx].second;
    int maxno = 9999;
    if (b_capacity) {
        map<const ItemType*, int>::const_iterator it = b_capacity->find(&itype);
        if (it != b_capacity->end()) maxno = it->second;
    }
    return (cur_no < maxno);
}

int Knight::getMaxNo(const ItemType &itype) const
{
    // note: this returns '0' to mean 'no maximum'. be careful!
    if (b_capacity) {
        map<const ItemType*, int>::const_iterator it = b_capacity->find(&itype);
        if (it != b_capacity->end()) return it->second;
    }
    return 0;
}

int Knight::addToBackpack(const ItemType &itype, int no_to_add)
{
    if (no_to_add <= 0) return 0;
    
    int idx = backpackFind(itype);
    if (idx == -1) {
        backpack.push_back(make_pair(&itype, 0));
        idx = backpack.size()-1;
    }
    
    int & no(backpack[idx].second);
    const int oldno = no;

    int maxno = getMaxNo(itype);

    no += no_to_add;
    if (no > maxno && maxno != 0) no = maxno;
    if (no != oldno) getPlayer()->getStatusDisplay().setBackpack
                         (itype.getBackpackSlot(), itype.getBackpackGraphic(),
                          itype.getBackpackOverdraw(), no, maxno);
    return no - oldno;
}

void Knight::rmFromBackpack(const ItemType &itype, int no_to_rm)
{
    int idx = backpackFind(itype);
    if (idx == -1) return;

    int & no(backpack[idx].second);
    no -= no_to_rm;
    if (no <= 0) {
        // Number has fallen to zero. Delete it from the backpack vector entirely.
        if (idx < backpack.size()-1) swap(backpack[idx], backpack.back());
        backpack.pop_back();
        getPlayer()->getStatusDisplay().setBackpack(itype.getBackpackSlot(),0,0,0,0);
    } else {
        getPlayer()->getStatusDisplay().setBackpack(itype.getBackpackSlot(),
            itype.getBackpackGraphic(), itype.getBackpackOverdraw(), no, getMaxNo(itype));
    }
}




//
// dropAllItems
//

void Knight::dropAllItems(bool move_back /* = false */)
{
    if (!getMap()) return;

    shared_ptr<Knight> self = static_pointer_cast<Knight>(shared_from_this());
    StuffManager &stuff_manager(Mediator::instance().getStuffManager());

    // If "move_back" is true then we drop the stuff one square behind
    // the knight's current position.  This is used when a knight
    // falls down a pit -- we don't want important items being dropped
    // on wrong side of a pit, rendering the quest impossible.
    MapCoord mc = getPos();
    if (move_back) mc = DisplaceCoord(mc, Opposite(getFacing()));

    // Unload the backpack into a "StuffContents" object. Also see if the stuff amounts
    // to only a single item (bkpk_single_item); in this case a stuff bag won't be used.
    StuffContents bkpk_contents;
    unloadBackpack(bkpk_contents);
    const bool must_drop_stuff = !bkpk_contents.isEmpty();
    shared_ptr<Item> bkpk_single_item = bkpk_contents.getSingleItem();
    ASSERT(getBackpackCount() == 0);  // check that backpack was fully unloaded!

    const MapDirection pref_drop_dir = Opposite(getFacing());
    
    if (canDropHeld()) {
        // We have an item-in-hand that needs to be dropped
        shared_ptr<Item> item(new Item(*getItemInHand()));
        const bool success = DropItem(item, *getMap(), mc, true, true, pref_drop_dir, self);

        if (!success) {
            // could not drop it into the map. so add to "displaced items" (for later respawning)
            // and also manually run the on_drop event.
            getMap()->addDisplacedItem(item);
            item->getType().onDrop(*getMap(), MapCoord(), self);            
        }

        setItemInHand(0);
    }

    if (must_drop_stuff) {
        // We have backpack item(s) that need to be dropped

        if (bkpk_single_item) {
            // Ordinary drop (don't use stuff bags).
            const bool success = DropItem(bkpk_single_item, *getMap(), mc, true, true, pref_drop_dir, self);

            if (!success && bkpk_single_item->getNumber() != 0) {
                // Was not (fully) dropped into the map - must add it to the displaced items
                getMap()->addDisplacedItem(bkpk_single_item);
                bkpk_single_item->getType().onDrop(*getMap(), MapCoord(), self);
            }

        } else {
            // Try to drop a stuff bag.
            shared_ptr<Item> stuff_bag(new Item(stuff_manager.getStuffBagItemType()));
            MapCoord drop_mc;
            const bool success = DropItem(stuff_bag, *getMap(), mc, true, true, pref_drop_dir, self, &drop_mc);
            if (success) {
                // Note that DropItem will run the stuff bag's onDrop event; but this
                // will not actually have done anything (because we haven't set the
                // contents in the StuffManager yet). So we next have to set contents in
                // the StuffManager and re-call the onDrop event.
                stuff_manager.setStuffContents(drop_mc, bkpk_contents);
                stuff_manager.doDrop(drop_mc, self);
            } else {
                // Stuff bag could not be dropped. Instead, add all the backpack items
                // to displaced list.
                bkpk_contents.putIntoDisplacedItems(*getMap(), self);
            }
        }
    }
}

void Knight::unloadBackpack(StuffContents &sc)
{
    // Empties out the Knight's backpack, and returns a stuff bag or single item as
    // appropriate.

    while (getBackpackCount() > 0) {
        int i = getBackpackCount() - 1;
        const ItemType &itype(getBackpackItem(i));
        const int num = getNumCarried(i);
        int no_to_rm = min(num, itype.getMaxStack());
        shared_ptr<Item> item(new Item(itype, no_to_rm));
        sc.addItem(item);
        rmFromBackpack(itype, no_to_rm);
    }
}


//
// damage, poison, addToHealth
//

void Knight::damage(int amount, Player *attacker, int stun_until, bool inhibit_squelch)
{
    // "filter out" damage if knight has invulnerability.
    if (!getInvulnerability()) {
        // run damage hook(s)
        shared_ptr<Knight> self(static_pointer_cast<Knight>(shared_from_this()));
        Mediator::instance().runHook("HOOK_KNIGHT_DAMAGE", self);
        if (amount > 0 && !inhibit_squelch) {
            Mediator::instance().runHook("HOOK_CREATURE_SQUELCH", self);
        }

        // call base class damage
        Creature::damage(amount, attacker, stun_until, inhibit_squelch);

        // update health
        player.getStatusDisplay().setHealth(getHealth());
        
        // Note we CANNOT call resetMagic here because of a subtle interaction between rmTask
        // (which is done by resetMagic) and addTask (which is done by HealingTask) on the
        // regeneration_task, with the result that the RegenerationTask gets added twice :-(
        // Therefore, we call resetSpeed only.
        resetSpeed(); // recalculate speed of SUPER knights.
    }
}

void Knight::addToHealth(int amount)
{
    Creature::addToHealth(amount);
    player.getStatusDisplay().setHealth(getHealth());
    resetSpeed();  // may be necessary if you have super.
}

void Knight::poison(Player *attacker)
{
    // "filters out" poison if knight has invulnerability or poison immunity
    if (!getPoisonImmunity() && !getInvulnerability()) {
        // run damage hook (BEFORE calling poison(), i.e. while the knight is still around!)
        Mediator::instance().runHook("HOOK_KNIGHT_DAMAGE", static_pointer_cast<Knight>(shared_from_this()));

        // poison the knight
        Creature::poison(attacker);
        
        // also, we want to set the magic status to PARALYZATION (This turns the potion bottle
        // green).
        getPlayer()->getStatusDisplay().setPotionMagic(PARALYZATION, false);
    }
}


//
// throwing of items
//

void Knight::throwAwayItem(const ItemType *i)
{
    if (i) {
        if (getItemInHand() == i) {
            setItemInHand(0);
        } else {
            rmFromBackpack(*i, 1);
        }
    }
    if (getItemInHand()) setOverlay(getItemInHand()->getOverlay());
}

bool Knight::canThrowItem(const ItemType &it, bool strict) const
{
    if (!canThrow(strict)) return false;
    if (getNumCarried(it)==0) return false;
    if (!it.canThrow()) return false;
    return true;
}

void Knight::throwItem(const ItemType &it)
{
    if (!canThrowItem(it, true)) return;
    const bool use_time_penalty = !it.isBig() && getPlayer()->getActionBarControls();

    if (use_time_penalty) {
        Mediator &med = Mediator::instance();
        const int gvt = med.getGVT();
        if (!getDaggerThrownFlag()) {
            dagger_time = gvt + med.cfgInt("dagger_time_delay");
            setDaggerThrownFlag();
        }
        if (gvt < dagger_time) {
            // not time to throw daggers yet!
            return;
        }
    }
    doThrow(it);
}

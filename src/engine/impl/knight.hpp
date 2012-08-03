/*
 * knight.hpp
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
 * Knight: represents a player (knight) in the dungeon.
 *
 */

#ifndef KNIGHT_HPP
#define KNIGHT_HPP

#include "creature.hpp"
#include "potion_magic.hpp"

#include <list>
#include <map>
#include <vector>
using namespace std;

class DispelObserver;
class DungeonView;
class ItemType;
class Player;
class StuffContents;
class Task;

class Knight : public Creature {
public:
    // if backpack_capacities is non-null, then it points to a map
    // giving the maximum number for each itemtype. (if an itemtype is
    // not in the map then the number held is assumed to be
    // unlimited.) (the map is not copied!)
    Knight(Player &pl, const map<const ItemType *, int> * backpack_capacities,
           int health, MapHeight height, ItemType * default_item,
           const Anim * lower_anim, int spd);
    virtual ~Knight();

    virtual Player * getPlayer() const { return &player; }
    virtual Originator getOriginator() const { return Originator(OT_Player(), &player); }
    

    //
    // Magic
    //
    // "Potion magic" controls invisibility, strength, quickness,
    // regeneration, super (only one of which can be active at a
    // time). Invulnerability and poison immunity can be turned on and
    // off independently. In each case "stop_time" gives the gvt after
    // which the effect runs out.
    //
    void setPotionMagic(PotionMagic, int stop_time);
    void setInvulnerability(bool i, int stop_time);
    void setPoisonImmunity(bool pi, int stop_time);
    void setSenseKnight(bool sk, int stop_time);
    void setRevealLocation(bool rl, int stop_time);
    void setReveal2(bool rl);
    void setCrystalBall(bool cb);  // uses a counter.
    void dispelMagic();  // cancel all magic effects
    PotionMagic getPotionMagic() const;
    bool getInvulnerability() const;
    bool getPoisonImmunity() const;
    bool getSenseKnight() const;
    bool getRevealLocation() const;
    bool getCrystalBall() const;

    void startHomeHealing();
    void stopHomeHealing();

    // we may add "dispel observers":
    // (There is no "rm" function but they are automatically removed
    // when the weak pointer expires.)
    void addDispelObserver(weak_ptr<DispelObserver> o) { dispel_list.push_front(o); }
    
    
    // overridden from creature:
    virtual void onDeath(DeathMode, const Originator &originator);  // calls dropAllItems, also places a knight corpse
    virtual bool hasStrength() const;
    virtual bool hasQuickness() const;

    // overridden from Entity:
    virtual bool isVisibleToPlayer(const Player &p) const;

    virtual void onDownswing();

    //
    // Inventory 
    //
    // -- item in hand is handled by Creature (but we override
    // Creature::setItemInHand to ensure that the default item is
    // handled properly).
    //
    // -- "backpack" items are stored as (itemtype, number_held) pairs.
    //
    virtual void setItemInHand(ItemType *i);
    bool canDropHeld() const { return getItemInHand() && getItemInHand() != default_item; }

    virtual void removeItem(const ItemType &itype, int number); // removes from either item-in-hand or backpack    
    
    int getBackpackCount() const { return backpack.size(); }   // get no of itemtypes in backpack
    ItemType & getBackpackItem(int idx) const { return *backpack[idx].first; }   // get itemtype
    int getNumCarried(int idx) const { return backpack[idx].second; }                  // get number held (by index)
    int getNumCarried(const ItemType &itype) const;     // get number held (by itemtype)
    int getMaxNo(const ItemType &) const;  // returns 0 if no maximum (be careful!)
    
    bool canAddToBackpack(const ItemType &itype) const;  // true if (at least one of) "itype" can be added
    int addToBackpack(ItemType &itype, int no_to_add);  // returns number *actually* added.
    void rmFromBackpack(const ItemType &itype, int no_to_rm);
    
    void dropAllItems(bool move_back = false);  // drops the entire inventory.

    
    // 
    // override damage(), poison() to check for invulnerability and/or poison immunity
    // (and also to tell DungeonView about updated health, and to run damage hooks).
    //
    virtual void damage(int amount, const Originator &originator, int stun_until = -1, bool inhibit_squelch = false);
    virtual void addToHealth(int amount);
    virtual void poison(const Originator &attacker);


    // throwing of backpack items (as opposed to item-in-hand).
    // canThrowItem: true if Creature::canThrow() and you are carrying itemtype and itemtype->canThrow().
    bool canThrowItem(const ItemType &itemtype, bool strict) const;
    void throwItem(ItemType &itemtype);
    

protected:
    virtual bool isParalyzed() const;
    virtual void throwAwayItem(const ItemType *);
    
private:
    void unloadBackpack(StuffContents &sc);
    int backpackFind(const ItemType &) const;
    void resetMagic();
    void resetSpeed();
    void setResetMagicTask(int);
    friend class ResetMagicTask;
    
private:
    typedef vector<pair<ItemType *, int> > BackpackType;
    BackpackType backpack;
    ItemType * default_item;
    const map<const ItemType*, int> * b_capacity;
    Player & player;
    PotionMagic potion_magic; int potion_stop_time;
    int invuln_stop_time;
    int poison_immun_stop_time;
    int sense_kt_stop_time;
    int reveal_locn_stop_time;
    int reveal_2;
    int crystal_ball_count;
    shared_ptr<Task> regeneration_task, home_healing_task;
    list<weak_ptr<DispelObserver> > dispel_list;
    int dagger_time;
};

#endif

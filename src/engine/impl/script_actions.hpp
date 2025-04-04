/*
 * script_actions.hpp
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
 * LegacyActions usable from the config system (used for switch effects etc)
 *
 */

#ifndef SCRIPT_ACTIONS_HPP
#define SCRIPT_ACTIONS_HPP

#include "legacy_action.hpp"

class A_ChangeItem : public LegacyAction {
public:
    explicit A_ChangeItem(ItemType *itype) : item_type(itype) { }
    virtual void execute(const ActionData &) const;
private:
    ItemType *item_type;
    ACTION_MAKER("ChangeItem");
};

class A_ChangeTile : public LegacyAction {
public:
    explicit A_ChangeTile(shared_ptr<Tile> t) : tile(t) { }
    virtual void execute(const ActionData &) const;
private:
    shared_ptr<Tile> tile;
    ACTION_MAKER("ChangeTile");
};

class A_CrystalStart : public LegacyAction {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("CrystalStart");
};

class A_CrystalStop : public LegacyAction {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("CrystalStop");
};

class A_Damage : public LegacyAction {
public:
    A_Damage(int amt, int st, bool is) : amount(amt), stun_time(st), inhibit_squelch(is) { }
    virtual void execute(const ActionData &) const;
private:
    int amount;
    int stun_time;
    bool inhibit_squelch;  // special hack used only for bear traps (at the moment)
    ACTION_MAKER("Damage");
};

class A_FlashMessage : public LegacyAction {
public:
    explicit A_FlashMessage(const std::string &m, int nt) : msg(m), num_times(nt) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("FlashMessage");
    std::string msg;
    int num_times;
};

class A_FlashScreen : public LegacyAction {
public:
    explicit A_FlashScreen(int d) : delay(d) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("FlashScreen");
    int delay;
};

class A_FullZombieActivity : public LegacyAction {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("FullZombieActivity");
};

class A_Necromancy : public LegacyAction {
public:
    A_Necromancy(int nzom, int rang) : nzoms(nzom), range(rang) { }
    virtual bool possible(const ActionData &) const;
    virtual bool executeWithResult(const ActionData &) const;
private:
    ACTION_MAKER("Necromancy");
    int nzoms, range;
};

class A_Nop : public LegacyAction {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Nop");
};

class A_NormalZombieActivity : public LegacyAction {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("NormalZombieActivity");
};

class A_PitKill : public LegacyAction {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("PitKill");
};

class A_RevealStart : public LegacyAction {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("RevealStart");
};

class A_RevealStop : public LegacyAction {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("RevealStop");
};

// ZombieKill kills a monster of a particular type
// (it doesn't have to be a zombie; the name is for historical reasons)
class A_ZombieKill : public LegacyAction {
public:
    explicit A_ZombieKill(const MonsterType &zom_type_) : zom_type(zom_type_) { }
    virtual bool possible(const ActionData &) const;
    virtual bool executeWithResult(const ActionData &) const;
private:
    ACTION_MAKER("ZombieKill");
    const MonsterType &zom_type;
};

#endif

    

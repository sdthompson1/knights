/*
 * control_actions.hpp
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

#ifndef CONTROL_ACTIONS_HPP
#define CONTROL_ACTIONS_HPP

class KnightsConfigImpl;

#include "legacy_action.hpp"

#include <vector>

class Control;


//
// Adds the standard controls to a KnightsConfigImpl
//
void AddStandardControls(lua_State *lua, KnightsConfigImpl *kc);


//
// LegacyActions used by the standard controls (including Activate)
//

class A_Activate : public LegacyAction {
public:
    A_Activate(int dx_, int dy_) : dx(dx_), dy(dy_) { }
    virtual bool possible(const ActionData &) const;
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Activate");
    int dx, dy;
};

class A_Attack : public LegacyAction {
public:
    explicit A_Attack(MapDirection d) : dir(d) { }
    virtual bool canExecuteWhileMoving() const { return true; }
    virtual void execute(const ActionData &) const;
private:
    MapDirection dir;
};

class A_AttackNoDir : public LegacyAction {
public:
    virtual bool canExecuteWhileMoving() const { return true; }
    virtual void execute(const ActionData &) const;
};

class A_Move : public LegacyAction {
public:
    explicit A_Move(MapDirection d) : dir(d) { }
    virtual bool canExecuteWhileMoving() const { return true; }
    virtual void execute(const ActionData &) const;
private:
    MapDirection dir;
};

class A_Withdraw : public LegacyAction {
public:
    virtual void execute(const ActionData &) const;
};


//
// Other control-related actions.
//

class A_Drop : public LegacyAction {
public:
    explicit A_Drop(ItemType &it_) : it(it_) { }
    virtual bool possible(const ActionData &) const;
    virtual void execute(const ActionData &) const;
private:
    ItemType &it;
    ACTION_MAKER("Drop");
};

class A_DropHeld : public LegacyAction {
public:
    virtual bool possible(const ActionData &) const;
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("DropHeld");
};

class A_PickLock : public LegacyAction {
public:
    A_PickLock(float p, int wt) : prob(p), waiting_time(wt) { }
    virtual bool possible(const ActionData &) const;
    virtual void execute(const ActionData &) const;
private:
    float prob;
    int waiting_time;
    ACTION_MAKER("PickLock");
};

class A_PickUp : public LegacyAction {
public:
    virtual bool possible(const ActionData &) const;
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("PickUp");
};

class A_SetBearTrap : public LegacyAction {
public:
    explicit A_SetBearTrap(ItemType &trap_) : trap_itype(trap_) { }
    virtual bool possible(const ActionData &) const;
    virtual void execute(const ActionData &) const;
private:
    ItemType &trap_itype;
    ACTION_MAKER("SetBearTrap");
};

class A_SetBladeTrap : public LegacyAction {
public:
    explicit A_SetBladeTrap(ItemType &missile) : missile_type(missile) { }
    virtual bool possible(const ActionData &) const;
    virtual void execute(const ActionData &) const;
private:
    ItemType &missile_type;
    ACTION_MAKER("SetBladeTrap");
};

class A_SetPoisonTrap : public LegacyAction {
public:
    virtual bool possible(const ActionData &) const;
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("SetPoisonTrap");
};

class A_Suicide : public LegacyAction {
public:
    virtual bool canExecuteWhileStunned() const { return true; }
    virtual bool canExecuteWhileMoving() const { return true; }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Suicide");
};

class A_Swing : public LegacyAction {
public:
    virtual bool possible(const ActionData &) const;
    virtual bool canExecuteWhileMoving() const { return true; }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Swing");
};

class A_SwingOrDrop : public LegacyAction {
public:
    virtual bool possible(const ActionData &) const;
    virtual bool canExecuteWhileMoving() const { return true; }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("SwingOrDrop");
};

// A_Throw is usually used for throwing daggers. It looks at the
// ActionData Item parameter to determine which item to throw.
class A_Throw : public LegacyAction {
public:
    virtual bool possible(const ActionData &) const;
    virtual bool canExecuteWhileMoving() const { return true; }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Throw");
};

// ThrowOrShoot is used for the "throw weapon" control button. Usually used 
// for throwing daggers, shooting crossbows or throwing the axe.
class A_ThrowOrShoot : public LegacyAction {
public:
    virtual bool canExecuteWhileMoving() const { return true; }
    virtual bool possible(const ActionData &) const;
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("ThrowOrShoot");
};

#endif

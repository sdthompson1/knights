/*
 * magic_actions.hpp
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
 * Actions related to magic (potions and scrolls).
 * 
 * Most of these actually do the same thing which is just to call
 * Knight::setPotionMagic (or its variants setInvulnerability and
 * setPoisonImmunity).
 *
 */

#ifndef MAGIC_ACTIONS_HPP
#define MAGIC_ACTIONS_HPP

#include "action.hpp"

// kconfig
#include "kconfig_fwd.hpp"

class A_Attractor : public Action {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Attractor");
};

class A_DispelMagic : public Action {
public:
    explicit A_DispelMagic(const string &m) : msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("DispelMagic");
    string msg;
};

class A_Healing : public Action {
public:
    explicit A_Healing(const string &m) : msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Healing");
    string msg;
};

class A_Invisibility : public Action {
public:
    explicit A_Invisibility(const KConfig::RandomInt * ri, const string &m) : dur(ri), msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Invisibility");
    const KConfig::RandomInt *dur;
    string msg;
};

class A_Invulnerability : public Action {
public:
    explicit A_Invulnerability(const KConfig::RandomInt *ri, const string &m) : dur(ri), msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Invulnerability");
    const KConfig::RandomInt *dur;
    string msg;
};

class A_MagicMapping : public Action {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("MagicMapping");
};

class A_OpenWays : public Action {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("OpenWays");
};

class A_Paralyzation : public Action {
public:
    explicit A_Paralyzation(const KConfig::RandomInt *ri) : dur(ri) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Paralyzation");
    const KConfig::RandomInt *dur;
};

class A_Poison : public Action {
public:
    explicit A_Poison(const string &m) : msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Poison");
    string msg;
};

class A_PoisonImmunity : public Action {
public:
    explicit A_PoisonImmunity(const KConfig::RandomInt *ri, const string &m) : dur(ri), msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("PoisonImmunity");
    const KConfig::RandomInt *dur;
    string msg;
};

class A_Quickness : public Action {
public:
    explicit A_Quickness(const KConfig::RandomInt * ri, const string &m) : dur(ri), msg(m) { }    
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Quickness");
    const KConfig::RandomInt *dur;
    string msg;
};

class A_Regeneration : public Action {
public:
    explicit A_Regeneration(const KConfig::RandomInt *ri, const string &m) : dur(ri), msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Regeneration");
    const KConfig::RandomInt *dur;
    string msg;
};

class A_RevealLocation : public Action {
public:
    explicit A_RevealLocation(const KConfig::RandomInt * ri) : dur(ri) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("RevealLocation");
    const KConfig::RandomInt *dur;
};

class A_SenseItems : public Action {
public:
    explicit A_SenseItems(const KConfig::RandomInt * ri) : dur(ri) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("SenseItems");
    const KConfig::RandomInt *dur;
};    

class A_SenseKnight : public Action {
public:
    explicit A_SenseKnight(const KConfig::RandomInt * ri) : dur(ri) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("SenseKnight");
    const KConfig::RandomInt *dur;
};

class A_Strength : public Action {
public:
    explicit A_Strength(const KConfig::RandomInt * ri, const string &m) : dur(ri), msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Strength");
    const KConfig::RandomInt *dur;
    string msg;
};

class A_Super : public Action {
public:
    explicit A_Super(const KConfig::RandomInt * ri, const string &m) : dur(ri), msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Super");
    const KConfig::RandomInt *dur;
    string msg;
};

class A_Teleport : public Action {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Teleport");
};

class A_WipeMap : public Action {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("WipeMap");
};

class A_ZombifyActor : public Action {
public:
    explicit A_ZombifyActor(const MonsterType &zom_type_) : zom_type(zom_type_) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("ZombifyActor");
    const MonsterType & zom_type;
};

class A_ZombifyTarget : public Action {
public:
    explicit A_ZombifyTarget(const MonsterType &zom_type_) : zom_type(zom_type_) { }
    virtual bool possible(const ActionData &) const;
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("ZombifyTarget");
    const MonsterType & zom_type;
};



#endif

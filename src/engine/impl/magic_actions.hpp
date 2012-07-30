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
 * LegacyActions related to magic (potions and scrolls).
 * 
 * Most of these actually do the same thing which is just to call
 * Knight::setPotionMagic (or its variants setInvulnerability and
 * setPoisonImmunity).
 *
 */

#ifndef MAGIC_ACTIONS_HPP
#define MAGIC_ACTIONS_HPP

#include "legacy_action.hpp"

class A_Attractor : public LegacyAction {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Attractor");
};

class A_DispelMagic : public LegacyAction {
public:
    explicit A_DispelMagic(const string &m) : msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("DispelMagic");
    string msg;
};

class A_Healing : public LegacyAction {
public:
    explicit A_Healing(const string &m) : msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Healing");
    string msg;
};

class A_Invisibility : public LegacyAction {
public:
    explicit A_Invisibility(int i, const string &m) : dur(i), msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Invisibility");
    int dur;
    string msg;
};

class A_Invulnerability : public LegacyAction {
public:
    explicit A_Invulnerability(int i, const string &m) : dur(i), msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Invulnerability");
    int dur;
    string msg;
};

class A_MagicMapping : public LegacyAction {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("MagicMapping");
};

class A_OpenWays : public LegacyAction {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("OpenWays");
};

class A_Paralyzation : public LegacyAction {
public:
    explicit A_Paralyzation(int i) : dur(i) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Paralyzation");
    int dur;
};

class A_Poison : public LegacyAction {
public:
    explicit A_Poison(const string &m) : msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Poison");
    string msg;
};

class A_PoisonImmunity : public LegacyAction {
public:
    explicit A_PoisonImmunity(int i, const string &m) : dur(i), msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("PoisonImmunity");
    int dur;
    string msg;
};

class A_Quickness : public LegacyAction {
public:
    explicit A_Quickness(int i, const string &m) : dur(i), msg(m) { }    
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Quickness");
    int dur;
    string msg;
};

class A_Regeneration : public LegacyAction {
public:
    explicit A_Regeneration(int i, const string &m) : dur(i), msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Regeneration");
    int dur;
    string msg;
};

class A_RevealLocation : public LegacyAction {
public:
    explicit A_RevealLocation(int i) : dur(i) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("RevealLocation");
    int dur;
};

class A_SenseItems : public LegacyAction {
public:
    explicit A_SenseItems(int i) : dur(i) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("SenseItems");
    int dur;
};    

class A_SenseKnight : public LegacyAction {
public:
    explicit A_SenseKnight(int i) : dur(i) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("SenseKnight");
    int dur;
};

class A_Strength : public LegacyAction {
public:
    explicit A_Strength(int i, const string &m) : dur(i), msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Strength");
    int dur;
    string msg;
};

class A_Super : public LegacyAction {
public:
    explicit A_Super(int i, const string &m) : dur(i), msg(m) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Super");
    int dur;
    string msg;
};

class A_Teleport : public LegacyAction {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("Teleport");
};

class A_WipeMap : public LegacyAction {
public:
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("WipeMap");
};

class A_ZombifyActor : public LegacyAction {
public:
    explicit A_ZombifyActor(const MonsterType &zom_type_) : zom_type(zom_type_) { }
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("ZombifyActor");
    const MonsterType & zom_type;
};

class A_ZombifyTarget : public LegacyAction {
public:
    explicit A_ZombifyTarget(const MonsterType &zom_type_) : zom_type(zom_type_) { }
    virtual bool possible(const ActionData &) const;
    virtual void execute(const ActionData &) const;
private:
    ACTION_MAKER("ZombifyTarget");
    const MonsterType & zom_type;
};



#endif

/*
 * menu_constraints.cpp
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

#include "kfile.hpp"
#include "knights_config_impl.hpp"
#include "menu_constraints.hpp"
#include "menu_int.hpp"
#include "menu_selections.hpp"

//
// MenuConstraint base class
//

class MenuConstraint {
public:
    MenuConstraint(const std::string &k, int v) : key(k), val(v) { }
    bool applicable(const MenuSelections &msel) const { return key.empty() || msel.getValue(key) == val; }
    bool isQuest() const { return key == "quest"; }  // see below
    
    virtual ~MenuConstraint() { }
    virtual bool apply(MenuSelections &msel, int nplayers) const = 0;
    virtual bool canSetTo(const MenuSelections &msel, const std::string &key, int val, int nplayers) const = 0;

private:
    // The constraint is only active if the given key has the given value.
    // (unless key.empty(), in which case the constraint is always active.)
    std::string key;
    int val;
};

//
// Constrain
//

class Constrain : public MenuConstraint {
public:
    Constrain(const std::string &ak, int av, const std::string &ck, const MenuInt * cv) 
        : MenuConstraint(ak,av), key(ck), val(cv) { }
    virtual bool apply(MenuSelections &msel, int nplayers) const;
    virtual bool canSetTo(const MenuSelections &msel, const string &k, int v, int nplayers) const;
private:
    std::string key;
    const MenuInt *val;
};

bool Constrain::apply(MenuSelections &msel, int) const
{
    if (!val) return false;
    const int new_val = val->getValue(msel);
    if (msel.getValue(key) == new_val) return false;
    msel.setValue(key, new_val);
    return true;
}

bool Constrain::canSetTo(const MenuSelections &msel, const string &k, int v, int) const
{
    if (k != key) return true;
    const int new_val = val->getValue(msel);
    return v == new_val;
}

//
// ConstrainMin
//

class ConstrainMin : public MenuConstraint {
public:
    ConstrainMin(const string &ak, int av, const string &ck, const MenuInt * cv) : MenuConstraint(ak,av), key(ck), val(cv) { }
    virtual bool apply(MenuSelections &msel, int nplayers) const;
    virtual bool canSetTo(const MenuSelections &msel, const string &k, int v, int nplayers) const;
private:
    std::string key;
    const MenuInt *val;
};

bool ConstrainMin::apply(MenuSelections &msel, int) const
{
    if (!val) return false;
    const int mn = val->getValue(msel);
    const int cur = msel.getValue(key);
    if (cur < mn) {
        msel.setValue(key, mn);
        return true;
    } else {
        return false;
    }
}

bool ConstrainMin::canSetTo(const MenuSelections &msel, const string &k, int v, int) const
{
    if (k != key) return true;
    const int mn = val->getValue(msel);
    return v >= mn;
}


//
// ConstrainMax
//

class ConstrainMax : public MenuConstraint {
public:
    ConstrainMax(const string &ak, int av, const string &ck, const MenuInt * cv) : MenuConstraint(ak,av), key(ck), val(cv) { }
    virtual bool apply(MenuSelections &msel, int nplayers) const;
    virtual bool canSetTo(const MenuSelections &msel, const std::string &k, int v, int nplayers) const;
private:
    std::string key;
    const MenuInt *val;
};

bool ConstrainMax::apply(MenuSelections &msel, int) const
{
    if (!val) return false;
    const int mx = val->getValue(msel);
    const int cur = msel.getValue(key);
    if (cur > mx) {
        msel.setValue(key, mx);
        return true;
    } else {
        return false;
    }
}

bool ConstrainMax::canSetTo(const MenuSelections &msel, const string &k, int v, int) const
{
    if (k != key) return true;
    const int mx = val->getValue(msel);
    return v <= mx;
}


//
// ConstrainList
//

// NOTE: Be careful with this because the constraint solver isn't too intelligent.
// The FIRST entry in the list should be a "safe" setting that is always valid,
// otherwise the constraint solver may get stuck in a loop and start the quest with invalid settings.

class ConstrainList : public MenuConstraint {
public:
    ConstrainList(const string &ak, int av, const string &ck, const std::vector<const MenuInt *> &cv)
        : MenuConstraint(ak, av), key(ck), values(cv) { ASSERT(!values.empty()); }
    virtual bool apply(MenuSelections &msel, int nplayers) const;
    virtual bool canSetTo(const MenuSelections &msel, const std::string &k, int v, int nplayers) const;
private:
    std::string key;
    std::vector<const MenuInt *> values;
};

bool ConstrainList::apply(MenuSelections &msel, int) const
{
    const int cur = msel.getValue(key);
    for (std::vector<const MenuInt *>::const_iterator it = values.begin(); it != values.end(); ++it) {
        const int allowed_val = (*it)->getValue(msel);
        if (cur == allowed_val) {
            return false;
        }
    }
    msel.setValue(key, values.front()->getValue(msel));
    return true;
}

bool ConstrainList::canSetTo(const MenuSelections &msel, const string &k, int v, int) const
{
    if (k != key) return true;
    for (std::vector<const MenuInt *>::const_iterator it = values.begin(); it != values.end(); ++it) {
        const int allowed_val = (*it)->getValue(msel);
        if (v == allowed_val) return true;
    }
    return false;
}


//
// ConstrainMinMaxPlayers
//

class ConstrainMinMaxPlayers : public MenuConstraint {
public:
    ConstrainMinMaxPlayers(const string &k, int v, const MenuInt * mnp, const MenuInt * mxp)
        : MenuConstraint("", 0), key(k), value(v), min_players(mnp), max_players(mxp) { }
    virtual bool apply(MenuSelections &msel, int nplayers) const;
    virtual bool canSetTo(const MenuSelections &msel, const std::string &k, int v, int nplayers) const;
    int getMinPlayers(const MenuSelections &msel) const;
private:
    std::string key;
    const int value;
    const MenuInt * min_players;
    const MenuInt * max_players;
};

bool ConstrainMinMaxPlayers::apply(MenuSelections &msel, int nplayers) const
{
    const int min_p = min_players ? min_players->getValue(msel) : 0;
    const int max_p = max_players ? max_players->getValue(msel) : 999999;
    
    const bool forbidden = msel.getValue(key) == value && (nplayers > max_p || nplayers < min_p);
    if (forbidden) {
        msel.setValue(key, value - 1); // set it to something else
        return true;
    } else {
        return false;
    }
}

bool ConstrainMinMaxPlayers::canSetTo(const MenuSelections &msel, const string &k, int v, int nplayers) const
{
    const int min_p = min_players ? min_players->getValue(msel) : 0;
    const int max_p = max_players ? max_players->getValue(msel) : 999999;
    
    const bool forbidden = k == key && v == value && (nplayers < min_p || nplayers > max_p);
    return !forbidden;
}

int ConstrainMinMaxPlayers::getMinPlayers(const MenuSelections &msel) const
{
    if (key.empty()) return 0;
    if (msel.getValue(key) == value) {
        return min_players ? min_players->getValue(msel) : 0;
    } else {
        return 0;
    }
}

//
// Create function
//

boost::shared_ptr<MenuConstraint> CreateMenuConstraint(const std::string &key, int val,
                                                       const std::string &name, KnightsConfigImpl &kc)
{
    boost::shared_ptr<MenuConstraint> result;
    
    KConfig::KFile::List lst(*kc.getKFile(), "", 2);
    lst.push(0);
    std::string con_key = kc.getKFile()->popString();

    const MenuInt * con_mi = 0;
    std::vector<const MenuInt *> con_list;
    if (name != "ConstrainList") {
        lst.push(1);
        con_mi = kc.popMenuInt();
    } else {
        lst.push(1);
        KConfig::KFile::List lst2(*kc.getKFile(), "");
        const int sz = lst2.getSize();
        con_list.reserve(sz);
        for (int i = 0; i < sz; ++i) {
            lst2.push(i);
            con_list.push_back(kc.popMenuInt());
        }
    }
    
    if (name == "Constrain") {
        result.reset(new Constrain(key, val, con_key, con_mi));
    } else if (name == "ConstrainList") {
        result.reset(new ConstrainList(key, val, con_key, con_list));
    } else if (name == "ConstrainMin") {
        result.reset(new ConstrainMin(key, val, con_key, con_mi));
    } else if (name == "ConstrainMax") {
        result.reset(new ConstrainMax(key, val, con_key, con_mi));
    } else if (name == "ConstrainMaxPlayers") {
        // ConstrainMaxPlayers has been slightly hacked in to the existing framework, this is why
        // con_key is ignored for this constraint...
        result.reset(new ConstrainMinMaxPlayers(key, val, 0, con_mi));
    } else if (name == "ConstrainMinPlayers") {
        result.reset(new ConstrainMinMaxPlayers(key, val, con_mi, 0));
    } else {
        kc.getKFile()->errExpected("MenuDirective");
    }
    
    return result;
}


//
// implementation of MenuConstraints methods
//

void MenuConstraints::apply(const Menu &menu, MenuSelections &msel, int nplayers) const
{
    // First update the current values to a legal configuration.
    // (Limit of 100 iterations to prevent infinite loops.)

    // NOTE: "quest" constraints are done in a loop to begin with, then if there are outstanding
    // non-quest constraints, these are done separately afterwards. This is necessary because
    // some of the "quest" constraints violate the max-players constraints and we would therefore
    // get an infinite loop otherwise.

    bool doing_quest_constraints = true;
    
    for (int count = 0; count < 100; ++count) {
        bool changed = false;
        
        for (int i = 0; i < menu.getNumItems(); ++i) {
            const MenuItem &item = menu.getItem(i);
            const std::string &key = item.getKey();
            const int min_value = item.getMinValue();
            const int max_value = item.hasNumValues() ? min_value + item.getNumValues() - 1 : 99999999;

            const int cur_val = msel.getValue(key);

            if (cur_val < min_value || cur_val > max_value) {
                // set it back to its lower bound
                msel.setValue(key, min_value);
                changed = true;
            } else {
                // apply all constraints to this value.
                for (std::vector<boost::shared_ptr<MenuConstraint> >::const_iterator con = constraints.begin();
                con != constraints.end(); ++con) {
                    if (doing_quest_constraints != (*con)->isQuest()) continue; // do only quest, or only non-quest, as appropriate
                    if ((*con)->applicable(msel) && (*con)->apply(msel, nplayers)) changed = true;
                }
            }
        }

        if (!changed) {
            if (doing_quest_constraints) doing_quest_constraints = false;  // go to non-quest pass
            else break;  // all done.
        }
    }

    // Now set all 'allowed values'
    // Note: quest constraints are not considered on this pass, because we want them to be able to set a custom quest
    // without having to deselect current quest first.
    for (int i = 0; i < menu.getNumItems(); ++i) {
        const MenuItem &item = menu.getItem(i);
        const std::string &key = item.getKey();

        std::vector<int> allowed_values;
        
        if (item.hasNumValues()) {
            for (int x = 0; x < item.getNumValues(); ++x) {
                const int proposed_value = item.getMinValue() + x;
                bool can_set = true;
                for (std::vector<boost::shared_ptr<MenuConstraint> >::const_iterator con = constraints.begin();
                con != constraints.end(); ++con) {
                    if (!(*con)->isQuest()) {
                        if ((*con)->applicable(msel) && !(*con)->canSetTo(msel, key, proposed_value, nplayers)) {
                            can_set = false;
                            break;
                        }
                    }
                }
                if (can_set) allowed_values.push_back(proposed_value);
            }
        }
        
        msel.setAllowedValues(key, allowed_values);
    }
}

void MenuConstraints::addConstraintFromKFile(const std::string &key, int val, const std::string &dname, KnightsConfigImpl &kc)
{
    constraints.push_back(CreateMenuConstraint(key, val, dname, kc));
}

int MenuConstraints::getMinPlayers(const MenuSelections &msel) const
{
    int min_players = 0;
    for (std::vector<boost::shared_ptr<MenuConstraint> >::const_iterator con = constraints.begin();
    con != constraints.end(); ++con) {
        // TODO: cleaner way of doing this? (problem is basically that ConstrainMinMaxPlayers doesn't use the
        // standard "applicable()" method of deciding whether the constraint is active.)
        ConstrainMinMaxPlayers *c = dynamic_cast<ConstrainMinMaxPlayers*>(con->get());
        if (c) {
            min_players = std::max(min_players, c->getMinPlayers(msel));
        }
    }
    return min_players;
}

/*
 * menu_constraints.hpp
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
 * Implements constraints on menu settings.
 *
 */

#ifndef MENU_CONSTRAINTS_HPP
#define MENU_CONSTRAINTS_HPP

#include "boost/shared_ptr.hpp"

#include <string>
#include <vector>

class KnightsConfigImpl;
class Menu;
class MenuConstraint;  // implementation class
class MenuSelections;

class MenuConstraints {
public:
    // update menu selections to be consistent with constraints.
    // also update 'allowed settings'.
    void apply(const Menu &menu, MenuSelections &msel, int nplayers) const;

    // determine min number of players for current quest
    int getMinPlayers(const MenuSelections &msel) const;
    
    // construction
    void addConstraintFromKFile(const std::string &key, int val, const std::string &dname, KnightsConfigImpl &kc);

private:
    std::vector<boost::shared_ptr<MenuConstraint> > constraints;
};

#endif

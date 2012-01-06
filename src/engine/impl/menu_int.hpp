/*
 * menu_int.hpp
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

#ifndef MENU_INT_HPP
#define MENU_INT_HPP

#include "menu_selections.hpp"

class MenuInt {
public:
    virtual ~MenuInt() { }
    virtual int getValue(const MenuSelections &msel) const = 0;
};

class MenuIntConst : public MenuInt {
public:
    explicit MenuIntConst(int xx) : x(xx) { }
    virtual int getValue(const MenuSelections &) const { return x; }
private:
    int x;
};

class MenuIntVar : public MenuInt {
public:
    explicit MenuIntVar(const std::string &key_) : key(key_) { }
    virtual int getValue(const MenuSelections &msel) const { return msel.getValue(key); }
private:
    std::string key;
};

#endif

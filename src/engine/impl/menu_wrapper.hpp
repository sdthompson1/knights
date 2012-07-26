/*
 * menu_wrapper.hpp
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
 * This class interfaces to the Lua menu system.
 *
 */

#ifndef MENU_WRAPPER_HPP
#define MENU_WRAPPER_HPP

#include "boost/shared_ptr.hpp"

class MenuListener;
struct MenuWrapperImpl;

struct lua_State;

class MenuWrapper {
public:
    // pops a menu table from the top of the stack
    // If 'strict', all constraints will be checked. If non-strict,
    // min players & min teams constraints will be ignored.
    MenuWrapper(lua_State *lua, bool strict);

    bool getStrict() const;
    
    // get the underlying Menu object (stores "GUI" aspects of the menu)
    const Menu & getMenu() const;

    // get all the current menu selections (in the form of MenuListener calls)
    void getCurrentSettings(MenuListener &listener) const;
    
    // These are called by the client when certain events of interest occur.
    // They may change menu settings, or allowed values, in which case the listener
    // will be called once per menu-item that was changed.
    void changeSetting(int item_num, int new_choice_num, MenuListener &listener);
    void changeNumberOfPlayers(int nplayers, int nteams, MenuListener &listener);
    
private:
    boost::shared_ptr<MenuWrapperImpl> pimpl;
};

#endif

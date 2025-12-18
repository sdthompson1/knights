/*
 * user_control.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2025.
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

#ifndef USER_CONTROL_HPP
#define USER_CONTROL_HPP

#include "localization.hpp"
#include "map_support.hpp"

#include "network/byte_buf.hpp"  // coercri

#include <string>
#include <vector>

class Graphic;

class UserControl {
public:
    // reads from lua, doesn't pop. sets id to -1.
    UserControl(lua_State *lua, int idx);

    void newIndex(lua_State *lua);

    // cut down default ctor.
    explicit UserControl(bool cts) 
        : id(-1), menu_graphic(0), menu_direction(D_NORTH),
        tap_priority(0), action_bar_slot(0), action_bar_priority(0),
        continuous(cts), suicide_key(false),
        menu_special(0)
    { }

    void setID(int id_) { id = id_; } // called by KnightsConfigImpl
    int getID() const { return id; }

    // If menu_graphic == 0 then this control won't appear on the
    // menu, else it will appear, preferably in the slot given by
    // getMenuDirection.
    const Graphic * getMenuGraphic() const { return menu_graphic; }
    MapDirection getMenuDirection() const { return menu_direction; }

    // Action bar settings
    int getActionBarSlot() const { return action_bar_slot; }
    int getActionBarPriority() const { return action_bar_priority; }

    // Old (Amiga Knights) control settings
    int getTapPriority() const { return tap_priority; }
    bool getSuicideKey() const { return suicide_key; }
    
    // "Continuous" controls (e.g. lock picking) are executed for as
    // long as you hold the button down. "Noncontinuous" controls
    // (e.g. opening a door) execute only once.
    bool isContinuous() const { return continuous; }

    // Special properties
    // MS_WEAK = Can be overridden by another control on the menu (e.g. open door: can be overridden by lock picks)
    // MS_APPR_BASED = This control is only available if "approach based" controls are being used. (NOT CURRENTLY USED)
    // MS_NO_MENU = This control should not appear on the Action Menu (but it can appear on the Action Bar).
    enum MenuSpecial { MS_WEAK=1, MS_APPR_BASED = 2, MS_NO_MENU = 4 };
    unsigned int getMenuSpecial() const { return menu_special; }

    // Name. Displayed below the Action Bar.
    const LocalKey & getNameKey() const { return name; }

    // serialization
    explicit UserControl(int id_, Coercri::InputByteBuf &buf, const std::vector<const Graphic *> &graphics);
    void serialize(Coercri::OutputByteBuf &buf) const;
    
private:
    int id;
    const Graphic *menu_graphic;
    MapDirection menu_direction;
    int tap_priority;
    int action_bar_slot;
    int action_bar_priority;
    bool continuous;
    bool suicide_key;
    unsigned int menu_special;
    LocalKey name;
};

//
// Certain "standard" controls are given fixed ID numbers to help
// identify them.
//
// NOTE: These are the indices within the standard controls vector.
// The actual ID numbers are one plus this (because control IDs have
// to start from 1).
//

enum StandardControlIndex {
    SC_ATTACK = 0,
    SC_MOVE = 4,
    SC_WITHDRAW = 8,
    SC_ATTACK_NO_DIR = 9,
    NUM_STANDARD_CONTROLS = 10
};

#endif

/*
 * control.hpp
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

#ifndef CONTROL_HPP
#define CONTROL_HPP

#include "lua_ref.hpp"
#include "user_control.hpp"

#include <vector>
using namespace std;

class Action;

//
// 'Controls' represent something a knight can do.
// (eg move, activate, pick up item).
//

class Control : public UserControl {
public:
    Control(lua_State *lua, int idx,
            const Graphic *menu_gfx, MapDirection menu_dir,
            int tap_pri, int action_slot, int action_pri, bool suicide,
            bool cts, unsigned int special, const std::string &name,
            const Action *action_);

    void pushTable(lua_State *lua) const { table_ref.push(lua); }

    // 'cut down' constructor for the standard controls.
    Control(int id, bool cts, const Action *ac)
        : UserControl(0, D_NORTH, 0, 0, 0, false, cts, 0, ""),
          action(ac)
    { setID(id); }

    // action to run when the control is selected.
    const Action * getAction() const { return action; }

private:
    const Action *action;
    LuaRef table_ref;
};

#endif

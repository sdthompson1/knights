/*
 * control.cpp
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

#include "misc.hpp"

#include "control.hpp"

#include "lua.hpp"

Control::Control(lua_State *lua, int idx,
                 const Graphic *menu_gfx, MapDirection menu_dir,
                 int tap_pri, int action_slot, int action_pri, bool suicide,
                 bool cts, unsigned int special, const std::string &name,
                 const Action *action_)
  : UserControl(menu_gfx, menu_dir, tap_pri, action_slot, action_pri, suicide, cts, special, name),
    action(action_)
{
    if (lua) {
        lua_pushvalue(lua, idx);
        table_ref.reset(lua);
    }
}
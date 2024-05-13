/*
 * control.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2024.
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

#ifndef CONTROL_HPP
#define CONTROL_HPP

#include "lua_func.hpp"
#include "lua_ref.hpp"
#include "user_control.hpp"

#include <vector>


//
// 'Controls' represent something a knight can do.
// (eg move, activate, pick up item).
//

class Control : public UserControl {
public:
    Control(lua_State *lua, int idx);   // reads from lua; doesn't pop.

    // 'cut down' constructor for the standard controls.
    Control(bool cts, const LuaFunc &ac, const LuaFunc &poss, bool wm, bool ws)
        : UserControl(cts),
          execute(ac), possible(poss),
          can_execute_while_moving(wm),
          can_execute_while_stunned(ws)
    { }

    void newIndex(lua_State *lua);  // also calls UserControl::newIndex (if required)

    // get the underlying lua table (if there is one).
    void pushTable(lua_State *lua) const { table_ref.push(lua); }
    
    // accessor methods.
    const LuaFunc & getExecuteFunc() const { return execute; }
    const LuaFunc & getPossibleFunc() const { return possible; }
    bool canExecuteWhileMoving() const { return can_execute_while_moving; }
    bool canExecuteWhileStunned() const { return can_execute_while_stunned; }

    bool checkPossible(const ActionData &ad) const;

private:
    LuaFunc execute;
    LuaFunc possible;
    bool can_execute_while_moving;
    bool can_execute_while_stunned;
    LuaRef table_ref;
};

#endif

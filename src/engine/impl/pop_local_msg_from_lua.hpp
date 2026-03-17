/*
 * pop_local_msg_from_lua.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2026.
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

#ifndef POP_LOCAL_MSG_FROM_LUA_HPP
#define POP_LOCAL_MSG_FROM_LUA_HPP

#include "localization.hpp"

struct lua_State;

// Pop a LocalMsg from the top of the Lua stack.
// If top of stack is a string, it represents a plain LocalKey.
// If it is a table, it has fields "key", "params" and "plural".
//  - Params are strings (which represent LocalKeys), integers, or Player userdata objects.
//  - Plural is optional - if nil it defaults to -1.
// Any error with the format throws a lua error.
LocalMsg PopLocalMsgFromLua(lua_State *lua);

#endif

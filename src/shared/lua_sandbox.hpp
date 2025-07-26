/*
 * lua_sandbox.hpp
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

#ifndef LUA_SANDBOX_HPP
#define LUA_SANDBOX_HPP

#include "boost/shared_ptr.hpp"

struct lua_State;

// Makes a new lua_State which is a "sandbox" i.e. only
// trusted functions are allowed.

// lua_close will automatically be called when the shared_ptr is
// released.

// lua_panic handler will be installed, it will throw a LuaPanic
// exception when called. If this happens the lua_State should be
// considered unusable and it should be closed asap.

boost::shared_ptr<lua_State> MakeLuaSandbox();

#endif


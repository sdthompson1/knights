/*
 * lua_table_base.hpp
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

#ifndef LUA_TABLE_BASE_HPP
#define LUA_TABLE_BASE_HPP

struct lua_State;

// Base class for Knights objects that have an underlying Lua table,
// that can be accessed via __index or __newindex metamethods in
// Knights code.

// Stores a reference to the lua_State, so should be destroyed BEFORE
// destroying the lua_State.

class LuaTableBase {
public:
    // Reads a lua table from the given index (doesn't pop it).
    // If lua_ == 0, doesn't read anything from Lua.
    LuaTableBase(lua_State *lua_, int idx);
    LuaTableBase(const LuaTableBase &);
    ~LuaTableBase();

    // Push the stored table onto the lua stack.
    void pushTable(lua_State *L) const;

private:
    void operator=(const LuaTableBase &);  // assignment not implemented

    lua_State *lua;
    int table_ref;
};

#endif

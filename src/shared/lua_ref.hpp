/*
 * lua_ref.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2013.
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

#ifndef LUA_REF_HPP
#define LUA_REF_HPP

struct lua_State;

class LuaRef {
public:
    LuaRef();                          // Makes an empty ref
    explicit LuaRef(lua_State *lua);   // Pops lua stack, and stores a reference

    LuaRef(const LuaRef &other);
    LuaRef & operator=(const LuaRef &other);
    ~LuaRef();

    // WARNING: getLuaState() may return null (if the ref was unset)
    lua_State * getLuaState() const { return lua; }
    void push(lua_State *L) const;

    void reset(lua_State *L);    // Pops lua stack, releases old ref and creates a new one.
    
    // hasValue() returns true if lua != 0 and stored value != nil
    bool hasValue() const;

private:
    lua_State *lua;
    int ref;
};

#endif

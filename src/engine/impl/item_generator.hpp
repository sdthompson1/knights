/*
 * item_generator.hpp
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

#ifndef ITEM_GENERATOR_HPP
#define ITEM_GENERATOR_HPP

#include "lua_ref.hpp"

#include <vector>
using namespace std;

class ItemType;

//
// ItemGenerator represents a Lua procedure which, when called, will
// return an itemtype and (optionally) a number of items to generate.
//
class ItemGenerator {
public:
    // Ctor pops a callable object from top of stack.
    explicit ItemGenerator(lua_State *lua);
    ItemGenerator() { }
    
    std::pair<ItemType *, int> get() const;

private:
    LuaRef item_gen_func;
};

#endif

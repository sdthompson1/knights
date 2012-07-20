/*
 * monster_type.hpp
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
 * Represents a "type" of monster.
 * Responsible for creating new monsters of that type, and for starting off monster AI.
 * 
 */

#ifndef MONSTER_TYPE_HPP
#define MONSTER_TYPE_HPP

#include "lua_ref.hpp"
#include "map_support.hpp"

class Monster;
class MonsterManager;
class TaskManager;

#include "boost/shared_ptr.hpp"
using namespace boost;

class MonsterType {
public:
    // reads table from top of stack (does not pop it)
    explicit MonsterType(lua_State *lua);
    virtual ~MonsterType() { }

    void pushTable(lua_State *lua) const { table_ref.push(lua); }
    
    // makeMonster should both return a new monster object, and start off an AI task.
    virtual shared_ptr<Monster> makeMonster(TaskManager &tm) const = 0;

    // at which height will this monster be generated?
    virtual MapHeight getHeight() const = 0;

private:
    LuaRef table_ref;
};


#endif

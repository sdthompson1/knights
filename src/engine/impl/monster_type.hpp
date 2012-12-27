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

#include "lua_func.hpp"
#include "lua_ref.hpp"
#include "map_support.hpp"

class Action;
class KnightsConfigImpl;
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
    virtual void newIndex(lua_State *lua); // handles keys common to all montypes
    
    // makeMonster should both return a new monster object, and start off an AI task.
    virtual shared_ptr<Monster> makeMonster(TaskManager &tm) const = 0;

    // at which height will this monster be generated?
    virtual MapHeight getHeight() const = 0;

    // This checks whether this particular monster type has any reason why it could not be created
    // at the given location.
    // At the time of writing (Sep 2012), for flying monsters this always returns true, but for
    // walking monsters it checks the "ai_avoid" list to stop walking monsters being created on such tiles (usu. pits).
    virtual bool okToCreateAt(const DungeonMap &dmap, const MapCoord &pos) const;

    const LuaFunc & getSoundAction() const { return sound_action; }
    const LuaFunc & getOnAttack() const { return on_attack; }
    const LuaFunc & getOnDamage() const { return on_damage; }
    const LuaFunc & getOnDeath() const { return on_death; }
    
private:
    LuaRef table_ref;
    LuaFunc sound_action;
    LuaFunc on_attack, on_damage, on_death;
};


#endif

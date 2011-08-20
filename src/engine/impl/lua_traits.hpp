/*
 * lua_traits.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
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

#ifndef LUA_TRAITS_HPP
#define LUA_TRAITS_HPP

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

class Control;
class Creature;
class Graphic;
class Sound;
class ItemType;
class Player;
class Tile;

// Define LuaTraits template which contains:

// tag -- lua tag value for this type

template<class T>
struct LuaTraits { };

template<>
struct LuaTraits<Graphic> {
    enum { tag = 1 };
};

template<>
struct LuaTraits<Sound> {
    enum { tag = 2 };
};

template<>
struct LuaTraits<ItemType> {
    enum { tag = 3 };
};

template<>
struct LuaTraits<Tile> {
    enum { tag = 4 };
};

template<>
struct LuaTraits<Player> {
    enum { tag = 5 };
};

// TODO: If we want Entities in Lua as well, then need to figure out how to deal with inheritance: Creature -> Entity.
template<>
struct LuaTraits<Creature> {
    enum { tag = 6 };
};

template<>
struct LuaTraits<Control> {
    enum { tag = 7 };
};

#endif

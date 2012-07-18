/*
 * lua_traits.hpp
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

#ifndef LUA_TRAITS_HPP
#define LUA_TRAITS_HPP

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

class Anim;
class Control;
class Creature;
class Graphic;
class Sound;
class ItemType;
class Overlay;
class Player;
class Tile;

// Define LuaTraits template which contains:

// tag -- lua tag value for this type

// SEE ALSO: lua_userdata.cpp which maps tags to "table getting functions"
// (and equality).

enum {
    TAG_GRAPHIC = 1,
    TAG_SOUND,
    TAG_ITEM_TYPE,
    TAG_TILE,
    TAG_PLAYER,
    TAG_CREATURE,
    TAG_CONTROL
};

template<class T>
struct LuaTraits { };

template<>
struct LuaTraits<Graphic> {
    enum { tag = TAG_GRAPHIC };
};

template<>
struct LuaTraits<Sound> {
    enum { tag = TAG_SOUND };
};

template<>
struct LuaTraits<const ItemType> {
    enum { tag = TAG_ITEM_TYPE };
};

template<>
struct LuaTraits<Tile> {
    enum { tag = TAG_TILE };
};

template<>
struct LuaTraits<Player> {
    enum { tag = TAG_PLAYER };
};

// TODO: If we want Entities in Lua as well, then need to figure out how to deal with inheritance: Creature -> Entity.
template<>
struct LuaTraits<Creature> {
    enum { tag = TAG_CREATURE };
};

template<>
struct LuaTraits<Control> {
    enum { tag = TAG_CONTROL };
};

template<>
struct LuaTraits<Overlay> {
    enum { tag = 8 };
};

template<>
struct LuaTraits<Anim> {
    enum { tag = 9 };
};

#endif

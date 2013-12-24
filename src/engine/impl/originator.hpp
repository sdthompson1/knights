/*
 * originator.hpp
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

#ifndef ORIGINATOR_HPP
#define ORIGINATOR_HPP

// This class defines who is the source of an attack (or other action).
// This is used to attribute kills.
//
// Note suicides may be represented either as "none" or as "player"
// (where getPlayer() is equal to the person being killed);
// we are a bit inconsistent about this currently.

class Player;

// Originator types
struct OT_Monster { };
struct OT_None { };
struct OT_Player { };    // takes one argument, which is the Player pointer.

class Originator {
public:

    // All ctors are explicit
    explicit Originator(OT_Monster) : player(0), is_monster(true) { }
    explicit Originator(OT_None) : player(0), is_monster(false) { }
    explicit Originator(OT_Player, Player *p)  // if p == null it converts into OT_None.
        : player(p), is_monster(false) { }

    bool isMonster() const { return is_monster; }
    bool isNone() const { return !is_monster && player == 0; }
    bool isPlayer() const { return !is_monster && player != 0; }
   
    Player * getPlayer() const { return is_monster ? 0 : player; }

private:
    // is_monster true ==> monster
    // is_monster false, player null ==> none
    // is_monster false, player non-null ==> player
    Player *player;
    bool is_monster;
};

#endif

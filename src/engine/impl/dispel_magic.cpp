/*
 * dispel_magic.cpp
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

#include "misc.hpp"

#include "dispel_magic.hpp"
#include "knight.hpp"
#include "player.hpp"

void DispelMagic(const vector<Player*> &players)
{
    for (vector<Player*>::const_iterator it = players.begin(); it != players.end(); ++it) {
        shared_ptr<Knight> kt = (*it)->getKnight();
        if (kt) {
            kt->dispelMagic();
        }
    }
}

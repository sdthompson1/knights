/*
 * dispel_magic.hpp
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

#ifndef DISPEL_MAGIC_HPP
#define DISPEL_MAGIC_HPP

#include "boost/shared_ptr.hpp"
using namespace boost;

#include <vector>

class Knight;
class Player;

// This routine simply calls dispelMagic() on each Knight.
void DispelMagic(const std::vector<Player*> &);

// DispelObserver: class that gets notified when a dispel magic occurs
// (These are attached to Knights.)
class DispelObserver {
public:
    virtual ~DispelObserver() { }
    virtual void onDispel(shared_ptr<Knight> kt) = 0;
};

#endif

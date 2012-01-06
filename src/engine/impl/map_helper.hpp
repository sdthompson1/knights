/*
 * map_helper.hpp
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

#ifndef MAP_HELPER_HPP
#define MAP_HELPER_HPP

// MapHelper: this class is used by Entities to help add themselves
// to, or remove themselves from, a dungeonmap.

class MapHelper {
    friend class Entity;
private:
    static void addEntity(shared_ptr<Entity> ag);
    static void rmEntity(shared_ptr<Entity> ag);
};

#endif

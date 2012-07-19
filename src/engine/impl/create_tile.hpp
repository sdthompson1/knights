/*
 * create_tile.hpp
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

#ifndef CREATE_TILE_HPP
#define CREATE_TILE_HPP

#include "boost/shared_ptr.hpp"

struct lua_State;

class KnightsConfigImpl;
class Tile;

// Factory function for Tiles. Creates appropriate tile type (Tile,
// Door, Home, etc) based on the "type" field of the lua table.

// Reads table from top of lua stack; does not pop it.

boost::shared_ptr<Tile> CreateTile(lua_State *lua, KnightsConfigImpl *kc);

#endif

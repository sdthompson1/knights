/*
 * dungeon_layout.hpp
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

/*
 * A DungeonLayout gives the layout for a Knights-style dungeon.
 *
 */

#ifndef DUNGEON_LAYOUT_HPP
#define DUNGEON_LAYOUT_HPP

#include <memory>
#include <string>
#include <vector>

struct lua_State;

enum BlockType {
    BT_NONE, BT_BLOCK, BT_EDGE, BT_SPECIAL, NUM_BLOCK_TYPES
};

class DungeonLayout {
public:
    // Pops a Lua table containing "width", "height", "data" and
    // constructs DungeonLayout from it. Throws LuaError if there is a
    // problem (w/ stack unchanged).
    explicit DungeonLayout(lua_State *lua);

    // accessors
    // for vert exits,  x ranges from 0 to w-1, y ranges from 0 to h-2.
    // for horiz exits, x ranges from 0 to w-2, y ranges from 0 to h-1.
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    BlockType getBlockType(int x, int y) const;
    bool hasHorizExit(int x, int y) const;
    bool hasVertExit(int x, int y) const;

private:
    int width, height;
    std::vector<BlockType> data;
    std::vector<bool> horiz_exits;
    std::vector<bool> vert_exits;
};

#endif

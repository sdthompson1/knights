/*
 * dungeon_layout.cpp
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

#include "misc.hpp"

#include "dungeon_layout.hpp"
#include "rng.hpp"

DungeonLayout::DungeonLayout(int w, int h)
    : width(w), height(h), data(w*h), horiz_exits((w-1)*h), vert_exits(w*(h-1))
{ }

void DungeonLayout::setBlockType(int x, int y, BlockType bt)
{
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    data[y*width+x] = bt;
}

void DungeonLayout::setVertExit(int x, int y, bool have_exit)
{
    if (x < 0 || x >= width || y < 0 || y >= (height-1)) return;
    vert_exits[y*width+x] = have_exit;
}

void DungeonLayout::setHorizExit(int x, int y, bool have_exit)
{
    if (x < 0 || x >= (width-1) || y < 0 || y >= height) return;
    horiz_exits[y*(width-1)+x] = have_exit;
}

BlockType DungeonLayout::getBlockType(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= height) return BT_BLOCK;
    return data[y*width+x];
}

bool DungeonLayout::hasVertExit(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= (height-1)) return false;
    return vert_exits[y*width + x];
}

bool DungeonLayout::hasHorizExit(int x, int y) const
{
    if (x < 0 || x >= (width-1) || y < 0 || y >= height) return false;
    return horiz_exits[y*(width-1) + x];
}


const DungeonLayout * RandomDungeonLayout::choose() const
{
    int i = g_rng.getInt(0, data.size());
    return data[i];
}

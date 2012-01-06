/*
 * colour_change.cpp
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

#include "colour_change.hpp"
#include "protocol.hpp"

#include <algorithm>
using namespace std;

void ColourChange::add(Colour old_col, Colour new_col)
{
    // NB would be faster to do some kind of incremental sort (eg an insertion sort),
    // but it's easier just to append the new value and then re-sort the whole lot :)
    mappings.push_back(make_pair(old_col, new_col));
    stable_sort(mappings.begin(), mappings.end());
}

struct CmpFirst {
    bool operator()(const pair<Colour,Colour> &lhs,
                    const pair<Colour,Colour> &rhs) const
    {
        return lhs.first < rhs.first;
    }
};

bool ColourChange::lookup(Colour old_col, Colour &new_col) const
{
    pair<Colour, Colour> x;
    x.first = old_col;

    CmpFirst cmp;
    vector<pair<Colour,Colour> >::const_iterator it
        = lower_bound(mappings.begin(), mappings.end(), x, cmp);
    if (it == mappings.end() || cmp(x, *it)) {
        return false;
    } else {
        new_col = it->second;
        return true;
    }
}


Colour::Colour(Coercri::InputByteBuf &buf)
{
    r = buf.readUbyte();
    g = buf.readUbyte();
    b = buf.readUbyte();
    a = buf.readUbyte();
}

void Colour::serialize(Coercri::OutputByteBuf &buf) const
{
    buf.writeUbyte(r);
    buf.writeUbyte(g);
    buf.writeUbyte(b);
    buf.writeUbyte(a);
}

ColourChange::ColourChange(Coercri::InputByteBuf &buf)
{
    const int num = buf.readVarInt();
    mappings.reserve(num);
    for (int i = 0; i < num; ++i) {
        Colour c1(buf);
        Colour c2(buf);
        mappings.push_back(std::make_pair(c1, c2));
    }
}

void ColourChange::serialize(Coercri::OutputByteBuf &buf) const
{
    buf.writeVarInt(mappings.size());
    for (vector<pair<Colour,Colour> >::const_iterator it = mappings.begin(); it != mappings.end(); ++it) {
        it->first.serialize(buf);
        it->second.serialize(buf);
    }
}

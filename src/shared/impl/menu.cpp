/*
 * menu.cpp
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

#include "misc.hpp"

#include "menu.hpp"

Menu::Menu(Coercri::InputByteBuf &buf)
{
    title = LocalKey(buf.readString());
    const int n_items = buf.readVarIntThrow(0, 10000);
    items.reserve(n_items);
    for (int i = 0; i < n_items; ++i) {
        items.push_back(MenuItem(buf));
    }
}

void Menu::serialize(Coercri::OutputByteBuf &buf) const
{
    buf.writeString(title.getKey());
    buf.writeVarInt(items.size());
    for (std::vector<MenuItem>::const_iterator it = items.begin(); it != items.end(); ++it) {
        it->serialize(buf);
    }
}

        

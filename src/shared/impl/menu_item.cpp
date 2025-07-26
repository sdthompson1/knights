/*
 * menu_item.cpp
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

#include "menu_item.hpp"

MenuItem::MenuItem(Coercri::InputByteBuf &buf)
{
    title = buf.readString();
    numeric = buf.readUbyte() != 0;

    if (numeric) {
        num_digits = buf.readVarInt();
        suffix = buf.readString();
    } else {
        const int nsettings = buf.readVarInt();
        value_str.reserve(nsettings);
        for (int i = 0; i < nsettings; ++i) {
            value_str.push_back(buf.readString());
        }
    }

    space_after = buf.readUbyte() != 0;
}

void MenuItem::serialize(Coercri::OutputByteBuf &buf) const
{
    buf.writeString(title);
    buf.writeUbyte(numeric ? 1 : 0);

    if (numeric) {
        buf.writeVarInt(num_digits);
        buf.writeString(suffix);
    } else {
        buf.writeVarInt(value_str.size());
        for (std::vector<std::string>::const_iterator it = value_str.begin(); it != value_str.end(); ++it) {
            buf.writeString(*it);
        }
    }
    
    buf.writeUbyte(space_after ? 1 : 0);
}

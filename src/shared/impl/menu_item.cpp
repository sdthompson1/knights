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
    title = LocalKey(buf.readString());
    numeric = buf.readUbyte() != 0;

    if (numeric) {
        num_digits = buf.readVarIntClamp(0, 50);
        suffix = LocalKey(buf.readString());
    } else {
        const int nsettings = buf.readVarIntThrow(0, 1000);
        dropdown_entries.reserve(nsettings);

        for (int i = 0; i < nsettings; ++i) {
            LocalKeyOrInteger lki;
            int flag = buf.readUbyte();
            if (flag != 0) {
                lki.is_integer = true;
                lki.integer = buf.readVarInt();
            } else {
                lki.is_integer = false;
                lki.local_key = LocalKey(buf.readString());
            }
            dropdown_entries.push_back(lki);
        }
    }

    space_after = buf.readUbyte() != 0;
}

void MenuItem::serialize(Coercri::OutputByteBuf &buf) const
{
    buf.writeString(title.getKey());
    buf.writeUbyte(numeric ? 1 : 0);

    if (numeric) {
        buf.writeVarInt(num_digits);
        buf.writeString(suffix.getKey());
    } else {
        buf.writeVarInt(dropdown_entries.size());
        for (auto const& lki : dropdown_entries) {
            if (lki.is_integer) {
                buf.writeUbyte(1);
                buf.writeVarInt(lki.integer);
            } else {
                buf.writeUbyte(0);
                buf.writeString(lki.local_key.getKey());
            }
        }
    }
    
    buf.writeUbyte(space_after ? 1 : 0);
}

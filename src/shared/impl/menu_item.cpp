/*
 * menu_item.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
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

#include "menu_item.hpp"
#include "protocol.hpp"

#include <sstream>

MenuItem::MenuItem(Coercri::InputByteBuf &buf)
{
    key = buf.readString();
    min_value = buf.readVarInt();
    title_str = buf.readString();
    const int n_vals = buf.readVarInt();
    value_str.resize(n_vals);
    for (int i = 0; i < n_vals; ++i) {
        value_str[i] = buf.readString();
    }
    space_after = buf.readUbyte() != 0;
}

void MenuItem::serialize(Coercri::OutputByteBuf &buf) const
{
    buf.writeString(key);
    buf.writeVarInt(min_value);
    buf.writeString(title_str);
    buf.writeVarInt(value_str.size());
    for (std::vector<std::string>::const_iterator it = value_str.begin(); it != value_str.end(); ++it) {
        buf.writeString(*it);
    }
    buf.writeUbyte(space_after ? 1 : 0);
}

int MenuItem::getMinValue() const
{
    return min_value;
}

bool MenuItem::hasNumValues() const
{
    return (key != "#time");
}

int MenuItem::getNumValues() const
{
    if (!hasNumValues()) throw UnexpectedError("MenuItem::getNumValues error");
    else return int(value_str.size());
}

std::string MenuItem::getValueString(int i) const
{
    const int val = i - min_value;
    if (key == "#time") {
        if (val == 0) {
            return "None";
        } else {
            std::ostringstream str;
            str << i << " min";
            if (i!=1) str << 's';
            return str.str();
        }
    } else {
        return value_str.at(val);
    }
}

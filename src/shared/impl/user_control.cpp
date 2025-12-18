/*
 * user_control.cpp
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

#include "graphic.hpp"
#include "user_control.hpp"

#include <stdexcept>

UserControl::UserControl(int id_, Coercri::InputByteBuf &buf, const std::vector<const Graphic *> &graphics)
{
    id = id_;
    const int gfx_id = buf.readVarInt();
    menu_graphic = gfx_id == 0 ? 0 : graphics.at(gfx_id - 1);
    const int d = buf.readUbyte();
    if (d > 3) throw std::runtime_error("error reading UserControl");
    menu_direction = MapDirection(d);
    tap_priority = buf.readVarInt();
    action_bar_slot = buf.readVarInt();
    action_bar_priority = buf.readVarInt();
    suicide_key = buf.readUbyte() != 0;
    menu_special = buf.readUbyte();
    continuous = buf.readUbyte() != 0;
    name = LocalKey(buf.readString());
}

void UserControl::serialize(Coercri::OutputByteBuf &buf) const
{
    buf.writeVarInt(menu_graphic ? menu_graphic->getID() : 0);
    buf.writeUbyte(menu_direction);
    buf.writeVarInt(tap_priority);
    buf.writeVarInt(action_bar_slot);
    buf.writeVarInt(action_bar_priority);
    buf.writeUbyte(suicide_key);
    buf.writeUbyte(menu_special);
    buf.writeUbyte(continuous);
    buf.writeString(name.getKey());
}

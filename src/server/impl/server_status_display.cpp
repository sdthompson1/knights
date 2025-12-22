/*
 * server_status_display.cpp
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
#include "protocol.hpp"
#include "read_write_loc.hpp"
#include "server_status_display.hpp"

void ServerStatusDisplay::setBackpack(int slot, const Graphic *gfx, const Graphic *overdraw, int no_carried, int no_max)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_SET_BACKPACK);
    buf.writeUbyte(slot);
    buf.writeVarInt(gfx ? gfx->getID() : 0);
    buf.writeVarInt(overdraw ? overdraw->getID() : 0);
    buf.writeUbyte(no_carried);
    buf.writeUbyte(no_max);
}

void ServerStatusDisplay::addSkull()
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_ADD_SKULL);
}

void ServerStatusDisplay::setHealth(int h)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_SET_HEALTH);
    buf.writeVarInt(h);
}

void ServerStatusDisplay::setPotionMagic(PotionMagic pm, bool poison_immunity)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_SET_POTION_MAGIC);
    buf.writeUbyte((poison_immunity?128:0) + int(pm));
}

void ServerStatusDisplay::setQuestHints(const std::vector<LocalMsg> &hints)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_EXTENDED_MESSAGE);
    buf.writeVarInt(SERVER_EXT_SET_QUEST_HINTS);

    size_t scratch;
    buf.writePayloadSize(scratch);
    
    buf.writeUbyte(hints.size());
    for (int i = 0; i < int(hints.size()); ++i) {
        WriteLocalMsg(buf, hints[i]);
    }

    buf.backpatchPayloadSize(scratch);
}

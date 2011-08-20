/*
 * server_status_display.cpp
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

#include "graphic.hpp"
#include "protocol.hpp"
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

void ServerStatusDisplay::setQuestMessage(const std::string &msg)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_SET_QUEST_MESSAGE);
    buf.writeString(msg);
}

void ServerStatusDisplay::setQuestIcons(const std::vector<StatusDisplay::QuestIconInfo> &icons)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_SET_QUEST_ICONS);
    buf.writeUbyte(icons.size());
    for (int i = 0; i < int(icons.size()); ++i) {
        buf.writeUbyte(icons[i].num_held);
        buf.writeUbyte(icons[i].num_required);
        buf.writeVarInt(icons[i].gfx_missing ? icons[i].gfx_missing->getID() : 0);
        buf.writeVarInt(icons[i].gfx_held ? icons[i].gfx_held->getID() : 0);
    }
}

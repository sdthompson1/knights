/*
 * my_menu_listeners.cpp
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

#include "my_menu_listeners.hpp"
#include "protocol.hpp"
#include "read_write_loc.hpp"

#include <ostream>


// --------- MyMenuListener ---------------------------

void MyMenuListener::settingChanged(int item_num, const char *,
                                    int choice_num, const char *,
                                    const std::vector<int> &allowed_choices)
{
    changed = true;
    for (std::vector<Coercri::OutputByteBuf>::iterator bit = bufs.begin(); bit != bufs.end(); ++bit) {
        Coercri::OutputByteBuf &buf = *bit;
        buf.writeUbyte(SERVER_SET_MENU_SELECTION);
        buf.writeVarInt(item_num);
        buf.writeVarInt(choice_num);
        buf.writeVarInt(allowed_choices.size());
        for (std::vector<int>::const_iterator it = allowed_choices.begin(); it != allowed_choices.end(); ++it) {
            buf.writeVarInt(*it);
        }
    }
}

void MyMenuListener::questDescriptionChanged(const std::vector<Paragraph> &paragraphs)
{
    for (std::vector<Coercri::OutputByteBuf>::iterator bit = bufs.begin(); bit != bufs.end(); ++bit) {
        Coercri::OutputByteBuf &buf = *bit;
        buf.writeUbyte(SERVER_SET_QUEST_DESCRIPTION);
        if (paragraphs.size() > 255) {
            throw ProtocolError(LocalKey("message_too_long"));
        }
        buf.writeUbyte(paragraphs.size());
        for (const Paragraph &p : paragraphs) {
            WriteLocalKeyAndParams(buf, p.key, p.plural, p.params);
        }
    }
}


// --------- LogMenuListener --------------------------

void LogMenuListener::settingChanged(int item_num, const char *item_key,
                                     int choice_num, const char *choice_string,
                                     const std::vector<int> &allowed_choices)
{
    str << ", " << item_key << "=" << choice_string;
}


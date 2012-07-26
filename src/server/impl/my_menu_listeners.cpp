/*
 * my_menu_listeners.cpp
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

#include "my_menu_listeners.hpp"
#include "protocol.hpp"

#include <ostream>


// --------- MyMenuListener ---------------------------

void MyMenuListener::settingChanged(int item_num, const char *,
                                    int choice_num, const char *,
                                    const std::vector<int> &allowed_choices)
{
    changed = true;
    for (std::vector<std::pair<Coercri::OutputByteBuf, bool> >::iterator bit = bufs.begin();
         bit != bufs.end(); ++bit) {
        
        if (bit->second && item_num == original_item && choice_num == original_choice) {
            // this client already knows about this setting. skip it.
            // (this is important to avoid 'lag' when editing numeric fields.)
            continue;
        }
        
        Coercri::OutputByteBuf &buf = bit->first;
        buf.writeUbyte(SERVER_SET_MENU_SELECTION);
        buf.writeVarInt(item_num);
        buf.writeVarInt(choice_num);
        buf.writeVarInt(allowed_choices.size());
        for (std::vector<int>::const_iterator it = allowed_choices.begin(); it != allowed_choices.end(); ++it) {
            buf.writeVarInt(*it);
        }
    }
}

void MyMenuListener::questDescriptionChanged(const std::string &s)
{
    for (std::vector<std::pair<Coercri::OutputByteBuf, bool> >::iterator bit = bufs.begin();
         bit != bufs.end(); ++bit) {
        Coercri::OutputByteBuf &buf = bit->first;
        buf.writeUbyte(SERVER_SET_QUEST_DESCRIPTION);
        buf.writeString(s);
    }
}


// --------- LogMenuListener --------------------------

void LogMenuListener::settingChanged(int item_num, const char *item_key,
                                     int choice_num, const char *choice_string,
                                     const std::vector<int> &allowed_choices)
{
    str << ", " << item_key << "=" << choice_string << ", ";
}


// --------- RandomQuestMenuListener ------------------

void RandomQuestMenuListener::settingChanged(int item_num, const char *item_key,
                                             int choice_num, const char *choice_string,
                                             const std::vector<int> &allowed_choices)
{
    str << item_num << '\0';
    str << choice_num << '\0';
}

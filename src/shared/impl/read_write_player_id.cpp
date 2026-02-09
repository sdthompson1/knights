/*
 * read_write_player_id.cpp
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

#include "player_id.hpp"
#include "read_write_player_id.hpp"

#include "network/byte_buf.hpp"

void WritePlayerID(Coercri::OutputByteBuf &buf, const PlayerID &id, bool write_untrusted_strings)
{
    buf.writeString(id.getPlatform());
    buf.writeString(id.getPlatformUserId());
    if (write_untrusted_strings) {
        buf.writeString(id.getUserName().asUTF8());
    } else {
        buf.writeString("");
    }
}

PlayerID ReadPlayerID(Coercri::InputByteBuf &buf, bool allow_untrusted_strings)
{
    std::string platform = buf.readString();
    std::string platform_user_id = buf.readString();
    std::string user_name_raw = buf.readString();

    if (!allow_untrusted_strings) {
        // Untrusted strings NOT allowed - so we cannot read the username
        // (an untrusted host might try to spoof usernames or insert offensive words etc.)
        user_name_raw.clear();
    }

    Coercri::UTF8String user_name = Coercri::UTF8String::fromUTF8Safe(user_name_raw);

    if (platform.empty()) {
        if (user_name.empty()) {
            // An empty player ID is valid
            return PlayerID();
        } else {
            return PlayerID(user_name);
        }
    } else {
        return PlayerID(platform, platform_user_id, user_name);
    }
}

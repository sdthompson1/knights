/*
 * read_write_loc.cpp
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

#include "localization.hpp"
#include "protocol.hpp"
#include "read_write_loc.hpp"

#include "network/byte_buf.hpp"

void WriteLocalMsg(Coercri::OutputByteBuf &buf, const LocalMsg &msg)
{
    buf.writeString(msg.key.getKey());
    buf.writeVarInt(msg.count);
    buf.writeUbyte(msg.params.size());
    for (const auto & param : msg.params) {
        switch (param.getType()) {
        case LocalParam::Type::LOCAL_KEY:
            buf.writeUbyte(0);
            buf.writeString(param.getLocalKey().getKey());
            break;
        case LocalParam::Type::PLAYER_ID:
            buf.writeUbyte(1);
            buf.writeString(param.getPlayerID().asString());
            break;
        case LocalParam::Type::INTEGER:
            buf.writeUbyte(2);
            buf.writeVarInt(param.getInteger());
            break;
        case LocalParam::Type::STRING:
            // note: client will only accept this in "safe" modes i.e. not VM mode
            buf.writeUbyte(3);
            buf.writeString(param.getString().asUTF8());
            break;
        default:
            throw std::logic_error("Incorrect LocalParam type");
        }
    }
}

void ReadLocalMsg(Coercri::InputByteBuf &buf, LocalMsg &msg, bool allow_untrusted_strings)
{
    msg.key = LocalKey(buf.readString());
    msg.count = buf.readVarInt();
    int num_params = buf.readUbyte();
    msg.params.clear();
    msg.params.reserve(num_params);
    for (int i = 0; i < num_params; ++i) {
        switch (buf.readUbyte()) {
        case 0:
            msg.params.push_back(LocalParam(LocalKey(buf.readString())));
            break;
        case 1:
            msg.params.push_back(LocalParam(PlayerID(buf.readString())));
            break;
        case 2:
            msg.params.push_back(LocalParam(buf.readVarInt()));
            break;
        case 3:
            {
                std::string str = buf.readString();
                if (!allow_untrusted_strings) {
                    // Untrusted strings are NOT allowed => replace it with "#####"
                    str = "#####";
                }
                msg.params.push_back(LocalParam(Coercri::UTF8String::fromUTF8Safe(str)));
            }
            break;
        default:
            throw ProtocolError(LocalKey("bad_server_message"));
            break;
        }
    }
}

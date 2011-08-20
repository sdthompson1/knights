/*
 * replay_file.cpp
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

#include "replay_file.hpp"

#include <vector>

ReplayFile::ReplayFile(const std::string &filename)
    : str(filename.c_str(), std::ios::in | std::ios::binary)
{ }

void ReplayFile::readMessage(std::string &msg,
                             int &int_arg,
                             std::string &extra_bytes)
{
    // Read the #KTS# header
    const char * header = "#KTS#";
    const int hdr_len = 5;

    int hdr_pos = 0;
    while (str) {
        char c = 0;
        str.get(c);
        if (c == header[hdr_pos]) {
            hdr_pos++;
            if (hdr_pos == hdr_len) break;
        } else {
            hdr_pos = 0;
        }
    }
    if (!str) {
        msg = "EOF";
        return;
    }

    // Read the msg code
    char msg_code[3];
    str.read(&msg_code[0], 3);
    msg.assign(&msg_code[0], 3);

    // Read the timestamps. Note these are ignored currently
    time_t seconds;
    unsigned int msec;
    str.read(reinterpret_cast<char*>(&seconds), sizeof(seconds));
    str.read(reinterpret_cast<char*>(&msec), sizeof(msec));

    // Read the message data
    str.read(reinterpret_cast<char*>(&int_arg), sizeof(int));
    int num_extra_bytes = 0;
    str.read(reinterpret_cast<char*>(&num_extra_bytes), sizeof(int));
    if (num_extra_bytes > 0) {
        std::vector<char> buf(num_extra_bytes);
        str.read(&buf[0], num_extra_bytes);
        extra_bytes.assign(&buf[0], num_extra_bytes);
    } else {
        extra_bytes.clear();
    }
}

/*
 * server_mini_map.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2024.
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

#include "protocol.hpp"
#include "server_mini_map.hpp"

#include "network/byte_buf.hpp"  // coercri

void ServerMiniMap::setSize(int width, int height)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_SET_MAP_SIZE);
    buf.writeUbyte(width);
    buf.writeUbyte(height);
}

void ServerMiniMap::setColour(int x, int y, MiniMapColour col)
{
    // To save a few bytes we encode 'runs' of squares into a single
    // command.
    // 
    // This takes advantage of the fact that, when mapping a whole
    // room, the KnightsEngine will call us in horizontal 'runs' (i.e.
    // looping over y first, then x).

    if (!mini_map_runs.empty()) {
        MiniMapRun &curr_run = mini_map_runs.back();
        const int sz = curr_run.cols.size();
        if (curr_run.y == y && curr_run.start_x + sz == x) {
            curr_run.cols.push_back(col);
            return;
        }
    }

    // add a new run
    MiniMapRun r;
    r.start_x = x;
    r.y = y;
    r.cols.push_back(col);
    mini_map_runs.push_back(r);
}

void ServerMiniMap::wipeMap()
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_WIPE_MAP);
}

void ServerMiniMap::mapKnightLocation(int n, int x, int y)
{
    std::map<int, KtLocn>::iterator it = prev_kt_locn.find(n);
    if (it == prev_kt_locn.end()) {
        KtLocn k;
        k.x = x;
        k.y = y;
        prev_kt_locn.insert(std::make_pair(n, k));
    } else if (it->second.x == x && it->second.y == y) {
        return;
    } else {
        it->second.x = x;
        it->second.y = y;
    }
    
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_MAP_KNIGHT_LOCATION);
    buf.writeUbyte(n);
    if (x<0) {
        buf.writeUbyte(255);
    } else {
        buf.writeUbyte(x);
        buf.writeUbyte(y);
    }
}

void ServerMiniMap::mapItemLocation(int x, int y, bool on)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_MAP_ITEM_LOCATION);
    buf.writeUbyte(x);
    buf.writeUbyte(y);
    buf.writeUbyte(on);
}

void ServerMiniMap::appendMiniMapCmds(std::vector<ubyte> &vec) const
{
    if (!mini_map_runs.empty()) {
        Coercri::OutputByteBuf buf(vec);
        buf.writeUbyte(SERVER_SET_COLOUR);
        buf.writeVarInt(mini_map_runs.size());
        for (std::vector<MiniMapRun>::const_iterator it = mini_map_runs.begin(); it != mini_map_runs.end(); ++it) {
            buf.writeUbyte(it->start_x);
            buf.writeUbyte(it->y);
            buf.writeUbyte(it->cols.size());
            for (size_t i = 0; i < it->cols.size(); ++i) {
                // TODO: could compress even more (each square would fit
                // into 2 bits) but we'll just use one byte per square...
                buf.writeUbyte(it->cols[i]);
            }
        }
    }
}

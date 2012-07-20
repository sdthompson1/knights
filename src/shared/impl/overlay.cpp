/*
 * overlay.cpp
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

#include "anim.hpp"
#include "graphic.hpp"
#include "overlay.hpp"
#include "protocol.hpp"

#include "lua.hpp"

Overlay::Overlay(lua_State *lua, int idx)
    : id(-1)
{
    if (lua) {
        lua_pushvalue(lua, idx); // push
        table_ref.reset(lua);    // pop
    }

    for (int i = 0; i < 4; ++i) raw_graphic[i] = 0;
    for (int i = 0; i < N_OVERLAY_FRAME*4; ++i) {
        offset_data[i].ofsx = offset_data[i].ofsy = 0;
        offset_data[i].dir = D_NORTH;
    }
}

void Overlay::getGraphic(MapDirection facing, int frame, const Graphic *&gfx, int &ofsx, int &ofsy) const
{
    gfx = 0;
    ofsx = ofsy = 0;
    if (frame != AF_THROW_DOWN) {  // overlays aren't drawn for this frame
        MapDirection d = D_NORTH;
        if (frame >= 0 && frame < N_OVERLAY_FRAME) {
            const OffsetData &od(offset_data[frame*4 + facing]);
            ofsx = od.ofsx;
            ofsy = od.ofsy;
            d = od.dir;
        }
        gfx = raw_graphic[d];
    }
}

void Overlay::setRawGraphic(MapDirection d, const Graphic *g)
{
    raw_graphic[d] = g;
}

void Overlay::setOffset(MapDirection facing, int frame, MapDirection new_dir, int ofsx, int ofsy)
{
    if (frame >= 0 && frame < N_OVERLAY_FRAME) {
        OffsetData &od(offset_data[frame*4 + facing]);
        od.ofsx = ofsx;
        od.ofsy = ofsy;
        od.dir = new_dir;
    }
}

Overlay::Overlay(int id_, Coercri::InputByteBuf &buf, const std::vector<const Graphic *> &graphics)
{
    id = id_;
    for (int i = 0; i < 4; ++i) {
        const int gfx_id = buf.readVarInt();
        raw_graphic[i] = gfx_id == 0 ? 0 : graphics.at(gfx_id-1);
    }

    for (int i = 0; i < N_OVERLAY_FRAME*4; ++i) {
        offset_data[i].ofsx = buf.readVarInt();
        offset_data[i].ofsy = buf.readVarInt();
        int d = buf.readUbyte();
        if (d > 3) throw ProtocolError("error reading Overlay");
        offset_data[i].dir = MapDirection(d);
    }
}

void Overlay::serialize(Coercri::OutputByteBuf &buf) const
{
    for (int i = 0; i < 4; ++i) {
        const Graphic *gfx = raw_graphic[i];
        buf.writeVarInt(gfx ? gfx->getID() : 0);
    }

    for (int i = 0; i < N_OVERLAY_FRAME*4; ++i) {
        buf.writeVarInt(offset_data[i].ofsx);
        buf.writeVarInt(offset_data[i].ofsy);
        buf.writeVarInt(offset_data[i].dir);
    }
}

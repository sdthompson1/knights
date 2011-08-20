/*
 * anim.cpp
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

#include "anim.hpp"
#include "graphic.hpp"
#include "protocol.hpp"

Anim::Anim(int id_, Coercri::InputByteBuf &buf, const std::vector<const Graphic*> &graphics)
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < NFRAMES; ++j) {
            const int gfx_id = buf.readVarInt();
            g[i][j] = gfx_id == 0 ? 0 : graphics.at(gfx_id-1);
        }
    }

    cc_normal = ColourChange(buf);
    cc_invulnerable = ColourChange(buf);

    id = id_;

    vbat_mode = buf.readUbyte() != 0;
}

void Anim::serialize(Coercri::OutputByteBuf &buf) const
{
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < NFRAMES; ++j) {
            const Graphic *gfx = g[i][j];
            buf.writeVarInt(gfx == 0 ? 0 : gfx->getID());
        }
    }

    cc_normal.serialize(buf);
    cc_invulnerable.serialize(buf);
    buf.writeUbyte(vbat_mode);
}

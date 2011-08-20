/*
 * graphic.cpp
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
#include "rstream.hpp"

#include "lua.hpp"

Graphic::Graphic(const Graphic &rhs)
    : filename(rhs.filename), hx(rhs.hx), hy(rhs.hy), r(rhs.r), g(rhs.g), b(rhs.b), id(rhs.id)
{
    if (rhs.colour_change.get()) {
        colour_change.reset(new ColourChange(*rhs.colour_change));
    }
}

Graphic::Graphic(int id_, Coercri::InputByteBuf &buf)
{
    filename = buf.readString();
    hx = buf.readVarInt();
    hy = buf.readVarInt();
    r = buf.readVarInt();
    g = buf.readVarInt();
    b = buf.readVarInt();
    id = id_;

    bool use_cc = buf.readUbyte() != 0;
    if (use_cc) {
        colour_change.reset(new ColourChange(buf));
    }
}

void Graphic::serialize(Coercri::OutputByteBuf &buf) const
{
    buf.writeString(filename);
    buf.writeVarInt(hx);
    buf.writeVarInt(hy);
    buf.writeVarInt(r);
    buf.writeVarInt(g);
    buf.writeVarInt(b);
    if (colour_change.get()) {
        buf.writeUbyte(1);
        colour_change->serialize(buf);
    } else {
        buf.writeUbyte(0);
    }
}

std::auto_ptr<Graphic> CreateGraphicFromLua(lua_State *lua)
{
    const int nargs = lua_gettop(lua);
    const char * filename = luaL_checkstring(lua, 1);

    int x=0, y=0, r=-1, g=-1, b=-1;
    
    if (nargs > 1) {
        
        r = luaL_checkinteger(lua, 2);
        g = luaL_checkinteger(lua, 3);
        b = luaL_checkinteger(lua, 4);
        
        if (nargs > 4) {
            x = luaL_checkinteger(lua, 5);
            y = luaL_checkinteger(lua, 6);
        }
    }
    
    std::auto_ptr<Graphic> gfx(new Graphic(filename, x, y, r, g, b));

    return gfx;
}

/*
 * graphic.cpp
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

#include "graphic.hpp"
#include "rstream.hpp"

#include "lua.hpp"

Graphic::Graphic(const Graphic &rhs)
    : file(rhs.file), hx(rhs.hx), hy(rhs.hy), r(rhs.r), g(rhs.g), b(rhs.b),
      size_hint_num(rhs.size_hint_num), size_hint_denom(rhs.size_hint_denom),
      id(rhs.id)
{
    if (rhs.colour_change.get()) {
        colour_change.reset(new ColourChange(*rhs.colour_change));
    }
}

Graphic::Graphic(int id_, Coercri::InputByteBuf &buf)
    : file(buf)
{
    hx = buf.readVarInt();
    hy = buf.readVarInt();
    r = buf.readVarInt();
    g = buf.readVarInt();
    b = buf.readVarInt();
    size_hint_num = buf.readVarInt();
    size_hint_denom = buf.readVarInt();
    id = id_;

    bool use_cc = buf.readUbyte() != 0;
    if (use_cc) {
        colour_change.reset(new ColourChange(buf));
    }
}

void Graphic::serialize(Coercri::OutputByteBuf &buf) const
{
    file.serialize(buf);
    buf.writeVarInt(hx);
    buf.writeVarInt(hy);
    buf.writeVarInt(r);
    buf.writeVarInt(g);
    buf.writeVarInt(b);
    buf.writeVarInt(size_hint_num);
    buf.writeVarInt(size_hint_denom);
    if (colour_change.get()) {
        buf.writeUbyte(1);
        colour_change->serialize(buf);
    } else {
        buf.writeUbyte(0);
    }
}

std::unique_ptr<Graphic> CreateGraphicFromLua(lua_State *lua)
{
    const int nargs = lua_gettop(lua);
    const char * filename = luaL_checkstring(lua, 1);

    int x=0, y=0, r=-1, g=-1, b=-1;
    int size_hint_num = 1, size_hint_denom = 1;
    
    if (nargs > 1) {
        
        r = luaL_checkint(lua, 2);
        g = luaL_checkint(lua, 3);
        b = luaL_checkint(lua, 4);
        
        if (nargs > 4) {
            x = luaL_checkint(lua, 5);
            y = luaL_checkint(lua, 6);

            if (nargs > 6) {
                size_hint_num = luaL_checkint(lua, 7);
                size_hint_denom = luaL_checkint(lua, 8);
            }
        }
    }

    lua_getglobal(lua, "_CWD");  // [_CWD]
    const char *cwd = lua_tostring(lua, -1);
    std::unique_ptr<Graphic> gfx(new Graphic(FileInfo(filename, cwd), x, y, r, g, b, size_hint_num, size_hint_denom));
    lua_pop(lua, 1);  // []
    
    return gfx;
}

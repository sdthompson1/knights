/*
 * graphic.hpp
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

/*
 * Stores filename and other information needed to identify a Graphic.
 *
 */

#ifndef GRAPHIC_HPP
#define GRAPHIC_HPP

#include "colour_change.hpp"
#include "file_info.hpp"

#include "network/byte_buf.hpp" // coercri

#include <memory>
#include <string>

struct lua_State;

class Graphic {
public:
    explicit Graphic(const FileInfo &file_,
                     int hx_ = 0, int hy_ = 0,
                     int r_ = -1, int g_ = -1, int b_ = -1,
                     int size_hint_num_ = 1, int size_hint_denom_ = 1)
        : file(file_),
          hx(hx_), hy(hy_), r(r_), g(g_), b(b_),
          size_hint_num(size_hint_num_), size_hint_denom(size_hint_denom_),
          id(0)
    { }

    // copy ctor. takes copy of the colour change if there is one.
    Graphic(const Graphic &rhs);

    const FileInfo &getFileInfo() const { return file; }
    int getHX() const { return hx; }
    int getHY() const { return hy; }

    // returns (-1,-1,-1) if transparency not used.
    int getR() const { return r; }
    int getG() const { return g; }
    int getB() const { return b; }

    // this is used for gfx like the Ogre which are drawn at a larger-than-normal size.
    // (e.g. size_hint == 1 implies the graphic is drawn at 3x the normal size)
    float getSizeHint() const { return float(size_hint_num)/float(size_hint_denom); }

    void setColourChange(ColourChange cc) { colour_change.reset(new ColourChange(cc)); }
    const ColourChange * getColourChange() const { return colour_change.get(); }
    
    int getID() const { return id; }    
    void setID(int i) { id = i; }

    // serialization
    explicit Graphic(int id_, Coercri::InputByteBuf &buf);
    void serialize(Coercri::OutputByteBuf &buf) const;
    
private:
    FileInfo file;
    int hx, hy;
    int r, g, b;
    int size_hint_num, size_hint_denom;  // stored as numerator/denominator to avoid complication of having to serialize floats.
    int id;
    std::unique_ptr<ColourChange> colour_change;  // most tiles don't use this. only used for the dead knight tiles currently.
};

// NOTE: The following may call lua_error
std::unique_ptr<Graphic> CreateGraphicFromLua(lua_State *lua);

#endif

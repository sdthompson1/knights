/*
 * anim.hpp
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

/*
 * Holds a set of graphics, for different facing directions and
 * animation frames. Used for rendering entities.
 *
 * We also have a special "vbat mode" which is used to do the flapping
 * wings of the vampire bats.
 * 
 */

#ifndef ANIM_HPP
#define ANIM_HPP

#include "colour_change.hpp"
#include "map_support.hpp"

#include "network/byte_buf.hpp" // coercri

class Graphic;

class Anim {
public:
    explicit Anim(int id_) : id(id_), vbat_mode(false)
        { for (int i=0; i<4; ++i) for (int j=0; j<NFRAMES; ++j) g[i][j] = 0; }
    void setBatMode() { vbat_mode = true; }

    const Graphic * getGraphic(MapDirection facing, int frame) const
        { return g[facing][frame]; }
    const ColourChange & getColourChange(bool invuln) const
        { return invuln ? cc_invulnerable : cc_normal; }
    bool getVbatMode() const
        { return vbat_mode; }
    int getID() const
        { return id; }
    
    void setGraphic(MapDirection d, int frm, const Graphic *g_) 
        { g[d][frm] = g_; }
    void setColourChangeNormal(const ColourChange &cc)
        { cc_normal = cc; }
    void setColourChangeInvulnerable(const ColourChange &cc)
        { cc_invulnerable = cc; }
    void setID(int i)  // called by KnightsConfig
        { id = i; }

    // serialization
    explicit Anim(int id_, Coercri::InputByteBuf &buf, const std::vector<const Graphic *> &graphics);
    void serialize(Coercri::OutputByteBuf &buf) const;
    
private:
    enum { NFRAMES = 8 };
    const Graphic * g[4][NFRAMES]; // 4 facing directions
    ColourChange cc_normal, cc_invulnerable;
    int id;
    bool vbat_mode;
};

// some standard frame numbers
enum AnimFrame { AF_NORMAL=0, AF_BACKSWING=1, AF_IMPACT=2, AF_PARRY=3, AF_THROW_BACK=4, AF_THROW_DOWN=5,
                 AF_XBOW=6, AF_XBOW_LOAD=7 };

#endif

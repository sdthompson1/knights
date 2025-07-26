/*
 * anim.hpp
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
#include "lua_ref.hpp"
#include "map_support.hpp"

#include "network/byte_buf.hpp" // coercri

class Graphic;

class Anim {
public:
    // Construct from lua
    Anim(int id_, lua_State *lua, int idx);

    void newIndex(lua_State *lua);
    
    // push the stored table
    void pushTable(lua_State *lua) const { table_ref.push(lua); }

    // Overwrite id (useful if copying another Anim and then modifying it)
    void setID(int id_) { id = id_; }
    
    // Set the colour changes (which can't be initialized from the lua table currently).
    void setColourChangeNormal(const ColourChange &cc)
        { cc_normal = cc; }
    void setColourChangeInvulnerable(const ColourChange &cc)
        { cc_invulnerable = cc; }

    // Accessors
    const Graphic * getGraphic(MapDirection facing, int frame) const
        { return g[facing][frame]; }
    const ColourChange & getColourChange(bool invuln) const
        { return invuln ? cc_invulnerable : cc_normal; }
    bool getVbatMode() const
        { return vbat_mode; }
    int getID() const
        { return id; }
    
    // Serialization
    explicit Anim(int id_, Coercri::InputByteBuf &buf, const std::vector<const Graphic *> &graphics);
    void serialize(Coercri::OutputByteBuf &buf) const;
    
private:
    enum { NFRAMES = 8 };
    const Graphic * g[4][NFRAMES]; // 4 facing directions
    ColourChange cc_normal, cc_invulnerable;
    int id;
    bool vbat_mode;
    LuaRef table_ref;
};

// some standard frame numbers
enum AnimFrame { AF_NORMAL=0, AF_BACKSWING=1, AF_IMPACT=2, AF_PARRY=3, AF_THROW_BACK=4, AF_THROW_DOWN=5,
                 AF_XBOW=6, AF_XBOW_LOAD=7 };

#endif

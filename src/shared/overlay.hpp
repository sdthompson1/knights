/*
 * overlay.hpp
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

/*
 * Overlays are drawn over the top of an Anim and are used to draw
 * held items.
 *
 */

#ifndef OVERLAY_HPP
#define OVERLAY_HPP

#include "map_support.hpp"

class Graphic;

class Overlay {
public:
    explicit Overlay(int id_);

    int getID() const { return id; }

    // Given entity's current facing and frame, return which graphic
    // should be shown and the offset to display it at. NB can return
    // graphic==0 in which case the overlay should not be drawn at
    // all.
    void getGraphic(MapDirection facing, int frame, const Graphic *&gfx, int &ofsx, int &ofsy) const;

    // set functions. called by KnightsConfigImpl.
    void setRawGraphic(MapDirection d, const Graphic *g);
    void setOffset(MapDirection facing, int frame, MapDirection new_dir, int ofsx, int ofsy);

    enum { N_OVERLAY_FRAME = 5 };

    // serialization
    explicit Overlay(int id_, Coercri::InputByteBuf &buf, const std::vector<const Graphic *> &graphics);
    void serialize(Coercri::OutputByteBuf &buf) const;
    
private:
    int id;
    const Graphic *raw_graphic[4];

    struct OffsetData {
        int ofsx;
        int ofsy;
        MapDirection dir;
    };
    OffsetData offset_data[N_OVERLAY_FRAME*4];
};

#endif

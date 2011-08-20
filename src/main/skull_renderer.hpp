/*
 * skull_renderer.hpp
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
 * Draws the columns of skulls, representing how many times you have died.
 *
 */

#ifndef SKULL_RENDERER_HPP
#define SKULL_RENDERER_HPP

#include "gfx/gfx_context.hpp" // from coercri

#include <vector>
using namespace std;

class SkullRenderer {
public:
    //
    // setup
    //
    
    SkullRenderer() { }
    
    // The skull graphics should be added in ascending order. The first has one skull
    // on it, the second has two skulls etc.
    void addGraphic(const Graphic *);

    // Pixel positions for the rows and columns. A graphic is drawn at (r,c) for each r and
    // each c, until all skulls have been drawn.
    void addColumn(int c);
    void addRow(int r);

    
    //
    // drawing
    //

    // draw (clip rectangle should be unset, will be taken care of by this func)
    void draw(Coercri::GfxContext &gc, GfxManager &gm, int no_skulls, int left, int top, float scale) const;
   
private:
    vector<const Graphic *> gfx;
    vector<int> rows, columns;
};

#endif

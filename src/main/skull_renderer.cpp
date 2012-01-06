/*
 * skull_renderer.cpp
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

#include "gfx_manager.hpp"
#include "round.hpp"
#include "skull_renderer.hpp"

void SkullRenderer::addGraphic(const Graphic *g)
{
    gfx.push_back(g);
}

void SkullRenderer::addColumn(int c)
{
    columns.push_back(c);
}

void SkullRenderer::addRow(int r)
{
    rows.push_back(r);
}

void SkullRenderer::draw(Coercri::GfxContext &gc, GfxManager &gm, int nsk, int left, int top, float scale_factor) const
{
    int r = 0;
    int c = 0;
    while (nsk > 0) {
        const int x = Round(float(columns[c])*scale_factor) + left;
        const int y = Round(float(rows[r])*scale_factor) + top;
        const int n = min(nsk, int(gfx.size()));

        int w, h;
        gm.getGraphicSize(*gfx[n-1], w, h);
        w = Round(float(w) * scale_factor);
        h = Round(float(h) * scale_factor);

        gm.drawTransformedGraphic(gc, x, y, *gfx[n-1], w, h);
        nsk -= n;
        ++r;
        if (r >= rows.size()) {
            r = 0;
            ++c;
            if (c >= columns.size()) {
                // maximum number of skulls has been reached
                break;
            }
        }
    }
}

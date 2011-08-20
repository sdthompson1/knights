/*
 * graphic_transform.cpp
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

#include "colour_change.hpp"
#include "gfx_resizer.hpp"
#include "graphic_transform.hpp"
#include "round.hpp"

boost::shared_ptr<const Coercri::PixelArray> CreateGraphicWithCC(boost::shared_ptr<const Coercri::PixelArray> pixels,
                                                          const ColourChange &cc)
{
    const int width = pixels->getWidth();
    const int height = pixels->getHeight();
    boost::shared_ptr<Coercri::PixelArray> new_pixels(new Coercri::PixelArray(width, height));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Coercri::Color &in = (*pixels)(x,y);
            Colour result;
            Coercri::Color &out = (*new_pixels)(x,y);
            if (cc.lookup(Colour(in.r, in.g, in.b, in.a), result)) {
                out.r = result.r;
                out.g = result.g;
                out.b = result.b;
                out.a = result.a;
            } else {
                out = in;
            }
        }
    }

    return new_pixels;
}

boost::shared_ptr<const Coercri::PixelArray> CreateResizedGraphic(const GfxResizer &resizer,
                                                           boost::shared_ptr<const Coercri::PixelArray> original,
                                                           int new_width, int new_height,
                                                           int old_hx, int old_hy, int &new_hx, int &new_hy)
{
    const int old_width = original->getWidth();
    const float scale = float(new_width) / float(old_width);
    new_hx = Round(float(old_hx) * scale);
    new_hy = Round(float(old_hy) * scale);
    return resizer.resize(original, new_width, new_height);
}

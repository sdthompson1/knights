/*
 * gfx_resizer_nearest_nbr.cpp
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

#include "gfx_resizer_nearest_nbr.hpp"

boost::shared_ptr<const Coercri::PixelArray> GfxResizerNearestNbr::resize(boost::shared_ptr<const Coercri::PixelArray> original,
                                                                          int new_width, int new_height) const
{
    const float new_width_f = float(new_width);
    const float new_height_f = float(new_height);
    const float old_width_f = float(original->getWidth());
    const float old_height_f = float(original->getHeight());
    const float scale_w = old_width_f / new_width_f;
    const float scale_h = old_height_f / new_height_f;

    boost::shared_ptr<Coercri::PixelArray> new_pixels(new Coercri::PixelArray(new_width, new_height));

    for (int y=0; y<new_height; ++y) {
        for (int x=0; x<new_width; ++x) {
            
            int old_x = int(float(x + 0.5f) * scale_w);
            int old_y = int(float(y + 0.5f) * scale_h);
            
            if (old_x >= original->getWidth()) old_x = original->getWidth() - 1;
            if (old_y >= original->getHeight()) old_y = original->getHeight() - 1;

            ASSERT(old_x >= 0 && old_y >= 0);

            (*new_pixels)(x,y) = (*original)(old_x, old_y);
        }
    }

    return new_pixels;
}

/*
 * gfx_resizer.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2013.
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
 * GfxResizer: Interface for working out "resized rectangles". This
 * allows the display to adapt if the window is resized by the user.
 *
 */

#ifndef GFX_RESIZER_HPP
#define GFX_RESIZER_HPP

#include "gfx/pixel_array.hpp"   // coercri

#include "boost/shared_ptr.hpp"

class GfxResizer {
public:
    virtual ~GfxResizer() { }

    // Rounds the scale factor to a value acceptable to the resizer.
    // Post-condition: either rounded_down <= ideal_scale_factor <= rounded_up,
    // or rounded_down == rounded_up < ideal_scale_factor,
    // or ideal_scale_factor < rounded_down == rounded_up
    // (the latter two cases occur if the ideal_scale_factor is out of range for this resizer).

    // NOTE: At the moment we have a restriction that this always returns integer factors, OR
    // is completely unrestricted (because of the way getResizedRectangle works; see display.cpp)
    
    virtual void roundScaleFactor(float ideal_scale_factor, float &rounded_down, float &rounded_up) const
    {
        rounded_down = rounded_up = ideal_scale_factor;
    }
    
    // Resize a PixelArray
    virtual boost::shared_ptr<const Coercri::PixelArray> resize(boost::shared_ptr<const Coercri::PixelArray> original, 
                                                                int new_width, int new_height) const = 0;
};

#endif

                                     

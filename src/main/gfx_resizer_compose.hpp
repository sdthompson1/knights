/*
 * gfx_resizer_compose.hpp
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
 * GfxResizerCompose: composes two GfxResizers, useful e.g. for running
 * Scale2x followed by a nearest-nbr or bilinear pass.
 * 
 */

#ifndef GFX_RESIZER_COMPOSE
#define GFX_RESIZER_COMPOSE

#include "gfx_resizer.hpp"

class GfxResizerCompose : public GfxResizer {
public:
    // NOTE: Can set right=NULL, cannot set left=NULL though.
    GfxResizerCompose(boost::shared_ptr<GfxResizer> left_, boost::shared_ptr<GfxResizer> right_, bool lock_to_int_)
        : left(left_), right(right_), lock_to_int(lock_to_int_) { }

    virtual void roundScaleFactor(float ideal_scale_factor, float &rounded_down, float &rounded_up) const;

    virtual boost::shared_ptr<const Coercri::PixelArray> resize(boost::shared_ptr<const Coercri::PixelArray> original,
                                                                int new_width, int new_height) const;

private:
    float intermedScaleFactor(float ideal_scale_factor) const;
    
    boost::shared_ptr<GfxResizer> left, right;
    bool lock_to_int;
};

#endif

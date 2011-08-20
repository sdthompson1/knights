/*
 * gfx_resizer_nearest_nbr.hpp
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
 * GfxResizerNearestNbr: Uses nearest-neighbour method to resize a graphic
 *
 */

#ifndef GFX_RESIZER_NEAREST_NBR
#define GFX_RESIZER_NEAREST_NBR

#include "gfx_resizer.hpp"

class GfxResizerNearestNbr : public GfxResizer {
public:
    virtual boost::shared_ptr<const Coercri::PixelArray> resize(boost::shared_ptr<const Coercri::PixelArray> original,
                                                                int new_width, int new_height) const;
};

#endif

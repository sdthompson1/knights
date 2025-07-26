/*
 * graphic_transform.hpp
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
 * Resizing and Colour-Changing of graphics
 *
 */

#ifndef GRAPHIC_TRANSFORM_HPP
#define GRAPHIC_TRANSFORM_HPP

#include "gfx/pixel_array.hpp"  // coercri

#include "boost/shared_ptr.hpp"

class ColourChange;
class GfxResizer;

// alpha of the new graphic = max(original alpha, max_alpha)
// This is used for drawing invisible teammates
boost::shared_ptr<const Coercri::PixelArray> CreateGraphicWithCC(boost::shared_ptr<const Coercri::PixelArray> original,
                                                          const ColourChange &cc,
                                                          unsigned char max_alpha = 255);

boost::shared_ptr<const Coercri::PixelArray> CreateResizedGraphic(const GfxResizer &resizer,
                                                           boost::shared_ptr<const Coercri::PixelArray> original,
                                                           int new_width, int new_height,
                                                           int old_hx, int old_hy, int &new_hx, int &new_hy);

#endif

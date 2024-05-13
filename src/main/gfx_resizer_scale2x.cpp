/*
 * gfx_resizer_scale2x.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2024.
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
 * TODO: May want to introduce a colour tolerance, instead of using
 * exact == operator for the pixel comparisons? (It's fine at the
 * moment because the Knights graphics use a limited palette so
 * close-but-not-identical colours do not arise.)
 * 
 */

#include "misc.hpp"

#include "gfx_resizer_scale2x.hpp"

#include <algorithm>
#include <cmath>
using namespace std;

namespace {

    // The Scale2x algorithm
    
    boost::shared_ptr<Coercri::PixelArray> Scale2x(boost::shared_ptr<const Coercri::PixelArray> original)
    {
        const int width = original->getWidth();
        const int height = original->getHeight();
        
        boost::shared_ptr<Coercri::PixelArray> output(new Coercri::PixelArray(width*2, height*2));
        
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                using Coercri::Color;
                
                const int yminus = y == 0 ? 0 : y-1;
                const int yplus = y == height-1 ? height-1 : y+1;
                const int xminus = x == 0 ? 0 : x-1;
                const int xplus = x == width-1 ? width-1 : x+1;

                const Color& b = (*original)(x, yminus);
                const Color& d = (*original)(xminus, y);
                const Color& e = (*original)(x, y);
                const Color& f = (*original)(xplus, y);
                const Color& h = (*original)(x, yplus);
                
                Color& e0 = (*output)(x*2, y*2);
                Color& e1 = (*output)(x*2+1, y*2); 
                Color& e2 = (*output)(x*2, y*2+1);
                Color& e3 = (*output)(x*2+1, y*2+1);
                
                if (b != h && d != f) {
                    e0 = d == b ? d : e;
                    e1 = b == f ? f : e;
                    e2 = d == h ? d : e;
                    e3 = h == f ? f : e;
                } else {
                    e0 = e1 = e2 = e3 = e;
                }
            }
        }
        
        return output;
    }

    
    // The Scale3x algorithm

    boost::shared_ptr<Coercri::PixelArray> Scale3x(boost::shared_ptr<const Coercri::PixelArray> original)
    {
        const int width = original->getWidth();
        const int height = original->getHeight();

        boost::shared_ptr<Coercri::PixelArray> output(new Coercri::PixelArray(width*3, height*3));
        
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                using Coercri::Color;
                
                const int yminus = y == 0 ? 0 : y-1;
                const int yplus = y == height-1 ? height-1 : y+1;
                const int xminus = x == 0 ? 0 : x-1;
                const int xplus = x == width-1 ? width-1 : x+1;

                const Color& a = (*original)(xminus, yminus);
                const Color& b = (*original)(x,      yminus);
                const Color& c = (*original)(xplus,  yminus);
                const Color& d = (*original)(xminus, y);
                const Color& e = (*original)(x,      y);
                const Color& f = (*original)(xplus,  y);
                const Color& g = (*original)(xminus, yplus);
                const Color& h = (*original)(x,      yplus);
                const Color& i = (*original)(xplus,  yplus);

                Color& e0 = (*output)(x*3,   y*3);
                Color& e1 = (*output)(x*3+1, y*3);
                Color& e2 = (*output)(x*3+2, y*3);
                Color& e3 = (*output)(x*3,   y*3+1);
                Color& e4 = (*output)(x*3+1, y*3+1);
                Color& e5 = (*output)(x*3+2, y*3+1);
                Color& e6 = (*output)(x*3,   y*3+2);
                Color& e7 = (*output)(x*3+1, y*3+2);
                Color& e8 = (*output)(x*3+2, y*3+2);

                if (b != h && d != f) {
                    e0 = d == b ? d : e;
                    e1 = (d == b && e != c) || (b == f && e != a) ? b : e;
                    e2 = b == f ? f : e;
                    e3 = (d == b && e != g) || (d == h && e != a) ? d : e;
                    e4 = e;
                    e5 = (b == f && e != i) || (h == f && e != c) ? f : e;
                    e6 = d == h ? d : e;
                    e7 = (d == h && e != i) || (h == f && e != g) ? h : e;
                    e8 = h == f ? f : e;
                } else {
                    e0 = e1 = e2 = e3 = e4 = e5 = e6 = e7 = e8 = e;
                }
            }
        }

        return output;
    }
}

namespace {
    bool Check(int factor)
    {
        ASSERT(factor >= 1);
        
        while ((factor & 1) == 0) {
            factor >>= 1;
        }

        while (factor % 3 == 0 && factor >= 3) {
            factor /= 3;
        }

        return (factor == 1);
    }
    
    int Round(int factor)
    {
        // We need to find the greatest X < factor
        // such that X is of the form 2^a * 3^b (for a,b integers).

        // Crappy algorithm I know, but we'll do this by brute force
        // testing all integers starting from X and working downwards.

        for (int test = factor; test >= 1; ++test) {
            if (Check(test)) return test;
        }

        ASSERT(false); // should never reach here, because test==1 should always work!
        return 1; // prevents compiler warning
    }
}       

void GfxResizerScale2x::roundScaleFactor(float ideal_scale_factor, float &rounded_down, float &rounded_up) const
{    
    // For lower bound: need to find greatest X <= factor
    // such that X is of the form 2^a * 3^b (for a,b integers).

    // Crappy algorithm I know, but do this by brute force
    // testing all integers starting from floor(ideal_scale_factor) and
    // working downwards.

    int lower_result = int(ideal_scale_factor);
    if (lower_result < 1) lower_result = 1;
    while (!Check(lower_result)) --lower_result;

    // For upper bound do it the other way around ie increase from ceil(ideal_scale_factor).
    int upper_result = int(ceil(ideal_scale_factor));
    if (upper_result < 1) upper_result = 1;
    while (!Check(upper_result)) ++upper_result;
    
    rounded_down = float(lower_result);
    rounded_up = float(upper_result);

    // Clamp to max_scale if required
    if (max_scale > 0) {
        rounded_down = min(rounded_down, float(max_scale));
        rounded_up = min(rounded_up, float(max_scale));
    }
}

boost::shared_ptr<const Coercri::PixelArray> GfxResizerScale2x::resize(boost::shared_ptr<const Coercri::PixelArray> original,
                                                                       int new_width, int new_height) const
{
    // We assume that they are scaling to a size we support.
    // (In other words, they should have called roundScaleFactor to get an acceptable scale factor before calling this routine...
    // but the Knights code always works this way, at least at the moment.)
    ASSERT(new_height >= original->getHeight());
    ASSERT(new_width >= original->getWidth());
    ASSERT(new_height / original->getHeight() == new_width / original->getWidth());

    int factor = Round(new_height / original->getHeight());

    // We arbitrarily decide to do the scale3x before the scale2x. (Could try this either way around,
    // although arguably 6x or higher magnification is unlikely in practice anyway...)

    boost::shared_ptr<const Coercri::PixelArray> result = original;
    int old_width = original->getWidth();
    int old_height = original->getHeight();
    
    while (factor % 3 == 0) {
        result = Scale3x(result);
        old_width *= 3;
        old_height *= 3;
        factor /= 3;
    }

    while (factor % 2 == 0) {
        result = Scale2x(result);
        old_width *= 2;
        old_height *= 2;
        factor /= 2;
    }

    // If this assertion fails you are trying to use an inappropriate scale factor
    // e.g. trying to scale by 5x.
    ASSERT(old_width*factor == new_width);
    ASSERT(old_height*factor == new_height);
    
    return result;
}

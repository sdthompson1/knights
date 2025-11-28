/*
 * gfx_resizer_compose.cpp
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

#include "misc.hpp"

#include "gfx_resizer_compose.hpp"
#include "round.hpp"

#include <cmath>
using namespace std;

float GfxResizerCompose::intermedScaleFactor(float ideal_scale_factor) const
{
    if (!right) return 1;  // "right" is assumed to be the identity in this case.
    
    float intermed_down, intermed_up;
    right->roundScaleFactor(ideal_scale_factor, intermed_down, intermed_up);

    // we use intermed_down or intermed_up, whichever is closer
    const float diff_down = fabs(intermed_down - ideal_scale_factor);
    const float diff_up = fabs(intermed_up - ideal_scale_factor);
    
    return (diff_up < diff_down) ? intermed_up : intermed_down;
}

void GfxResizerCompose::roundScaleFactor(float ideal_scale_factor, float &final_down, float &final_up) const
{
    if (lock_to_int) {
        // If lock_to_int is set then first we round ideal_scale_factor to an int, THEN we round that to
        // whatever the scalers want. So we don't guarantee an integer scaling; we merely get as close as we
        // can to that, given what the scalers want to do.
        // Note: We always round down rather than up.
        // Note: We don't do any rounding if the scale factor is less than one (otherwise the scale
        // factor would become zero which causes bugs later on).
        if (ideal_scale_factor >= 1) ideal_scale_factor = int(ideal_scale_factor);
    }
    
    // Work out what the intermediate will scale to.
    const float intermed = intermedScaleFactor(ideal_scale_factor);
    
    // We now have a graphic of size intermed times the original
    // This must be scaled by ideal_scale_factor / intermed to get to the required final size.
    left->roundScaleFactor(ideal_scale_factor / intermed, final_down, final_up);

    // correct for the fact that we did the intermediate scaling!
    final_down *= intermed;
    final_up *= intermed;
}

Coercri::PixelArray GfxResizerCompose::resize(const Coercri::PixelArray &original,
                                              int new_width, int new_height) const
{
    const float ideal_scale_factor = float(new_height)/float(original.getHeight());

    const float intermed_scale_factor = intermedScaleFactor(ideal_scale_factor);
    const int intermed_width = Round(original.getWidth() * intermed_scale_factor);
    const int intermed_height = Round(original.getHeight() * intermed_scale_factor);

    if (right) {
        Coercri::PixelArray intermed = right->resize(original, intermed_width, intermed_height);
        return left->resize(intermed, new_width, new_height);
    } else {
        return left->resize(original, new_width, new_height);
    }
}

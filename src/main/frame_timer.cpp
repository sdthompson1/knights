/*
 * frame_timer.cpp
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

#include "frame_timer.hpp"

// Coercri includes
#include "timer/timer.hpp"

#include <algorithm>

namespace {
    // Max slack allowed between t_smoothed_us and t_now_us
    const int64_t SYNC_LIMIT_US = 50000;

    // Fraction to lerp delta_t_us, towards the actual measured
    // value, each frame
    const int64_t LERP_DIVISOR = 10;

    // Rounded division function
    int64_t RoundedDivide(int64_t value, int64_t divisor) {
        return (value + (value >= 0 ? divisor/2 : -divisor/2)) / divisor;
    }
}

FrameTimer::FrameTimer(Coercri::Timer &timer_, int max_fps)
    : timer(timer_)
{
    t_prev_us = t_smoothed_us = t_prev_smoothed_us = 0;
    delta_t_us = 0;
    if (max_fps == 0) {
        min_frame_time_us = 0;
    } else {
        min_frame_time_us = INT64_C(1000000) / int64_t(max_fps);
    }
}

void FrameTimer::markEndOfFrame()
{
    // Backup old t_smoothed_us value
    t_prev_smoothed_us = t_smoothed_us;

    // Get the time now
    uint64_t t_now_us = timer.getUsec();
    int64_t measured_delta_t_us = t_now_us - t_prev_us;

    // Will adding delta_t_us to t_smoothed_us keep us within a few milliseconds
    // (i.e. SYNC_LIMIT_US) of the real time?
    int64_t diff_us = t_smoothed_us + delta_t_us - t_now_us;
    if (diff_us > SYNC_LIMIT_US || diff_us < -SYNC_LIMIT_US) {

        // No, that would push us too far out of sync with real time.

        // Instead, just set t_smoothed_us to the real time (but not allowing
        // it to go backwards).

        t_smoothed_us = std::max(t_smoothed_us, t_now_us);
        delta_t_us = measured_delta_t_us;

    } else {

        // Yes, that would be valid, and keep t_smoothed_us within a
        // few milliseconds of real time. So let's do that.

        t_smoothed_us += delta_t_us;

        // Now update delta_t_us to be closer to measured_delta_t_us.

        int64_t difference_us = measured_delta_t_us - delta_t_us;
        delta_t_us += RoundedDivide(difference_us, LERP_DIVISOR);
    }

    // Store this frame's "timer.getUsec" value for use in next
    // frame's calculations.
    t_prev_us = t_now_us;

    // If delta_t has somehow gone negative (or zero) then reset it to 1
    if (delta_t_us <= 0) {
        delta_t_us = 1;
    }
}

int64_t FrameTimer::getSleepTimeUsec() const
{
    int64_t frame_time_us = t_smoothed_us - t_prev_smoothed_us;
    if (frame_time_us < min_frame_time_us) {
        return min_frame_time_us - frame_time_us;
    } else {
        return 0;
    }
}

/*
 * frame_timer.hpp
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

/* FrameTimer is responsible for calculating frame timestamps.

   Ideally, we would have some sort of graphics API that would give
   a microsecond-accurate timestamp for when the next frame is going
   to hit the display (e.g. time of the next VSYNC or something like that).

   Unfortunately, that is not available in SDL, so the next idea is to
   estimate it by saying that the timestamp of each frame is just the
   value of timer->getUsec() at the moment that the previous frame's
   Present call returns. In theory, if Vsync is enabled, Present
   should return immediately after the vertical blank, and this should
   give us nice stable timer readings.

   However, in practice, it does not work like that. The graphics
   driver can insert vsync delays at any point, not just in Present.
   So trying to use the timer in that way results in "wobbly"
   animation (sometimes delta_t is too high and sometimes it is too
   low). (Also, vsync might not be enabled.)

   Therefore, this class will measure timer->getUsec() after each
   Present call, but also apply some "smoothing" to the measured timer
   values. This should mean that any "wobbles" are smoothed out over
   several frames and are therefore less noticeable.
*/


#ifndef FRAME_TIMER_HPP
#define FRAME_TIMER_HPP

#include <cstdint>

namespace Coercri {
    class Timer;
}

class FrameTimer {
public:
    // Note: If the Timer is destroyed then no further calls to the
    // FrameTimer methods should be made.
    FrameTimer(Coercri::Timer &timer_, int max_fps);

    // Call this at the end of the game main loop. A time sample will
    // be taken.
    void markEndOfFrame();

    // Call this to obtain a smoothed microsecond timestamp
    // appropriate for the next frame to be rendered.
    uint64_t getFrameTimestampUsec() const { return t_smoothed_us; }

    // Call this to figure out how long we should sleep for to enforce
    // the max_fps limit. (Note this can return 0 to indicate "no
    // sleep required".) This should be called immediately after
    // markEndOfFrame.
    int64_t getSleepTimeUsec() const;

private:
    Coercri::Timer &timer;
    uint64_t t_prev_us;     // Exact real time at previous "markEndOfFrame"
    uint64_t t_smoothed_us; // Smoothed timestamp for use with next frame
    uint64_t t_prev_smoothed_us;  // Previous value of t_smoothed_us
    int64_t delta_t_us;     // Current time-delta
    int64_t min_frame_time_us;
};

#endif

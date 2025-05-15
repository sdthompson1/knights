/*
 * frame_time_adjust.cpp
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

#include "frame_time_adjust.hpp"

unsigned int MsecToFrameCount(unsigned int total_msec, unsigned int fps)
{
    // Split into seconds and milliseconds to avoid 32-bit overflow issues.
    unsigned int seconds = total_msec / 1000;
    unsigned int milliseconds = total_msec % 1000;

    // Then calculate number of frames.
    // One second = "fps" frames
    // One millisecond = "fps/1000" frames
    return seconds * fps + (milliseconds * fps) / 1000;
}

unsigned int FrameCountToMsec(unsigned int frames, unsigned int fps)
{
    // Split into multiples of fps, and remainder
    unsigned int multiples_of_fps = frames / fps;
    unsigned int remainder = frames % fps;

    // Each "multiple of fps" corresponds to one second, or 1000 milliseconds
    // Each "remainder" corresponds to one frame, or (1000/fps) milliseconds
    // Note: The division needs to round up, so that this function is the inverse of
    // MsecToFrameCount; this is why we add fps-1 below.
    return 1000 * multiples_of_fps + (1000 * remainder + fps - 1) / fps;
}

/*
 * screen.hpp
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
 * Base class for game screens (menu screen, in game screen, etc)
 * 
 */

#ifndef SCREEN_HPP
#define SCREEN_HPP

// coercri
#include "gfx/window.hpp"

#include "guichan.hpp"

#include "boost/shared_ptr.hpp"

#include <cstdint>

class KnightsApp;

class Screen {
public:
    // This method starts the screen
    // Returns true if it wants gui enabled, or false if disabled.
    virtual bool start(KnightsApp &app, boost::shared_ptr<Coercri::Window> window, gcn::Gui &gui) = 0;

    // The destructor should stop the screen (or do nothing if start() wasn't called)
    virtual ~Screen() { }
    
    // Update and draw methods.
    // Update is called every frame.
    // Draw is called after update, but only if window->needsRepaint() is true.
    // (Note: Main loop will call cg_listener->draw(), so draw() should
    // only do non-Guichan drawing.)
    virtual void update() { }
    virtual void draw(uint64_t frame_timestamp_us, Coercri::GfxContext &gc) { }
};

#endif

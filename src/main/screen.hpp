/*
 * screen.hpp
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
 * Base class for game screens (menu screen, in game screen, etc)
 * 
 */

#ifndef SCREEN_HPP
#define SCREEN_HPP

// coercri
#include "gfx/window.hpp"

#include "guichan.hpp"

#include "boost/shared_ptr.hpp"

class KnightsApp;

class Screen {
public:
    // This method starts the screen
    // Returns true if it wants gui enabled, or false if disabled.
    virtual bool start(KnightsApp &app, boost::shared_ptr<Coercri::Window> window, gcn::Gui &gui) = 0;

    // The destructor should stop the screen (or do nothing if start() wasn't called)
    virtual ~Screen() { }
    
    // getUpdateInterval: How often we want periodic updates (or 0 if we don't need this).
    // update() will be called at MOST every getUpdateInterval(), but can be called less frequently if CPU time is limited.
    // NOTE: update() ceases to be called after a successful requestScreenChange().
    virtual unsigned int getUpdateInterval() { return 0; }
    virtual void update() { }

    // draw: Draw the screen
    // NOTE: Main loop will call cg_listener->draw() so this is only needed for non-Guichan drawing.
    virtual void draw(Coercri::GfxContext &gc) { }

    virtual unsigned int getMaxLag() { return getUpdateInterval() * 5; }
};

#endif

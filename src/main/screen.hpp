/*
 * screen.hpp
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

enum UpdateType {
    // update() and draw() are both called every "frame" (frame duration is set by
    // "fps" setting in client_config)
    UPDATE_TYPE_REALTIME,

    // update() is called approximately every 100 ms
    // draw() is only called when the window needs repainting
    UPDATE_TYPE_INFREQUENT
};

class Screen {
public:
    // This method starts the screen
    // Returns true if it wants gui enabled, or false if disabled.
    virtual bool start(KnightsApp &app, boost::shared_ptr<Coercri::Window> window, gcn::Gui &gui) = 0;

    // The destructor should stop the screen (or do nothing if start() wasn't called)
    virtual ~Screen() { }
    
    // Which update type does this screen require (see above)
    virtual UpdateType getUpdateType() { return UPDATE_TYPE_INFREQUENT; }

    // Update and draw methods. See above for the schedule on which these are called.
    // (Note: both might be called less frequently than advertised if CPU load is heavy.)
    // (Note: Main loop will call cg_listener->draw(), so draw() should only do non-Guichan drawing.)
    virtual void update() { }
    virtual void draw(Coercri::GfxContext &gc) { }
};

#endif

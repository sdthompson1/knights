/*
 * FILE:
 *   window.hpp
 *
 * PURPOSE:
 *   Class representing a Window. Windows can display graphics and
 *   receive keyboard and mouse input.
 *
 * AUTHOR:
 *   Stephen Thompson <stephen@solarflare.org.uk>
 *
 * COPYRIGHT:
 *   Copyright (C) Stephen Thompson, 2008 - 2024.
 *
 *   This file is part of the "Coercri" software library. Usage of "Coercri"
 *   is permitted under the terms of the Boost Software License, Version 1.0, 
 *   the text of which is displayed below.
 *
 *   Boost Software License - Version 1.0 - August 17th, 2003
 *
 *   Permission is hereby granted, free of charge, to any person or organization
 *   obtaining a copy of the software and accompanying documentation covered by
 *   this license (the "Software") to use, reproduce, display, distribute,
 *   execute, and transmit the Software, and to prepare derivative works of the
 *   Software, and to permit third-parties to whom the Software is furnished to
 *   do so, all subject to the following:
 *
 *   The copyright notices in the Software and this entire statement, including
 *   the above license grant, this restriction and the following disclaimer,
 *   must be included in all copies of the Software, in whole or in part, and
 *   all derivative works of the Software, unless such copies or derivative
 *   works are solely in the form of machine-executable object code generated by
 *   a source language processor.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 *   SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 *   FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *   DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef COERCRI_WINDOW_HPP
#define COERCRI_WINDOW_HPP

#include "rectangle.hpp"
#include "region.hpp"
#include "../core/coercri_error.hpp"

#include "boost/shared_ptr.hpp"
#include <vector>

namespace Coercri {

    class GfxContext;
    class PixelArray;
    class WindowListener;
    
    class Window {
    public:
        Window() : for_each_idx(NULL_FOR_EACH_IDX) { }
        virtual ~Window() { }

        //
        // Virtual functions
        //
        
        // get current window properties
        virtual void getSize(int &w, int &h) const = 0;
        
        // Query whether the window currently has focus
        virtual bool hasFocus() const = 0;

        // Bring the window to the front and give it focus (if the OS allows this).
        virtual void popToFront() = 0;
        
        // Show or hide the mouse pointer.
        virtual void showMousePointer(bool shown) = 0;

        // Capture the mouse. This allows mouse messages to be received even
        // if the mouse is outside the window
        virtual void captureMouse(bool captured) {
            // (subclasses will override if they implement this)
            throw CoercriError("captureMouse: not implemented");
        }
        
        // Switch between windowed and full screen
        virtual void switchToWindowed(int w, int h) = 0;
        virtual void switchToFullScreen() = 0;

        // Determine whether the window is currently maximized or in
        // full-screen mode
        virtual bool isMaximized() const = 0;
        virtual bool isFullScreen() const = 0;

        // Get a GfxContext for drawing.
        // Notes:
        // 1) only one GfxContext should exist at any given time
        // 2) a GfxContext should not exist across a call to GfxDriver::pollEvents.
        // 3) the window should not be destroyed while there is an outstanding GfxContext.
        virtual std::unique_ptr<GfxContext> createGfxContext() = 0;
        
        // Set the icon for this window. (Only some backends actually implement this.)
        virtual void setIcon(const PixelArray &) = 0;

        

        //
        // Non-virtual functions
        //
        
        // WindowListeners for this window
        void addWindowListener(WindowListener *);
        void rmWindowListener(WindowListener *);

        template<class Func>
        void forEachListener(Func func) {
            // take some care here because the listeners may remove
            // themselves (or other listeners) from the list during
            // the iteration.
            
            if (for_each_idx != NULL_FOR_EACH_IDX) {
                throw CoercriError("Recursive call to forEachWindowListener");
            }

            for (for_each_idx = 0; for_each_idx < int(listeners.size()); ++for_each_idx) {
                func(listeners[for_each_idx]);
            }

            for_each_idx = NULL_FOR_EACH_IDX;
        }

        // Invalid region handling
        void invalidateRectangle(const Rectangle &rect) { invalid_region.addRectangle(rect); }
        void invalidateRegion(const Region &region) { invalid_region.addRegion(region); }
        void invalidateAll();
        void cancelInvalidRegion() { invalid_region.clear(); }
        const Region & getInvalidRegion() const { return invalid_region; }
        bool needsRepaint() const { return !invalid_region.isEmpty(); }

        
    protected:
        std::vector<WindowListener*> listeners;
        int for_each_idx;
        enum { NULL_FOR_EACH_IDX = -999 };
        Region invalid_region;
        
    private:
        // prevent copying
        Window(const Window &);
        void operator=(const Window &);
    };

}

#endif

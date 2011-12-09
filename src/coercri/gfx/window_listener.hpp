/*
 * FILE:
 *   window_listener.hpp
 *
 * PURPOSE:
 *   Interface for window events: close, gain/lose focus,
 *   keyboard/mouse input, etc.
 *
 * AUTHOR:
 *   Stephen Thompson <stephen@solarflare.org.uk>
 *
 * COPYRIGHT:
 *   Copyright (C) Stephen Thompson, 2008 - 2009.
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

#ifndef COERCRI_WINDOW_LISTENER_HPP
#define COERCRI_WINDOW_LISTENER_HPP

#include "key_code.hpp"
#include "mouse_button.hpp"

namespace Coercri {

    class GfxContext;
    class Region;
    class Window;
    
    class WindowListener {
    public:

        virtual ~WindowListener() { }
        
        // User requested to close the window (e.g. by clicking "X" button)
        // Note: The window does not automatically close, the application has to do that
        // by destroying the Coercri::Window object.
        virtual void onClose() { }

        // Window gained or lost focus
        virtual void onGainFocus() { }
        virtual void onLoseFocus() { }

        // Window resized
        virtual void onResize(int new_width, int new_height) { }

        // Window minimized / un-minimized
        virtual void onActivate() { }
        virtual void onDeactivate() { }

        
        // Keyboard events

        // onRawKey. Used for "raw" key press/release events.
        // pressed = true for KEY DOWN event, false for KEY UP event.
        // rk = raw key code, see key_code.hpp.

        // NOTE: If window loses focus while a key is held, the "key
        // up" msg may not be generated. It is recommended that
        // applications clear any "key pressed" flags they may be
        // holding whenever the window loses focus.
        
        virtual void onRawKey(bool pressed, RawKey rk) { }

        
        // onCookedKey. Used for "cooked" keypresses such as <a>, <A>, <F1>, <Ctrl-c>, <Alt-Shift-F8>

        // ck = cooked key code, see key_code.hpp
        // ch = character (Unicode code point) in case of CK_CHARACTER.
        // mods = OR'd list of key modifiers.

        // Note that the character returned is "case sensitive", so,
        // for example, there is a difference between Ctrl-c and
        // Ctrl-Shift-C and Ctrl-C (the latter with caps lock on).

        // Note that CK_CHARACTER is only supposed to return printable characters.
        
        virtual void onCookedKey(CookedKey ck, int ch, KeyModifier mods) { }

        
        // Mouse button events
        virtual void onMouseDown(int x, int y, MouseButton button) { }
        virtual void onMouseUp(int x, int y, MouseButton button) { }

        // Mouse motion event
        virtual void onMouseMove(int new_x, int new_y) { }
    };

}

#endif

/*
 * FILE:
 *   cg_listener.cpp
 *
 * AUTHOR:
 *   Stephen Thompson
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

#include "cg_graphics.hpp"
#include "cg_input.hpp"
#include "cg_listener.hpp"
#include "../core/utf8string.hpp"
#include "../gfx/window.hpp"
#include "../timer/timer.hpp"

#include "guichan.hpp"

namespace Coercri {

    namespace {
        void SendKeyToGuichan(CGInput &input, gcn::Key k, KeyModifier mods)
        {
            // create two Guichan events: one pressed, and one released.
            gcn::KeyInput key_input(k, gcn::KeyInput::PRESSED);
            key_input.setAltPressed((mods & KM_ALT) != 0);
            key_input.setControlPressed((mods & KM_CONTROL) != 0);
            key_input.setShiftPressed((mods & KM_SHIFT) != 0);
            // note: we don't support guichan's "meta" modifer and "numeric pad" flag.
            input.addKeyInput(key_input);
            key_input.setType(gcn::KeyInput::RELEASED);
            input.addKeyInput(key_input);
        }
    }

    class CGListenerImpl {
    public:
        CGListenerImpl(boost::shared_ptr<Window> win, boost::shared_ptr<gcn::Gui> g, boost::shared_ptr<Timer> tmr)
            : window(win), gui(g), timer(tmr), input(tmr), gui_enabled(false),
              last_mouse_x(0), last_mouse_y(0)
        { }
        
        boost::shared_ptr<Window> window;
        boost::shared_ptr<gcn::Gui> gui;
        boost::shared_ptr<Timer> timer;
        
        CGInput input;
        CGGraphics graphics;
        
        bool gui_enabled;
        int last_mouse_x, last_mouse_y;
    };
    
    CGListener::CGListener(boost::shared_ptr<Window> window, boost::shared_ptr<gcn::Gui> gui, boost::shared_ptr<Timer> timer)
        : pimpl(new CGListenerImpl(window, gui, timer))
    {
        pimpl->gui->setInput(&pimpl->input);
        pimpl->gui->setGraphics(&pimpl->graphics);
    }

    void CGListener::enableGui()
    {
        if (pimpl->gui_enabled) return;
        pimpl->gui_enabled = true;

        // Window may have been resized while the GUI was disabled, so do a resize now.
        int w, h;
        pimpl->window->getSize(w, h);
        onResize(w, h);
    }

    void CGListener::disableGui()
    {
        pimpl->gui_enabled = false;
    }

    bool CGListener::isGuiEnabled() const
    {
        return pimpl->gui_enabled;
    }

    bool CGListener::processInput()
    {
        if (!pimpl->gui_enabled) return false;
        
        if (pimpl->input.inputWaiting()) {
            pimpl->gui->logic();
            pimpl->window->invalidateAll();
            return true;
        } else {
            return false;
        }
    }
    
    void CGListener::draw(GfxContext &gc)
    {
        if (!pimpl->gui_enabled) return;

        pimpl->gui->logic();
        
        pimpl->graphics.setTarget(&gc);
        pimpl->gui->draw();
        pimpl->graphics.setTarget(0);
    }

    void CGListener::onResize(int new_width, int new_height)
    {
        if (!pimpl->gui_enabled) return;
        
        if (pimpl->gui->getTop()) {
            pimpl->gui->getTop()->setSize(new_width, new_height);
            pimpl->window->invalidateAll();  // make sure it gets redrawn.
        }
    }

    void CGListener::onKey(KeyEventType type, KeyCode kc, KeyModifier mods)
    {
        // only process PRESSED and AUTO_REPEAT events here.
        if (type == KEY_RELEASED) return;

        // only process 'special' keys. (the normal "text" keys will be
        // processed by onTextInput.)

        // note: this does mean guichan will not get keypresses such as ctrl+c currently 
        // (because these are neither a special key nor do they generate a text event.)
        // but guichan doesn't do anything with such keypresses (afaik) so hopefully 
        // this won't matter.
       
        gcn::Key k = 0;
        switch (kc) {
        case KC_BACKSPACE: k = gcn::Key::BACKSPACE; break;
        case KC_DELETE: k = gcn::Key::DELETE; break;
        case KC_DOWN: k = gcn::Key::DOWN; break;
        case KC_END: k = gcn::Key::END; break;
        case KC_ESCAPE: k = gcn::Key::ESCAPE; break;
        case KC_F1: k = gcn::Key::F1; break;
        case KC_F2: k = gcn::Key::F2; break;
        case KC_F3: k = gcn::Key::F3; break;
        case KC_F4: k = gcn::Key::F4; break;
        case KC_F5: k = gcn::Key::F5; break;
        case KC_F6: k = gcn::Key::F6; break;
        case KC_F7: k = gcn::Key::F7; break;
        case KC_F8: k = gcn::Key::F8; break;
        case KC_F9: k = gcn::Key::F9; break;
        case KC_F10: k = gcn::Key::F10; break;
        case KC_F11: k = gcn::Key::F11; break;
        case KC_F12: k = gcn::Key::F12; break;
        case KC_F13: k = gcn::Key::F13; break;
        case KC_F14: k = gcn::Key::F14; break;
        case KC_F15: k = gcn::Key::F15; break;
        case KC_HOME: k = gcn::Key::HOME; break;
        case KC_INSERT: k = gcn::Key::INSERT; break;
        case KC_LEFT: k = gcn::Key::LEFT; break;
        case KC_LEFT_WINDOWS: k = gcn::Key::LEFT_SUPER; break;
        case KC_PAGE_DOWN: k = gcn::Key::PAGE_DOWN; break;
        case KC_PAGE_UP: k = gcn::Key::PAGE_UP; break;
        case KC_PAUSE: k = gcn::Key::PAUSE; break;
        case KC_PRINT_SCREEN: k = gcn::Key::PRINT_SCREEN; break;
        case KC_RETURN: case KC_KP_ENTER: k = gcn::Key::ENTER; break;
        case KC_RIGHT: k = gcn::Key::RIGHT; break;
        case KC_RIGHT_WINDOWS: k = gcn::Key::RIGHT_SUPER; break;
        case KC_TAB: k = gcn::Key::TAB; break;
        case KC_UP: k = gcn::Key::UP; break;
        default: return;    // Unknown key
        }

        SendKeyToGuichan(pimpl->input, k, mods);
    }

    void CGListener::onTextInput(const UTF8String &txt_in)
    {
        // realistically, guichan only works with Latin-1 chars at the moment.
        const std::string & txt = txt_in.asLatin1();
        
        for (std::string::const_iterator it = txt.begin(); it != txt.end(); ++it) {
            SendKeyToGuichan(pimpl->input,
                                gcn::Key(static_cast<unsigned char>(*it)),
                                KeyModifier(0));
        }
    }
    
    void CGListener::onMouseDown(int x, int y, MouseButton button)
    {
        if (!pimpl->gui_enabled) return;
        
        unsigned int b, t;
        switch (button) {
        case MB_LEFT:
            b = gcn::MouseInput::LEFT;
            t = gcn::MouseInput::PRESSED;
            break;
        case MB_RIGHT:
            b = gcn::MouseInput::RIGHT;
            t = gcn::MouseInput::PRESSED;
            break;
        case MB_MIDDLE:
            b = gcn::MouseInput::MIDDLE;
            t = gcn::MouseInput::PRESSED;
            break;
        case MB_WHEEL_UP:
            b = gcn::MouseInput::EMPTY;
            t = gcn::MouseInput::WHEEL_MOVED_UP;
            break;
        case MB_WHEEL_DOWN:
            b = gcn::MouseInput::EMPTY;
            t = gcn::MouseInput::WHEEL_MOVED_DOWN;
            break;
        default:
            return;
        }
        const int timestamp = pimpl->timer ? pimpl->timer->getMsec() : 0;

        pimpl->input.addMouseInput(gcn::MouseInput(b, t, x, y, timestamp));
        pimpl->last_mouse_x = x;
        pimpl->last_mouse_y = y;
    }

    void CGListener::onMouseUp(int x, int y, MouseButton button)
    {
        if (!pimpl->gui_enabled) return;
        
        unsigned int b;
        switch (button) {
        case MB_LEFT: b = gcn::MouseInput::LEFT; break;
        case MB_RIGHT: b = gcn::MouseInput::RIGHT; break;
        case MB_MIDDLE: b = gcn::MouseInput::MIDDLE; break;
        default: return;
        }
        const int timestamp = pimpl->timer ? pimpl->timer->getMsec() : 0;
        pimpl->input.addMouseInput(gcn::MouseInput(b, gcn::MouseInput::RELEASED, x, y, timestamp));
        pimpl->last_mouse_x = x;
        pimpl->last_mouse_y = y;
    }

    void CGListener::onMouseMove(int x, int y)
    {
        if (!pimpl->gui_enabled) return;
        
        const int timestamp = pimpl->timer ? pimpl->timer->getMsec() : 0;
        pimpl->input.addMouseInput(gcn::MouseInput(gcn::MouseInput::EMPTY, gcn::MouseInput::MOVED, x, y, timestamp));
        pimpl->last_mouse_x = x;
        pimpl->last_mouse_y = y;
    }

    void CGListener::repeatLastMouseInput()
    {
        onMouseMove(pimpl->last_mouse_x, pimpl->last_mouse_y);
    }
}

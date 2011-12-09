/*
 * FILE:
 *   sdl_window.cpp
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

#include "sdl_gfx_context.hpp"
#include "sdl_window.hpp"
#include "../../core/coercri_error.hpp"

#include "SDL.h"

#ifdef WIN32
#include "SDL_syswm.h"
#include <windows.h>
#endif

namespace Coercri {

    extern unsigned int g_sdl_required_flags;
    extern bool g_sdl_has_focus;

    SDLWindow::SDLWindow()
        : need_window_resize(false)
    {
        g_sdl_has_focus = true;
    }

    void SDLWindow::getSize(int &w, int &h) const
    {
        SDL_Surface *surf = SDL_GetVideoSurface();
        if (surf) {
            w = surf->w;
            h = surf->h;
        } else {
            w = h = 0;
        }
    }

    bool SDLWindow::hasFocus() const
    {
        return g_sdl_has_focus;
    }

    void SDLWindow::popToFront()
    {
#ifdef WIN32
        SDL_SysWMinfo wminfo;
        SDL_VERSION(&wminfo.version);
        if (SDL_GetWMInfo(&wminfo) != 1) {
            // error
        } else {
            const HWND hwnd = wminfo.window;

            // Restore it if it was minimized
            if (IsIconic(hwnd)) {
                ShowWindow(hwnd, SW_RESTORE);
            }

            // Focus the window / bring it to front (if possible)
            SetForegroundWindow(hwnd);
        }
#endif
    }
    
    void SDLWindow::showMousePointer(bool shown)
    {
        SDL_ShowCursor(shown ? SDL_ENABLE : SDL_DISABLE);
    }
    
    void SDLWindow::switchToWindowed(int w, int h)
    {
        bool currently_full_screen = ((g_sdl_required_flags & SDL_FULLSCREEN) != 0);
        g_sdl_required_flags &= (~SDL_FULLSCREEN);

        SDL_SetVideoMode(w, h, 0, g_sdl_required_flags);

        if (currently_full_screen) {
            // Need two calls to SetVideoMode to work around a bug when SDL is used with xmonad
            SDL_SetVideoMode(w, h, 0, g_sdl_required_flags);
        }
        
        need_window_resize = true;
    }

    void SDLWindow::switchToFullScreen(int w, int h)
    {
        g_sdl_required_flags |= SDL_FULLSCREEN;
        SDL_SetVideoMode(w, h, 0, g_sdl_required_flags);
        need_window_resize = true;
    }

    bool SDLWindow::isFullScreen() const
    {
        SDL_Surface *surf = SDL_GetVideoSurface();
        if (surf) {
            return ((surf->flags & SDL_FULLSCREEN) != 0);
        } else {
            return false;
        }
    }

    std::auto_ptr<GfxContext> SDLWindow::createGfxContext()
    {
        std::auto_ptr<GfxContext> p(new SDLGfxContext(*SDL_GetVideoSurface()));
        return p;
    }
}

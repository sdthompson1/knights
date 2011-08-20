/*
 * FILE:
 *   sdl_graphic.cpp
 *
 * AUTHOR:
 *   Stephen Thompson
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

#include "delete_sdl_surface.hpp"
#include "sdl_graphic.hpp"
#include "../../core/coercri_error.hpp"

namespace Coercri {

    SDLGraphic::SDLGraphic(boost::shared_ptr<const PixelArray> p, int hx_, int hy_)
        : pixels(p), hx(hx_), hy(hy_), need_reload(false)
    {
        if (!pixels) throw CoercriError("Null pixel_array used for graphic");
    }

    bool SDLGraphic::loadSurface() const
    {
        if (need_reload) {
            // Must reload the surface after it was lost
            const int result = SDL_LockSurface(surface.get());
            if (result != 0) {
                // Can't yet reload
                return false;
            } else {
                // Can reload. Delete the existing surface
                need_reload = false;
                SDL_UnlockSurface(surface.get());
                surface.reset();
            }
        }

        if (!surface) {
            boost::shared_ptr<SDL_Surface> temp(
               SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA,
                                    pixels->getWidth(),
                                    pixels->getHeight(),
                                    32,
                                    0xff000000,
                                    0xff0000,
                                    0xff00,
                                    0xff),
               DeleteSDLSurface());
            SDL_LockSurface(temp.get());
            for (int y = 0; y < temp->h; ++y) {
                for (int x = 0; x < temp->w; ++x) {
                    const Color& col = (*pixels)(x,y);
                    *((Uint32*)(temp->pixels) + y*temp->pitch/4 + x) = SDL_MapRGBA(temp->format, col.r, col.g, col.b, col.a);
                }
            }
            SDL_UnlockSurface(temp.get());
            surface.reset(SDL_DisplayFormatAlpha(temp.get()), DeleteSDLSurface());
        }
        return true;
    }
    
    void SDLGraphic::blit(SDL_Surface &dest, int x, int y) const
    {
        if (!loadSurface()) return;

        // Set up dest_rect (SDL ignores the width and height parts, we only need to set x & y)
        SDL_Rect dest_rect;
        dest_rect.x = x - hx;
        dest_rect.y = y - hy;

        // Do the blit
        const int result = SDL_BlitSurface(surface.get(), 0, &dest, &dest_rect);
        if (result == -2) {
            // This means the surface was lost
            need_reload = true;
        }
    }

    int SDLGraphic::getWidth() const
    {
        return pixels->getWidth();
    }

    int SDLGraphic::getHeight() const
    {
        return pixels->getHeight();
    }

    void SDLGraphic::getHandle(int &x, int &y) const
    {
        x = hx; y = hy;
    }

    boost::shared_ptr<const PixelArray> SDLGraphic::getPixels() const
    {
        return pixels;
    }
}

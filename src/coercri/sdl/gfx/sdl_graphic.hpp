/*
 * FILE:
 *   sdl_graphic.hpp
 *
 * PURPOSE:
 *   Implementation of Graphic for SDL
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

#ifndef COERCRI_SDL_GRAPHIC_HPP
#define COERCRI_SDL_GRAPHIC_HPP

#include "../../gfx/graphic.hpp"
#include "../../gfx/pixel_array.hpp"

#include "boost/shared_ptr.hpp"

#include <SDL2/SDL.h>

namespace Coercri {

    class SDLWindow;

    class SDLGraphic : public Graphic {
    public:
        SDLGraphic(boost::shared_ptr<const PixelArray> pixels, int hx, int hy);
        ~SDLGraphic();

        void blit(SDLWindow *window, SDL_Renderer *renderer, int x, int y) const;
        void blitModulated(SDLWindow *window, SDL_Renderer *renderer, int x, int y, Color col) const;

        // overridden from Graphic:
        int getWidth() const;
        int getHeight() const;
        void getHandle(int &x, int &y) const;
        boost::shared_ptr<const PixelArray> getPixels() const;

        // this is called when the SDLWindow that this Graphic is "using" is being destroyed.
        void notifyWindowDestroyed(SDLWindow *window) const;

    private:
        // create a cached SDL_Texture object for rendering onto the given window.
        // this is called internally by blit and blitModulated as needed.
        void createTexture(SDLWindow *window, SDL_Renderer *renderer) const;

    private:
        boost::shared_ptr<const PixelArray> pixels;
        int hx, hy;

        // cached values:
        mutable SDLWindow *used_window;
        mutable SDL_Renderer *used_renderer;
        mutable boost::shared_ptr<SDL_Texture> texture;  // Constructed with DeleteSDLTexture
    };

}

#endif

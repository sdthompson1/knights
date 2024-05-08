/*
 * FILE:
 *   sdl_gfx_context.cpp
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
#include "sdl_graphic.hpp"

#include "../../core/coercri_error.hpp"
#include "../../gfx/font.hpp"

namespace Coercri {

    SDLGfxContext::SDLGfxContext(SDL_Renderer *rend)
        : renderer(rend)
    {
        clearClipRectangle();
    }

    SDLGfxContext::~SDLGfxContext()
    {
        SDL_RenderPresent(renderer);
    }

    void SDLGfxContext::setClipRectangle(const Rectangle &rect)
    {
        int w = 0, h = 0;
        SDL_GetRendererOutputSize(renderer, &w, &h);
        clip_rectangle = IntersectRects(rect, Rectangle(0, 0, w, h));
        loadClipRectangle();
    }

    void SDLGfxContext::clearClipRectangle()
    {
        int w = 0, h = 0;
        SDL_GetRendererOutputSize(renderer, &w, &h);
        clip_rectangle = Rectangle(0, 0, w, h);
        loadClipRectangle();
    }

    void SDLGfxContext::loadClipRectangle()
    {
        SDL_Rect sdl_rect;
        sdl_rect.x = clip_rectangle.getLeft();
        sdl_rect.y = clip_rectangle.getTop();
        sdl_rect.w = clip_rectangle.getWidth();
        sdl_rect.h = clip_rectangle.getHeight();
        SDL_RenderSetClipRect(renderer, &sdl_rect);
    }

    Rectangle SDLGfxContext::getClipRectangle() const
    {
        return clip_rectangle;
    }
        
    int SDLGfxContext::getWidth() const
    {
        int w = 0, h = 0;
        SDL_GetRendererOutputSize(renderer, &w, &h);
        return w;
    }

    int SDLGfxContext::getHeight() const
    {
        int w = 0, h = 0;
        SDL_GetRendererOutputSize(renderer, &w, &h);
        return h;
    }
    
    void SDLGfxContext::clearScreen(Color colour)
    {
        SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, colour.a);
        SDL_RenderClear(renderer);
    }

    void SDLGfxContext::plotPixel(int x, int y, Color colour)
    {
        SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, colour.a);
        SDL_RenderDrawPoint(renderer, x, y);
    }

    void SDLGfxContext::drawGraphic(int x, int y, const Graphic &graphic)
    {
        const SDLGraphic *sdl_graphic = dynamic_cast<const SDLGraphic*>(&graphic);
        if (sdl_graphic) {
            sdl_graphic->blit(renderer, x, y);
        }
    }

    void SDLGfxContext::drawLine(int x1, int y1, int x2, int y2, Color col)
    {
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    }

    void SDLGfxContext::drawRectangle(const Rectangle &rect, Color col)
    {
        SDL_Rect sdl_rect;
        sdl_rect.x = rect.getLeft();
        sdl_rect.y = rect.getTop();
        sdl_rect.w = rect.getWidth();
        sdl_rect.h = rect.getHeight();

        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
        SDL_RenderDrawRect(renderer, &sdl_rect);
    }

    void SDLGfxContext::fillRectangle(const Rectangle &rect, Color col)
    {
        SDL_Rect sdl_rect;
        sdl_rect.x = rect.getLeft();
        sdl_rect.y = rect.getTop();
        sdl_rect.w = rect.getWidth();
        sdl_rect.h = rect.getHeight();

        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
        SDL_RenderFillRect(renderer, &sdl_rect);
    }

    boost::shared_ptr<PixelArray> SDLGfxContext::takeScreenshot()
    {
        throw CoercriError("SDLGfxContext::takeScreenshot not implemented, sorry");
    }
}

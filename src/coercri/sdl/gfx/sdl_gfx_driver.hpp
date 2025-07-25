/*
 * FILE:
 *   sdl_gfx_driver.hpp
 *
 * PURPOSE:
 *   SDL implementation of GfxDriver
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

#ifndef COERCRI_SDL_GFX_DRIVER_HPP
#define COERCRI_SDL_GFX_DRIVER_HPP

#include "../../gfx/gfx_driver.hpp"
#include "sdl_window.hpp"
#include "../core/sdl_subsystem_handle.hpp"

namespace Coercri {

    class SDLGfxDriver : public GfxDriver {
    public:
        // Constructor
        SDLGfxDriver();

        // Functions overridden from GfxDriver
        virtual boost::shared_ptr<Window> createWindow(const WindowParams &params) override;
        virtual boost::shared_ptr<Graphic> createGraphic(boost::shared_ptr<const PixelArray> pixels, int hx = 0, int hy = 0) override;
        virtual bool pollEvents() override;
        virtual bool waitEventMsec(int ms) override;
        
    private:
        SDLSubSystemHandle video_subsystem;
        std::vector<boost::weak_ptr<SDLWindow> > windows;
    };
    
}

#endif

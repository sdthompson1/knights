/*
 * FILE:
 *   bitmap_font.hpp
 *
 * PURPOSE:
 *   Simple class for drawing bitmap fonts.
 *
 * AUTHOR:
 *   Stephen Thompson <stephen@solarflare.org.uk>
 *
 * COPYRIGHT:
 *   Copyright (C) Stephen Thompson, 2008 - 2013.
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

#ifndef COERCRI_BITMAP_FONT_HPP
#define COERCRI_BITMAP_FONT_HPP

#include "font.hpp"

#include "boost/noncopyable.hpp"
#include "boost/shared_ptr.hpp"

#ifdef _MSC_VER
#pragma warning (disable:4200)
#endif

namespace Coercri {

    class KernTable;
    class PixelArray;
    
    class BitmapFont : public Font, boost::noncopyable {
    public:
        // Updated 21-May-2012.

        // Old constructor, imports SFont-style bitmap and interprets
        // it as a "two colour" font.
        // 
        // The alpha channel in the PixelArray is ignored. For the
        // colour channels:
        //
        // 1) The top row should contain colours (255,0,255) and
        // (0,0,0), these are the SFont markers defining the extents
        // of each character.
        //
        // 2) The rest of the image is interpreted as a
        // black-and-white image of the font (black = "transparent"
        // pixels, anything other than black = "opaque" pixels).
        //
        // There is no kerning when this ctor is used.
        //
        explicit BitmapFont(boost::shared_ptr<PixelArray> pix);

        
        // New constructor. Ctor takes a kern table and the (uniform)
        // height of each character, and the setupCharacter() and
        // plotPixel() methods are used to define the actual
        // characters. (setupCharacter "creates" a character and
        // initializes alpha to zero; plotPixel sets the alpha for one
        // pixel at a time.)
        //
        // Note char 32 is a special case, it is always completely
        // blank and plotPixel has no effect for this character.
        //
        // Note plotPixel() is harmless (and does nothing) if called
        // with "out of range" x,y values. However, it crashes if
        // called on a "c" that hasn't been set up by setupChar.
        //
        BitmapFont(boost::shared_ptr<KernTable> kern_table, int height_);
        void setupCharacter(char c, int width, int height, int xofs, int yofs, int xadvance);
        void plotPixel(char c, int x, int y, unsigned char alpha);

        
        // destructor
        ~BitmapFont();

        
        // Overridden from Font:
        void getTextSize(const std::string &text, int &w, int &h) const;
        int getTextHeight() const;
        void drawText(GfxContext &dest, int x, int y, const std::string &text, Color col) const;

    private:
        struct Character {
            int width, height;  // width and height of pixels[] array
            int xofs, yofs;     // offset from "pen position" (top left of character) to top left of pixels[] array.
            int xadvance;       // amount to move right after drawing this character.
            unsigned char pixels[];      // Variable sized array, row major, contains alpha values from 0-255.
        };

    private:
        Character * characters[256];
        boost::shared_ptr<KernTable> kern_table;
        int text_height;
    };
}

#endif

/*
 * FILE:
 *   cg_font.hpp
 *
 * PURPOSE:
 *   Coercri implementation of gcn::Font
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

#ifndef COERCRI_CG_FONT_HPP
#define COERCRI_CG_FONT_HPP

#include "../gfx/color.hpp"
#include "guichan.hpp"
#include "boost/shared_ptr.hpp"

namespace Coercri {

    class Font;

    class CGFont : public gcn::Font {
    public:
        // f must be non-null
        explicit CGFont(boost::shared_ptr<Coercri::Font> f)
            : font(f) { }

        // overridden from gcn::Font
        // NOTE: Guichan uses Latin-1 encoding, so all string
        // arguments are interpreted as Latin-1 here.
        virtual void drawString(gcn::Graphics *graphics, const std::string &text_latin1, int x, int y);
        virtual int getHeight() const;
        virtual int getWidth(const std::string &text_latin1) const;

    private:
        boost::shared_ptr<Coercri::Font> font;
    };

}

#endif

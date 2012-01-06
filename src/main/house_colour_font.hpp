/*
 * house_colour_font.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2012.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Knights is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Knights.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * Abuse of gcn::Font to draw the little coloured squares for the knight house colours.
 *
 */ 

#ifndef HOUSE_COLOUR_FONT_HPP
#define HOUSE_COLOUR_FONT_HPP

#include "gfx/color.hpp"
#include "guichan.hpp"

std::string ColToText(const Coercri::Color &c);
gcn::Color TextToCol(const std::string &x);

class HouseColourFont : public gcn::Font {
public:
    explicit HouseColourFont(gcn::Font &base_font_, int hc_w, int hc_h)
        : base_font(base_font_), house_col_width(hc_w), house_col_height(hc_h) { }
    virtual void drawString(gcn::Graphics *graphics, const std::string &text, int x, int y);
    int getHeight() const { return std::max(base_font.getHeight(), house_col_height); }
    int getWidth(const std::string &) const;
private:
    void drawBox(gcn::Graphics *graphics, int x, int y, const gcn::Color &col) const;
    
    gcn::Font & base_font;
    int house_col_width, house_col_height;
};

#endif

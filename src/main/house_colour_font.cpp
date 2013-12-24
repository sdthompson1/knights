/*
 * house_colour_font.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2013.
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

#include "misc.hpp"

#include "house_colour_font.hpp"
#include "round.hpp"

std::string ColToText(const Coercri::Color &c)
{
    std::string x;
    x += '\001';  // special code meaning "house colour follows"
    x += (unsigned char)(c.r);
    x += (unsigned char)(c.g);
    x += (unsigned char)(c.b);
    return x;
}

gcn::Color TextToCol(const std::string &x)
{
    return gcn::Color((unsigned char)x[0], (unsigned char)x[1], (unsigned char)x[2]);
}

int HouseColourFont::getWidth(const std::string &x) const
{
    // We assume that a house colour square will always be at the beginning or end
    if (x.length() >= 4) {
        if (x[x.length()-4] == 1) {
            return base_font.getWidth(x.substr(0, x.length()-4)) + house_col_width;
        } else if (x[0] == 1) {
            return base_font.getWidth(x.substr(4)) + house_col_width;
        }
    }

    return base_font.getWidth(x);
}

void HouseColourFont::drawString(gcn::Graphics *graphics, const std::string &text, int x, int y)
{
    if (!graphics) return;
    
    std::string to_print;
    gcn::Color col;
    bool use_col = false;
    bool col_begin = false;
    
    if (text.length() >= 4) {
        if (text[text.length()-4] == 1) {
            to_print = text.substr(0, text.length()-4);
            col = TextToCol(text.substr(text.length()-3, 3));
            use_col = true;
            col_begin = false;
        } else if (text[0] == 1) {
            to_print = text.substr(4);
            col = TextToCol(text.substr(1, 3));
            use_col = true;
            col_begin = true;
        }
    }
    if (!use_col) {
        to_print = text;
    }

    if (use_col && col_begin) {
        drawBox(graphics, x, y, col);
        x += house_col_width;
    }
    base_font.drawString(graphics, to_print, x, y);
    if (use_col && !col_begin) {
        x += base_font.getWidth(to_print);
        drawBox(graphics, x, y, col);
    }
}

void HouseColourFont::drawBox(gcn::Graphics *graphics, int x, int y, const gcn::Color &col) const
{
    const int excess_height = base_font.getHeight() - house_col_height;
    const int offset = excess_height > 0 ? Round(excess_height*0.55f) : 0;
    y += offset;
        
    const int w = house_col_width, h = house_col_height;
    gcn::Color old_col = graphics->getColor();
    graphics->setColor(gcn::Color(0,0,0));
    graphics->drawLine(x+1,   y+1,   x+w-2, y+1  );
    graphics->drawLine(x+w-2, y+1,   x+w-2, y+h-2);
    graphics->drawLine(x+w-2, y+h-2, x+1,   y+h-2);
    graphics->drawLine(x+1,   y+h-2, x+1,   y+1);
    graphics->setColor(col);
    graphics->fillRectangle(gcn::Rectangle(x+2, y+2, w-4, h-4));
    graphics->setColor(old_col);
    x += w;
}

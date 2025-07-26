/*
 * gui_draw_box.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2025.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

#include "gui_draw_box.hpp"

void GuiDrawBox(gcn::Graphics *graphics, const BoxCol &col, int left, int top, int width, int height)
{
    for (int y = 0; y < height; ++y) {
        const float lambda = float(y) / float(height-1);
        const int r = int(col.rtop + lambda*(col.rbot - col.rtop));
        const int g = int(col.gtop + lambda*(col.gbot - col.gtop));
        const int b = int(col.btop + lambda*(col.bbot - col.btop));
        graphics->setColor(gcn::Color(r, g, b));

        if (y > 1 && y < height-2) {
            graphics->drawLine(left+1, top+y, left+width-2, top+y);
        } else if (y == 1 || y == height-2) {
            graphics->drawLine(left+2, top+y, left+width-3, top+y);
        }

        // calculate border colour by multiplication
        const float multiple = 0.85f - 0.15f * lambda;
        graphics->setColor(gcn::Color(int(r*multiple), int(g*multiple), int(b*multiple)));

        if (y == 0 || y == height-1) {
            graphics->drawLine(left+2, top+y, left+width-3, top+y);
        } else if (y == 1 || y == height-2) {
            graphics->drawPoint(left+1, top+y);
            graphics->drawPoint(left+width-2, top+y);
        } else {
            graphics->drawPoint(left, top+y);
            graphics->drawPoint(left+width-1, top+y);
        }
    }
}


namespace {
    const gcn::Color mid(0x66, 0x66, 0x44);
    const gcn::Color bright = mid + gcn::Color(0x28, 0x28, 0x14);
    const gcn::Color dark = mid - gcn::Color(0x28, 0x28, 0x14);

    gcn::Color GetCol1(int x, int w)
    {
        const float lambda = float(x)/float(2*w);
        return bright*(1-lambda) + dark*lambda;
    }

    gcn::Color GetCol2(int x, int w)
    {
        const float lambda = float(x+w)/float(2*w);
        return bright*(1-lambda) + dark*lambda;
    }
}

void GuiDrawBox2(gcn::Graphics *graphics,
                 int x0, int y0, int w, int h)
{
    for (int x = 0; x < w; x += 2) {
        // top row
        graphics->setColor(GetCol1(x, w));
        graphics->drawPoint(x+x0, y0);
        graphics->drawPoint(x+x0, y0+1);
        if (x+1 < w) {
            graphics->drawPoint(x+1+x0, y0);
            graphics->drawPoint(x+1+x0, y0+1);
        }
        // bottom row
        graphics->setColor(GetCol2(x, w));
        graphics->drawPoint(x+x0, y0+h-1);
        graphics->drawPoint(x+x0, y0+h-2);
        if (x+1 < w) {
            graphics->drawPoint(x+1+x0, y0+h-1);
            graphics->drawPoint(x+1+x0, y0+h-2);
        }
    }

    for (int y = 2; y < h-2; y+=2) {
        // left side
        graphics->setColor(GetCol1(y, h));
        graphics->drawPoint(x0, y0+y);
        graphics->drawPoint(x0+1, y0+y);
        if (y + 1 < h-2) {
            graphics->drawPoint(x0, y0+y+1);
            graphics->drawPoint(x0+1, y0+y+1);
        }
        // right side
        graphics->setColor(GetCol2(y, h));
        graphics->drawPoint(x0+w-2, y0+y);
        graphics->drawPoint(x0+w-1, y0+y);
        if (y + 1 < h-2) {
            graphics->drawPoint(x0+w-2, y0+y+1);
            graphics->drawPoint(x0+w-1, y0+y+1);
        }
    }

    // draw the inside part
    graphics->setColor(mid);
    graphics->fillRectangle(gcn::Rectangle(x0+2, y0+2, w-4, h-4));
}

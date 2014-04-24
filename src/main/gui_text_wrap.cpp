/*
 * gui_text_wrap.cpp
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

#include "gui_text_wrap.hpp"
#include "text_formatter.hpp"

namespace {
    const int pad = 2;
    
    struct ActualPrinter : Printer {
        ActualPrinter(int w, gcn::Font &f, gcn::Graphics *g) : width(w), font(f), graphics(g) { }

        int getTextWidth(const std::string &t_latin1) { return font.getWidth(t_latin1) + 2*pad; }
        int getTextHeight() { return font.getHeight(); }

        void printLine(const std::string &text_latin1, int y, bool do_centre)
        {
            if (graphics) {
                if (do_centre) {
                    graphics->drawText(text_latin1, width/2, y, gcn::Graphics::CENTER);
                } else {
                    graphics->drawText(text_latin1, pad, y);
                }
            }
        }

        int width;
        gcn::Font &font;
        gcn::Graphics *graphics;
    };
}

GuiTextWrap::GuiTextWrap()
    : opaque(false), centred(false), rich(false)
{
    addMouseListener(this);
}

void GuiTextWrap::draw(gcn::Graphics *graphics)
{
    if (isOpaque()) {
        // Clear the background first.
        graphics->setColor(getBackgroundColor());
        graphics->fillRectangle(gcn::Rectangle(0, 0, getWidth(), getHeight()));
    }
    
    graphics->setFont(getFont());
    graphics->setColor(getForegroundColor());

    ActualPrinter p(getWidth(), *getFont(), graphics);
    TextFormatter formatter(p, getWidth(), rich);
    formatter.printString(text);
}

void GuiTextWrap::adjustHeight()
{
    ActualPrinter p(getWidth(), *getFont(), 0);
    TextFormatter formatter(p, getWidth(), rich);
    setHeight(formatter.printString(text));
}

void GuiTextWrap::mouseReleased(gcn::MouseEvent &mouseEvent)
{
    distributeActionEvent();
}

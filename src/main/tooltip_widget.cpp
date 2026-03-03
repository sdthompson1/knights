/*
 * tooltip_widget.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2026.
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

#include "tooltip_widget.hpp"
#include "gui_draw_box.hpp"
#include "text_formatter.hpp"

namespace {
    const int PAD = 6;
    const int SHOW_DELAY_MS = 500;
    const BoxCol TOOLTIP_BOXCOL = { 255, 255, 220, 240, 240, 180 };

    struct TooltipPrinter : Printer {
        TooltipPrinter(gcn::Font &f, gcn::Graphics *g, int x_off, int y_off)
            : font(f), graphics(g), x_off(x_off), y_off(y_off), max_line_w(0) {}
        int getTextWidth(const Coercri::UTF8String &t) { return font.getWidth(t.asUTF8()); }
        int getTextHeight() { return font.getHeight(); }
        void printLine(const Coercri::UTF8String &t, int y, bool centre) {
            int w = font.getWidth(t.asUTF8());
            if (w > max_line_w) max_line_w = w;
            if (graphics) graphics->drawText(t.asUTF8(), x_off, y_off + y);
        }
        gcn::Font &font;
        gcn::Graphics *graphics;
        int x_off, y_off;
        int max_line_w;
    };
}

TooltipWidget::TooltipWidget(const Coercri::UTF8String &text_, int max_width, Coercri::Timer &timer_, Coercri::Window &window_)
    : text(text_), timer(timer_), window(window_), pending_show(false), show_start_msec(0)
{
    // Single dry-run: wrap at max_width, track the longest printed line.
    // After wrapping, every line fits within max_line_w, so shrinking the
    // widget to that width produces identical line breaks on the real draw.
    TooltipPrinter p(*getFont(), nullptr, PAD, PAD);
    TextFormatter formatter(p, max_width - 2*PAD, false);
    int text_h = formatter.printString(text);

    setSize(p.max_line_w + 2*PAD, text_h + 2*PAD);
    setVisible(false);
}

void TooltipWidget::draw(gcn::Graphics *graphics)
{
    GuiDrawBox(graphics, TOOLTIP_BOXCOL, 0, 0, getWidth(), getHeight());
    graphics->setFont(getFont());
    graphics->setColor(getForegroundColor());
    TooltipPrinter p(*getFont(), graphics, PAD, PAD);
    TextFormatter formatter(p, getWidth() - 2*PAD, false);
    formatter.printString(text);
}

void TooltipWidget::logic()
{
    if (pending_show && timer.getMsec() - show_start_msec >= (unsigned int)SHOW_DELAY_MS) {
        pending_show = false;
        setVisible(true);
        window.invalidateAll();
    }
}

void TooltipWidget::scheduleShow()
{
    pending_show = true;
    show_start_msec = timer.getMsec();
    requestMoveToTop();
}

void TooltipWidget::cancelShow()
{
    pending_show = false;
    setVisible(false);
}

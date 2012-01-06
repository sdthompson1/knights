/*
 * gui_centre.cpp
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

#include "gui_centre.hpp"

GuiCentre::GuiCentre(gcn::Widget *c)
    : gcn::ScrollArea(c), resize_in_progress(false)
{
    setBackgroundColor(gcn::Color(0,0,0));
    setScrollbarWidth(15);
    widgetResized(gcn::Event(0));
    addWidgetListener(this);
}

void GuiCentre::widgetResized(const gcn::Event &event)
{
    if (resize_in_progress) return;
    resize_in_progress = true;

    gcn::Widget *contained = getContent();
    if (contained) {

        const int win_w = getWidth(), win_h = getHeight();
        const int con_w = contained->getWidth(), con_h = contained->getHeight();

        const bool naive_horiz_scroll = con_w > win_w;
        const bool naive_vert_scroll = con_h > win_h;
        const bool horiz_scroll = naive_horiz_scroll || (naive_vert_scroll && con_w > win_w - getScrollbarWidth());
        const bool vert_scroll = naive_vert_scroll || (naive_horiz_scroll && con_h > win_h - getScrollbarWidth());

        const int avail_w = vert_scroll ? win_w - getScrollbarWidth() : win_w;
        const int avail_h = horiz_scroll ? win_h - getScrollbarWidth() : win_h;

        const int x = std::max(0, (avail_w - con_w) / 2);
        const int y = std::max(0, (avail_h - con_h) / 2);
        
        setDimension(gcn::Rectangle(x, y, win_w - x, win_h - y));
        logic();
    }

    resize_in_progress = false;
}

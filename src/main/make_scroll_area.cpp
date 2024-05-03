/*
 * make_scroll_area.cpp
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

#include "make_scroll_area.hpp"

std::unique_ptr<gcn::ScrollArea> MakeScrollArea(gcn::Widget &content, int width, int height, int scrollbar_width)
{
    std::unique_ptr<gcn::ScrollArea> scroll_area(new gcn::ScrollArea);
    scroll_area->setContent(&content);
    scroll_area->setBackgroundColor(gcn::Color(255,255,255));
    scroll_area->setOpaque(true);
    scroll_area->setSize(width, height);
    scroll_area->setScrollbarWidth(scrollbar_width);
    scroll_area->setFrameSize(1);

    return scroll_area;
}

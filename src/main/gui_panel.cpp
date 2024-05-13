/*
 * gui_panel.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2024.
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
#include "gui_panel.hpp"

GuiPanel::GuiPanel(gcn::Widget *c)
    : GuiSimpleContainer(c)
{
    const int DEFAULT_BORDER = 3;
    
    if (c) {
        setSize(c->getWidth() + 2*DEFAULT_BORDER, c->getHeight() + 2*DEFAULT_BORDER);
        c->setPosition(DEFAULT_BORDER, DEFAULT_BORDER);
    }
}

void GuiPanel::draw(gcn::Graphics *graphics)
{
    const BoxCol PANEL_BOXCOL = { 240, 240, 240, 220, 220, 220 };
    
    GuiDrawBox(graphics, PANEL_BOXCOL, 0, 0, getWidth(), getHeight());
    drawChildren(graphics);
}

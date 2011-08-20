/*
 * gui_button.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
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

#include "gui_button.hpp"
#include "gui_draw_box.hpp"

void GuiButton::draw(gcn::Graphics *graphics)
{
    const BoxCol BUTTON_BOXCOL = { 250, 250, 150, 210, 180, 100 };
    const BoxCol BUTTON_BOXCOL_PRESSED = { 210, 180, 100, 210, 180, 100 };
    
    if (isPressed()) {
        GuiDrawBox(graphics, BUTTON_BOXCOL_PRESSED, 0, 0, getWidth(), getHeight());
    } else {
        GuiDrawBox(graphics, BUTTON_BOXCOL, 0, 0, getWidth(), getHeight());
    }
    
    int textX = 0;
    int textY = getHeight() / 2 - getFont()->getHeight() / 2;
    
    switch (getAlignment()) {
    case gcn::Graphics::LEFT: textX = getSpacing(); break;
    case gcn::Graphics::CENTER: textX = getWidth() / 2; break;
    case gcn::Graphics::RIGHT: textX = getWidth() - getSpacing(); break;
    }
    
    graphics->setFont(getFont());
    graphics->setColor(getForegroundColor());
    if (isPressed()) {
        graphics->drawText(getCaption(), textX+1, textY+1, getAlignment());
    } else {
        graphics->drawText(getCaption(), textX, textY, getAlignment());
    }
}

/*
 * gui_numeric_field.cpp
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

#include "gui_numeric_field.hpp"

void GuiNumericField::keyPressed(gcn::KeyEvent &key_event)
{
    const std::string old_text = getText();
    
    if (key_event.getKey().isCharacter() && !key_event.getKey().isNumber()) {
        // reject the key (non-numeric character)
        key_event.consume();
    } else if (key_event.getKey().isNumber() && old_text.length() >= max_digits) {
        // reject the key (will make the text longer than 2 chars)
        key_event.consume();
    } else {
        // let the parent class handle it
        gcn::TextField::keyPressed(key_event);
    }

    // If the text has changed then fire an action event.
    if (old_text != getText()) {
        distributeActionEvent();
    }
}

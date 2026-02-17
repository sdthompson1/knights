/*
 * gui_numeric_field.cpp
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

#include "gui_numeric_field.hpp"

#include "core/utf8string.hpp"

void GuiNumericField::keyPressed(gcn::KeyEvent &key_event)
{
    const std::string old_text = getText();

    // Let the parent class handle it (arrow keys, backspace, delete, home, end, etc.)
    gcn::TextField::keyPressed(key_event);

    // If the text has changed then fire an action event.
    if (old_text != getText()) {
        distributeActionEvent();
    }
}

void GuiNumericField::receiveTextInput(const Coercri::UTF8String &text)
{
    const std::string &utf8_str = text.asUTF8();

    // Filter to only digit characters, respecting max_digits
    std::string filtered;
    for (char c : utf8_str) {
        if (c >= '0' && c <= '9' && mText.length() + filtered.length() < static_cast<size_t>(max_digits)) {
            filtered += c;
        }
    }

    if (filtered.empty()) {
        return;
    }

    // Insert at the current caret position
    mText.insert(mCaretPosition, filtered);
    mCaretPosition += filtered.length();
    fixScroll();

    // Fire an action event since the field contents changed
    distributeActionEvent();
}

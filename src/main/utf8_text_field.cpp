/*
 * utf8_text_field.cpp
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

#include "misc.hpp"

#include "utf8_text_field.hpp"

#include "core/utf8string.hpp"
#include "external/utf8.h"

#include <string>

void UTF8TextField::keyPressed(gcn::KeyEvent &keyEvent)
{
    gcn::Key key = keyEvent.getKey();

    // Handle arrow keys - these need to move by UTF-8 characters, not bytes
    if (key.getValue() == gcn::Key::LEFT && mCaretPosition > 0)
    {
        // Move back one UTF-8 character
        std::string::iterator it = mText.begin() + mCaretPosition;
        try {
            utf8::prior(it, mText.begin());
            mCaretPosition = it - mText.begin();
        } catch (...) {
            // If UTF-8 is invalid, just move back one byte
            --mCaretPosition;
        }
        keyEvent.consume();
        fixScroll();
        return;
    }

    else if (key.getValue() == gcn::Key::RIGHT && mCaretPosition < mText.size())
    {
        // Move forward one UTF-8 character
        std::string::iterator it = mText.begin() + mCaretPosition;
        try {
            utf8::next(it, mText.end());
            mCaretPosition = it - mText.begin();
        } catch (...) {
            // If UTF-8 is invalid, just move forward one byte
            ++mCaretPosition;
        }
        keyEvent.consume();
        fixScroll();
        return;
    }

    // Handle DELETE key - delete one UTF-8 character forward
    else if (key.getValue() == gcn::Key::DELETE && mCaretPosition < mText.size())
    {
        std::string::iterator it = mText.begin() + mCaretPosition;
        std::string::iterator start_it = it;
        try {
            utf8::next(it, mText.end());
            size_t char_len = it - start_it;
            mText.erase(mCaretPosition, char_len);
        } catch (...) {
            // If UTF-8 is invalid, just delete one byte
            mText.erase(mCaretPosition, 1);
        }
        keyEvent.consume();
        fixScroll();
        return;
    }

    // Handle BACKSPACE key - delete one UTF-8 character backward
    else if (key.getValue() == gcn::Key::BACKSPACE && mCaretPosition > 0)
    {
        std::string::iterator it = mText.begin() + mCaretPosition;
        std::string::iterator end_it = it;
        try {
            utf8::prior(it, mText.begin());
            size_t char_len = end_it - it;
            size_t new_pos = it - mText.begin();
            mText.erase(new_pos, char_len);
            mCaretPosition = new_pos;
        } catch (...) {
            // If UTF-8 is invalid, just delete one byte
            mText.erase(mCaretPosition - 1, 1);
            --mCaretPosition;
        }
        keyEvent.consume();
        fixScroll();
        return;
    }

    // For all other keys, use the default TextField behavior
    // (HOME, END, ENTER, character insertion all work fine with byte positions)
    else
    {
        gcn::TextField::keyPressed(keyEvent);
    }
}

void UTF8TextField::receiveTextInput(const Coercri::UTF8String &text)
{
    const std::string &utf8_str = text.asUTF8();

    if (utf8_str.empty()) {
        return;
    }

    // Insert the UTF-8 text at the current caret position
    mText.insert(mCaretPosition, utf8_str);

    // Advance caret by the number of bytes inserted
    mCaretPosition += utf8_str.length();

    // Ensure caret is visible
    fixScroll();
}

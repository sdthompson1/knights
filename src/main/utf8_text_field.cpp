/*
 * utf8_text_field.cpp
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

#include "misc.hpp"

#include "utf8_text_field.hpp"

#include "core/utf8string.hpp"
#include "external/utf8.h"

// Note: we should probably abstract away the clipboard handling via some
// method(s) on Coercri::GfxDriver, but for now accessing SDL directly
// should be OK - it is only a couple of calls after all.
#include <SDL2/SDL.h>

#include <algorithm>
#include <string>


//
// Private helpers
//

unsigned int UTF8TextField::selectionLeft() const
{
    return std::min(mSelectionStart, mCaretPosition);
}

unsigned int UTF8TextField::selectionRight() const
{
    return std::max(mSelectionStart, mCaretPosition);
}

std::string UTF8TextField::getSelectedText() const
{
    return mText.substr(selectionLeft(), selectionRight() - selectionLeft());
}

void UTF8TextField::deleteSelectedText()
{
    unsigned int left = selectionLeft();
    unsigned int right = selectionRight();
    mText.erase(left, right - left);
    mCaretPosition = left;
    mHasSelection = false;
}

void UTF8TextField::clearSelection()
{
    mHasSelection = false;
}

void UTF8TextField::startSelectionIfNeeded()
{
    if (!mHasSelection) {
        mSelectionStart = mCaretPosition;
        mHasSelection = true;
    }
}

// Heuristic word character test: ASCII alnum/underscore counts as a word char;
// non-ASCII code points (>127) are also treated as word chars so that runs of
// accented or CJK characters clump together as one word rather than each being
// treated as punctuation.
static bool isWordCodePoint(uint32_t cp)
{
    if (cp > 127) return true;
    return std::isalnum(static_cast<unsigned char>(cp)) || cp == '_';
}

void UTF8TextField::moveWordLeft()
{
    auto it = mText.begin() + mCaretPosition;
    // Skip non-word characters backward
    while (it != mText.begin()) {
        auto saved = it;
        try {
            uint32_t cp = utf8::prior(it, mText.begin());
            if (isWordCodePoint(cp)) { it = saved; break; }
        } catch (...) { --it; }
    }
    // Skip word characters backward
    while (it != mText.begin()) {
        auto saved = it;
        try {
            uint32_t cp = utf8::prior(it, mText.begin());
            if (!isWordCodePoint(cp)) { it = saved; break; }
        } catch (...) { --it; }
    }
    mCaretPosition = it - mText.begin();
}

void UTF8TextField::moveWordRight()
{
    auto it = mText.begin() + mCaretPosition;
    // Skip non-word characters forward
    while (it != mText.end()) {
        auto saved = it;
        try {
            uint32_t cp = utf8::next(it, mText.end());
            if (isWordCodePoint(cp)) { it = saved; break; }
        } catch (...) { ++it; }
    }
    // Skip word characters forward
    while (it != mText.end()) {
        auto saved = it;
        try {
            uint32_t cp = utf8::next(it, mText.end());
            if (!isWordCodePoint(cp)) { it = saved; break; }
        } catch (...) { ++it; }
    }
    mCaretPosition = it - mText.begin();
}


//
// draw override: same as base class but with selection highlight
//

void UTF8TextField::draw(gcn::Graphics* graphics)
{
    gcn::Color faceColor = getBaseColor();
    gcn::Color highlightColor, shadowColor;
    int alpha = getBaseColor().a;
    highlightColor = faceColor + 0x303030;
    highlightColor.a = alpha;
    shadowColor = faceColor - 0x303030;
    shadowColor.a = alpha;

    // Draw border
    graphics->setColor(shadowColor);
    graphics->drawLine(0, 0, getWidth() - 1, 0);
    graphics->drawLine(0, 1, 0, getHeight() - 2);
    graphics->setColor(highlightColor);
    graphics->drawLine(getWidth() - 1, 1, getWidth() - 1, getHeight() - 1);
    graphics->drawLine(0, getHeight() - 1, getWidth() - 1, getHeight() - 1);

    // Push clip area so drawings don't overlap the border
    graphics->pushClipArea(gcn::Rectangle(1, 1, getWidth() - 2, getHeight() - 2));

    // Fill background
    graphics->setColor(getBackgroundColor());
    graphics->fillRectangle(gcn::Rectangle(0, 0, getWidth(), getHeight()));

    // Draw selection highlight (text starts at x=1 within clip area)
    if (mHasSelection && selectionLeft() != selectionRight()) {
        int x1 = getFont()->getWidth(mText.substr(0, selectionLeft())) - mXScroll + 1;
        int x2 = getFont()->getWidth(mText.substr(0, selectionRight())) - mXScroll + 1;
        graphics->setColor(getSelectionColor());
        graphics->fillRectangle(gcn::Rectangle(x1, 0, x2 - x1, getHeight()));
    }

    // Draw caret (suppressed when there is a non-empty selection)
    if (isFocused() && !(mHasSelection && selectionLeft() != selectionRight())) {
        drawCaret(graphics, getFont()->getWidth(mText.substr(0, mCaretPosition)) - mXScroll);
    }

    // Draw text
    graphics->setColor(getForegroundColor());
    graphics->setFont(getFont());
    graphics->drawText(mText, 1 - mXScroll, 1);

    graphics->popClipArea();
}


//
// Mouse handlers
//

void UTF8TextField::mousePressed(gcn::MouseEvent& mouseEvent)
{
    if (mouseEvent.getButton() == gcn::MouseEvent::LEFT) {
        unsigned int clickPos = getFont()->getStringIndexAt(mText, mouseEvent.getX() + mXScroll);
        if (mouseEvent.isShiftPressed()) {
            startSelectionIfNeeded();
            mCaretPosition = clickPos;
        } else {
            clearSelection();
            mCaretPosition = clickPos;
            mSelectionStart = mCaretPosition;
        }
        fixScroll();
    }
}

void UTF8TextField::mouseDragged(gcn::MouseEvent& mouseEvent)
{
    if (mouseEvent.getButton() == gcn::MouseEvent::LEFT) {
        mHasSelection = true;
        mCaretPosition = getFont()->getStringIndexAt(mText, mouseEvent.getX() + mXScroll);
        fixScroll();
    }
    mouseEvent.consume();
}


//
// keyPressed: UTF-8-aware navigation and editing, with selection support
//

void UTF8TextField::keyPressed(gcn::KeyEvent &keyEvent)
{
    gcn::Key key = keyEvent.getKey();
    bool is_shift_held = keyEvent.isShiftPressed();
    bool is_control_held = keyEvent.isControlPressed()
        && !keyEvent.isAltPressed()
        && !keyEvent.isMetaPressed();
    bool is_control_only = is_control_held && !is_shift_held;

    switch (key.getValue()) {
    case gcn::Key::LEFT:
        if (is_control_held) {
            // Word movement: Ctrl+Left / Ctrl+Shift+Left
            if (is_shift_held) startSelectionIfNeeded();
            else clearSelection();
            moveWordLeft();
        } else if (!is_shift_held && mHasSelection) {
            // Collapse selection to left edge without further movement
            mCaretPosition = selectionLeft();
            clearSelection();
        } else if (mCaretPosition > 0) {
            if (is_shift_held) startSelectionIfNeeded();
            // Move back one UTF-8 character
            std::string::iterator it = mText.begin() + mCaretPosition;
            try {
                utf8::prior(it, mText.begin());
                mCaretPosition = it - mText.begin();
            } catch (...) {
                // If UTF-8 is invalid, just move back one byte
                --mCaretPosition;
            }
        }
        keyEvent.consume();
        fixScroll();
        break;

    case gcn::Key::RIGHT:
        if (is_control_held) {
            // Word movement: Ctrl+Right / Ctrl+Shift+Right
            if (is_shift_held) startSelectionIfNeeded();
            else clearSelection();
            moveWordRight();
        } else if (!is_shift_held && mHasSelection) {
            // Collapse selection to right edge without further movement
            mCaretPosition = selectionRight();
            clearSelection();
        } else if (mCaretPosition < mText.size()) {
            if (is_shift_held) startSelectionIfNeeded();
            // Move forward one UTF-8 character
            std::string::iterator it = mText.begin() + mCaretPosition;
            try {
                utf8::next(it, mText.end());
                mCaretPosition = it - mText.begin();
            } catch (...) {
                // If UTF-8 is invalid, just move forward one byte
                ++mCaretPosition;
            }
        }
        keyEvent.consume();
        fixScroll();
        break;

    case gcn::Key::HOME:
        if (is_shift_held) {
            startSelectionIfNeeded();
        } else {
            clearSelection();
        }
        mCaretPosition = 0;
        keyEvent.consume();
        fixScroll();
        break;

    case gcn::Key::END:
        if (is_shift_held) {
            startSelectionIfNeeded();
        } else {
            clearSelection();
        }
        mCaretPosition = mText.size();
        keyEvent.consume();
        fixScroll();
        break;

    case gcn::Key::DELETE:
        if (mHasSelection) {
            deleteSelectedText();
        } else if (is_control_held) {
            // Ctrl+Delete: delete forward one word
            unsigned int start = mCaretPosition;
            moveWordRight();
            mText.erase(start, mCaretPosition - start);
            mCaretPosition = start;
        } else if (mCaretPosition < mText.size()) {
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
        }
        keyEvent.consume();
        fixScroll();
        break;

    case gcn::Key::BACKSPACE:
        if (mHasSelection) {
            deleteSelectedText();
        } else if (is_control_held) {
            // Ctrl+Backspace: delete backward one word
            unsigned int end_pos = mCaretPosition;
            moveWordLeft();
            mText.erase(mCaretPosition, end_pos - mCaretPosition);
        } else if (mCaretPosition > 0) {
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
        }
        keyEvent.consume();
        fixScroll();
        break;

    case 'a':
        if (is_control_only) {
            // Ctrl+A: select all
            mSelectionStart = 0;
            mCaretPosition = mText.size();
            mHasSelection = true;
            keyEvent.consume();
            fixScroll();
        }
        break;

    case 'x':
        if (is_control_only) {
            // Ctrl+X: cut
            if (mHasSelection) {
                SDL_SetClipboardText(getSelectedText().c_str());
                deleteSelectedText();
                fixScroll();
            }
            keyEvent.consume();
        }
        break;

    case 'c':
        if (is_control_only) {
            // Ctrl+C: copy
            if (mHasSelection) {
                SDL_SetClipboardText(getSelectedText().c_str());
            }
            keyEvent.consume();
        }
        break;

    case 'v':
        if (is_control_only) {
            // Ctrl+V: paste
            if (mHasSelection) {
                deleteSelectedText();
            }
            char* clipboard = SDL_GetClipboardText();
            if (clipboard) {
                std::string clip_str(clipboard);
                SDL_free(clipboard);
                if (!clip_str.empty()) {
                    mText.insert(mCaretPosition, clip_str);
                    mCaretPosition += clip_str.length();
                }
            }
            keyEvent.consume();
            fixScroll();
        }
        break;

    default:
        // For all other keys, use the default TextField behavior
        // (ENTER, TAB, character insertion all work fine with byte positions)
        gcn::TextField::keyPressed(keyEvent);
        break;
    }
}


//
// receiveTextInput: insert text, replacing any active selection first
//

void UTF8TextField::receiveTextInput(const Coercri::UTF8String &text)
{
    const std::string &utf8_str = text.asUTF8();

    if (utf8_str.empty()) {
        return;
    }

    if (mHasSelection) {
        deleteSelectedText();
    }

    // Insert the UTF-8 text at the current caret position
    mText.insert(mCaretPosition, utf8_str);

    // Advance caret by the number of bytes inserted
    mCaretPosition += utf8_str.length();

    // Ensure caret is visible
    fixScroll();
}

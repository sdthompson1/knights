/*
 * utf8_text_field.hpp
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

#ifndef UTF8_TEXT_FIELD_HPP
#define UTF8_TEXT_FIELD_HPP

#include "guichan.hpp"
#include "utf8string.hpp"
#include "gcn/text_input_receiver.hpp"

#include <string>

//
// Like a gcn::TextField, except that it properly handles UTF-8 encoded text.
//
// In the base TextField class, mCaretPosition is treated as a character index,
// which doesn't work correctly for multi-byte UTF-8 characters.
//
// This class overrides keyPressed and mousePressed to treat mCaretPosition as
// a byte position within the UTF-8 string, while ensuring operations (cursor
// movement, deletion) work on character boundaries.
//
// Also adds text selection support with keyboard (Shift+Arrow/Home/End, Ctrl+A)
// and mouse (click+drag, Shift+click), plus clipboard integration (Ctrl+X/C/V).
//

class UTF8TextField : public gcn::TextField, public Coercri::TextInputReceiver {
public:
    UTF8TextField() : gcn::TextField(), mSelectionStart(0), mHasSelection(false) { }
    explicit UTF8TextField(const std::string& text) : gcn::TextField(text), mSelectionStart(0), mHasSelection(false) { }

    // Override keyPressed to handle character stepping correctly (code points rather than bytes)
    // and to support Shift+arrow selection, Ctrl+A, Ctrl+X/C/V.
    virtual void keyPressed(gcn::KeyEvent &event) override;

    // TextInputReceiver interface implementation
    virtual void receiveTextInput(const Coercri::UTF8String &text) override;

    // Override draw to render selection highlight
    virtual void draw(gcn::Graphics* graphics) override;

    // Override mouse handlers for click and drag selection
    virtual void mousePressed(gcn::MouseEvent &mouseEvent) override;
    virtual void mouseDragged(gcn::MouseEvent &mouseEvent) override;

private:
    unsigned int mSelectionStart;  // byte offset - anchor point of selection
    bool mHasSelection;            // whether a selection is active

    unsigned int selectionLeft() const;
    unsigned int selectionRight() const;
    std::string getSelectedText() const;
    void deleteSelectedText();
    void clearSelection();
    void startSelectionIfNeeded();
    void moveWordLeft();
    void moveWordRight();
};

#endif

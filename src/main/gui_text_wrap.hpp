/*
 * gui_text_wrap.hpp
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

#ifndef GUI_TEXT_WRAP_HPP
#define GUI_TEXT_WRAP_HPP

#include "guichan.hpp"

class GuiTextWrap : public gcn::Widget, public gcn::MouseListener {
public:
    GuiTextWrap();
    
    void setText(const std::string &t) { text = t; }
    const std::string & getText() const { return text; }

    void adjustHeight();
    
    // whether to draw the background
    void setOpaque(bool o) { opaque = o; }
    bool isOpaque() const { return opaque; }

    // whether to centre each line of text
    void setCentred(bool c) { centred = c; }
    bool isCentred() const { return centred; }

    // whether to interpret %c or %l "rich text" sequences
    void setRich(bool r) { rich = r; }
    bool isRich() const { return rich; }
    
    virtual void draw(gcn::Graphics *graphics);
    virtual void mouseReleased(gcn::MouseEvent &mouseEvent);

private:
    std::string text;
    bool opaque;
    bool centred;
    bool rich;
};

#endif

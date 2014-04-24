/*
 * gui_numeric_field.hpp
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

#ifndef GUI_NUMERIC_FIELD_HPP
#define GUI_NUMERIC_FIELD_HPP

#include "guichan.hpp"

#include <string>

//
// Like a gcn::TextField, except:
//
// 1) only numeric characters may be entered
//
// 2) the contents can be no longer than N characters (N is set in ctor)
//
// 3) fires an ActionEvent whenever the field contents are changed,
//    not just when RETURN is pressed.

class GuiNumericField : public gcn::TextField {
public:
    explicit GuiNumericField(int max_digits_) : max_digits(max_digits_) { }
    virtual void keyPressed(gcn::KeyEvent &event);
private:
    int max_digits;
};

#endif

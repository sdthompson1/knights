/*
 * gui_numeric_field.hpp
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

#ifndef GUI_NUMERIC_FIELD_HPP
#define GUI_NUMERIC_FIELD_HPP

#include "guichan.hpp"

#include <string>
using namespace std;

//
// Like a gcn::TextField, except:
//
// 1) only numeric characters may be entered
//
// 2) the contents can be no longer than 2 characters
//
// 3) fires an ActionEvent whenever the field contents are changed,
//    not just when RETURN is pressed.
//
// TODO: Could make it more customizable e.g. allow the client to
// specify the max number of characters. But for now it's only used in
// one place so this is not needed.

class GuiNumericField : public gcn::TextField {
public:
    virtual void keyPressed(gcn::KeyEvent &event);
};

#endif

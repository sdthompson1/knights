/*
 * my_dropdown.hpp
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

#ifndef MY_DROPDOWN_HPP
#define MY_DROPDOWN_HPP

#include "guichan.hpp"

enum class MouseWheelScrolling { DISABLED, ENABLED };

// This class customizes Guichan dropdowns in three ways:
//
// (1) Fix a Guichan bug where pressing up or down arrow keys (while
//     the list is "dropped down") causes the selection to move by
//     two places instead of one. (Note that this bug is fixed in
//     the latest upstream version of Guichan, but not in our local
//     version.)
//
// (2) Add an option to disable the mouse wheel scrolling. On the
//     quest selection menu this is desirable because it can be
//     abused by players (rapid mouse wheel scrolling causes lots
//     of update spam - this was Trac #66 in the old bug tracker).
//
// (3) Change the drawing style: changing the foreground colour
//     no longer modifies the colour of the "down-arrow" drawn
//     in the dropdown's button.

class MyDropDown : public gcn::DropDown {
public:
    MyDropDown(gcn::ListModel *listModel, MouseWheelScrolling mouse_wheel);
    void drawButton(gcn::Graphics *graphics) override;
    void keyPressed(gcn::KeyEvent &keyEvent) override;
    void mouseWheelMovedDown(gcn::MouseEvent &mouseEvent) override;
    void mouseWheelMovedUp(gcn::MouseEvent &mouseEvent) override;

private:
    MouseWheelScrolling m_mouse_wheel;
};

#endif

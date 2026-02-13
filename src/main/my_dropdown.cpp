/*
 * my_dropdown.cpp
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

#include "my_dropdown.hpp"

namespace {
    class NoWheelListBox : public gcn::ListBox {
    public:
        void mouseWheelMovedUp(gcn::MouseEvent &) override { }
        void mouseWheelMovedDown(gcn::MouseEvent &) override { }
    };
}

MyDropDown::MyDropDown(gcn::ListModel *listModel, MouseWheelScrolling mouse_wheel)
    : gcn::DropDown(listModel,
                    NULL,
                    mouse_wheel == MouseWheelScrolling::DISABLED ? new NoWheelListBox() : NULL),
      m_mouse_wheel(mouse_wheel)
{
    if (mouse_wheel == MouseWheelScrolling::DISABLED) {
        // We passed a custom ListBox to the base class, but we still want
        // the base class destructor to own and delete it.
        mInternalListBox = true;
    }
}

void MyDropDown::drawButton(gcn::Graphics *graphics)
{
    gcn::Color old_fg_col = getForegroundColor();
    setForegroundColor(gcn::Color(0, 0, 0));
    gcn::DropDown::drawButton(graphics);
    setForegroundColor(old_fg_col);
}

void MyDropDown::keyPressed(gcn::KeyEvent &keyEvent)
{
    if (keyEvent.isConsumed())
        return;

    if (keyEvent.getKey().getValue() == gcn::Key::ESCAPE && mDroppedDown) {
        foldUp();
        keyEvent.consume();
        return;
    }

    gcn::DropDown::keyPressed(keyEvent);
}

void MyDropDown::mouseWheelMovedDown(gcn::MouseEvent &mouseEvent)
{
    if (m_mouse_wheel == MouseWheelScrolling::ENABLED) {
        gcn::DropDown::mouseWheelMovedDown(mouseEvent);
    }
}

void MyDropDown::mouseWheelMovedUp(gcn::MouseEvent &mouseEvent)
{
    if (m_mouse_wheel == MouseWheelScrolling::ENABLED) {
        gcn::DropDown::mouseWheelMovedUp(mouseEvent);
    }
}

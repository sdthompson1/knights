/*
 * gui_simple_container.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
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

#include "gui_simple_container.hpp"

GuiSimpleContainer::GuiSimpleContainer(gcn::Widget *c)
{
    setChild(c);
    addWidgetListener(this);
}

void GuiSimpleContainer::setChild(gcn::Widget *c)
{
    if (!mWidgets.empty()) {
        mWidgets.front()->removeWidgetListener(this);
        clear();
    }
    if (c) {
        add(c);
        c->addWidgetListener(this);
        widgetResized(gcn::Event(this));  // Force any re-layout etc in the subclass
    }
}

gcn::Widget * GuiSimpleContainer::getChild()
{
    if (mWidgets.empty()) {
        return 0;
    } else {
        return mWidgets.front();
    }
}


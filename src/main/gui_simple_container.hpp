/*
 * gui_simple_container.hpp
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

/*
 * Container that contains only a single child widget.
 *
 */

#ifndef GUI_SIMPLE_CONTAINER_HPP
#define GUI_SIMPLE_CONTAINER_HPP

#include "guichan.hpp"

class GuiSimpleContainer : public gcn::BasicContainer, public gcn::WidgetListener {
public:
    explicit GuiSimpleContainer(gcn::Widget *c);

    void setChild(gcn::Widget *c);
    gcn::Widget * getChild();

    // NB: Subclasses should override void draw(gcn::Graphics *graphics). Should at least call drawChildren(graphics).
    // Subclasses may also override void widgetResized(const gcn::Event &event) (called when either this or child is resized).
};

#endif

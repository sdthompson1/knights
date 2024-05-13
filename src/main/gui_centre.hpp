/*
 * gui_centre.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2024.
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

#ifndef GUI_CENTRE_HPP
#define GUI_CENTRE_HPP

#include "guichan.hpp"

class GuiCentre : public gcn::ScrollArea, public gcn::WidgetListener {
public:
    explicit GuiCentre(gcn::Widget *c);

    // inherited from WidgetListener
    void widgetResized(const gcn::Event &event);

private:
    bool resize_in_progress;
};

#endif

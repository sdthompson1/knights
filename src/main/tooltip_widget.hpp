/*
 * tooltip_widget.hpp
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

#ifndef TOOLTIP_WIDGET_HPP
#define TOOLTIP_WIDGET_HPP

#include "guichan.hpp"
#include "utf8string.hpp"
#include "gfx/window.hpp"
#include "timer/timer.hpp"

class TooltipWidget : public gcn::Widget {
public:
    TooltipWidget(const Coercri::UTF8String &text, int max_width, Coercri::Timer &timer, Coercri::Window &window);
    void scheduleShow();  // call from mouseEntered
    void cancelShow();    // call from mouseExited
    void draw(gcn::Graphics *graphics) override;
    void logic() override;
private:
    Coercri::UTF8String text;
    Coercri::Timer &timer;
    Coercri::Window &window;
    bool pending_show;
    unsigned int show_start_msec;
};

#endif

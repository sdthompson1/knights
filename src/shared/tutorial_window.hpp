/*
 * tutorial_window.hpp
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

#ifndef TUTORIAL_WINDOW_HPP
#define TUTORIAL_WINDOW_HPP

#include "colour_change.hpp"

#include <string>
#include <vector>

class Graphic;

struct TutorialWindow {
    TutorialWindow() : popup(false) { }

    // window title
    std::string title_latin1;

    // Graphics to be displayed alongside title
    std::vector<const Graphic *> gfx;
    std::vector<ColourChange> cc;

    // text to be displayed.
    // ^ character indicates new page.
    // %L, %R, %U, %D, %A, %S indicate knight control keys (left/right/up/down/action/suicide).
    // Also %M represents the string "WASD keys", "ARROW KEYS", or a longhand description of the 
    //   current movement keys, as appropriate.
    std::string msg_latin1;

    // Whether this should be a popup window or not
    bool popup;
};

#endif

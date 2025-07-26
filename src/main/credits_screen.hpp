/*
 * credits_screen.hpp
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

#ifndef CREDITS_SCREEN_HPP
#define CREDITS_SCREEN_HPP

#include "screen.hpp"

#include <string>

class CreditsScreenImpl;

class CreditsScreen : public Screen {
public:
    // This is used both for the Credits screen and for the First Time screen.
    // The ctor parameter is the name of the text file to display.
    explicit CreditsScreen(const std::string &f, int w = 999999) : filename(f), width(w) { }
    
    virtual bool start(KnightsApp &knights_app, boost::shared_ptr<Coercri::Window> window, gcn::Gui &gui);

private:
    boost::shared_ptr<CreditsScreenImpl> pimpl;
    std::string filename;
    int width;  // width limit, in capital M's.
};

#endif

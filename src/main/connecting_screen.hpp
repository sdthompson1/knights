/*
 * connecting_screen.hpp
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

#ifndef CONNECTING_SCREEN_HPP
#define CONNECTING_SCREEN_HPP

#include "screen.hpp"

#include <memory>
#include <string>

class ConnectingScreenImpl;
class PlayerID;

// Display a "Connecting to..." message and a Cancel button.
// This screen will remain until something else moves us into the game, or
// the user clicks the cancel button.

class ConnectingScreen : public Screen {
public:
    ConnectingScreen(const std::string &address,
                     const std::string &address_display_name);   // e.g. hostname if address is IP addr
    ~ConnectingScreen();
    virtual bool start(KnightsApp &knights_app, boost::shared_ptr<Coercri::Window> window, gcn::Gui &gui);

private:
    std::unique_ptr<ConnectingScreenImpl> pimpl;
};

#endif

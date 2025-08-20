/*
 * connecting_screen.hpp
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

#ifndef CONNECTING_SCREEN_HPP
#define CONNECTING_SCREEN_HPP

#include "screen.hpp"

#include "boost/shared_ptr.hpp"
#include <string>

class ConnectingScreenImpl;
class PlayerID;

class ConnectingScreen : public Screen {
public:
    // Note: for online platform games, address should be the lobby_id, and
    // port should be zero.
    ConnectingScreen(const std::string &address,
                     int port,
                     bool join_lan_game,
                     const PlayerID &player_id);
    virtual bool start(KnightsApp &knights_app, boost::shared_ptr<Coercri::Window> window, gcn::Gui &gui);
    virtual void update();
private:
    boost::shared_ptr<ConnectingScreenImpl> pimpl;
};

#endif

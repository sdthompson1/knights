/*
 * lobby_screen.hpp
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

#ifndef LOBBY_SCREEN_HPP
#define LOBBY_SCREEN_HPP

#include "screen.hpp"

class LobbyScreenImpl;

class LobbyScreen : public Screen {
public:
    // it's expected that both the KnightsClient and the GameManager have been created
    // before we go into the LobbyScreen.
    // also, we should already have sent the CLIENT_SET_PLAYER_ID msg if applicable.
    explicit LobbyScreen(boost::shared_ptr<KnightsClient> cli, const std::string &svr_name);
    ~LobbyScreen();
    virtual bool start(KnightsApp &knights_app, boost::shared_ptr<Coercri::Window> window, gcn::Gui &gui);
    virtual void update();
   
private:
    // prevent copying
    LobbyScreen(const LobbyScreen &);
    void operator=(const LobbyScreen&);

private:
    boost::shared_ptr<LobbyScreenImpl> pimpl;
};

#endif

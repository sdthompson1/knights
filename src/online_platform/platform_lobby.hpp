/*
 * platform_lobby.hpp
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

#ifndef PLATFORM_LOBBY_HPP
#define PLATFORM_LOBBY_HPP

#ifdef ONLINE_PLATFORM

#include <string>

// A PlatformLobby is a group of players who have gathered together to play a game.
// Players can create their own lobbies, or join or search for other lobbies, using
// the methods in OnlinePlatform.

class PlatformLobby {
public:

    enum class State {
        JOINING,
        JOINED,
        FAILED
    };

    // Destructor automatically leaves the lobby
    virtual ~PlatformLobby() = default;
    
    // Returns the current state of the lobby connection
    virtual State getState() = 0;
    
    // Returns the platform user ID of the current lobby leader
    // (Returns empty string if the leader is not known yet)
    virtual std::string getLeaderId() = 0;

    // Set status code for the lobby - only the leader can do this
    virtual void setStatusCode(int status_code) = 0;
};

#endif

#endif

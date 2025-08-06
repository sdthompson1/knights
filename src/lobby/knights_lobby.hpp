/*
 * knights_lobby.hpp
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

#ifndef KNIGHTS_LOBBY_HPP
#define KNIGHTS_LOBBY_HPP

#include "boost/shared_ptr.hpp"

class KnightsClient;

// A "KnightsLobby" is a container for a Knights game. Depending on
// the subclass it might represent either a locally hosted server, or
// a connection to a remote Knights server. Some subclasses might also
// support "host migration", which means that the lobby might
// transition between "client" and "server" status as a function of
// time.

// All KnightsLobbies implicitly contain a local connection to the
// game, representing the local player. Data can be sent into and out
// of this connection via the KnightsClient class. To read data from
// the server, call KnightsLobby::readIncomingMessages, which will
// invoke the relevant KnightsClient callbacks as required. To send
// data to the server, call the relevant methods on KnightsClient to
// "store up" outgoing messages, then call
// KnightsLobby::sendOutgoingMessages.

class KnightsLobby {
public:
    virtual ~KnightsLobby() { }

    // Read messages out of the game and send them to the given
    // KnightsClient object (i.e. invoke KnightsClient callbacks as
    // necessary).
    virtual void readIncomingMessages(KnightsClient &client) = 0;

    // Read messages out of the KnightsClient object and send them
    // into the game.
    virtual void sendOutgoingMessages(KnightsClient &client) = 0;

    // Query information about the lobby
    virtual int getNumberOfPlayers() const = 0;
};

#endif

/*
 * simple_knights_lobby.hpp
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

#ifndef SIMPLE_KNIGHTS_LOBBY_HPP
#define SIMPLE_KNIGHTS_LOBBY_HPP

#include "knights_lobby.hpp"
#include "localization.hpp"

#include "boost/thread.hpp"

#include <string>

class KnightsConfig;
class KnightsServer;
class ServerConnection;

namespace Coercri {
    class NetworkConnection;
    class NetworkDriver;
    class Timer;
}

// SimpleKnightsLobby implements the KnightsLobby interface and allows
// for hosting of LAN or local-only games, or joining remote games
// (either remote LAN games or full Knights servers). It does not
// support host migration.

// Note: This class starts a background thread which listens for incoming
// connections and routes packets appropriately.

class SimpleKnightsLobby : public KnightsLobby {
public:
    // Create a local-only game. This will start a local server but it
    // will not open any network connections. A config for the server
    // must be provided. Split screen games will be allowed.
    SimpleKnightsLobby(boost::shared_ptr<Coercri::Timer> timer,
                       boost::shared_ptr<KnightsConfig> config,
                       const std::string &game_name);

    // Host a LAN game. This will start a local server and also start
    // listening for incoming network connections on the given port. A
    // config for the server must be provided. A reference to the
    // net_driver will be kept (until the lobby is destroyed).
    SimpleKnightsLobby(Coercri::NetworkDriver &net_driver,
                       boost::shared_ptr<Coercri::Timer> timer,
                       int port,
                       boost::shared_ptr<KnightsConfig> config,
                       const std::string &game_name);

    // Join a remote game. This will not start any server but instead
    // it will open a single outgoing connection to the given address
    // and port. A reference to the net_driver will be kept (until the
    // lobby is destroyed).
    SimpleKnightsLobby(Coercri::NetworkDriver &net_driver,
                       boost::shared_ptr<Coercri::Timer> timer,
                       const std::string &address,
                       int port);

    // Destructor - this will shut down the local server (if there is
    // one), signal the background thread to stop (and wait for it to
    // exit), and close any network connections.
    virtual ~SimpleKnightsLobby() override;

    // Message send/receive implementations (from base class)
    virtual void readIncomingMessages(KnightsClient &) override;
    virtual void sendOutgoingMessages(KnightsClient &) override;

    // Query information about the lobby (from base class)
    virtual int getNumberOfPlayers() const override;

private:
    friend class SimpleLobbyThread;

    // Background thread: used in "Host LAN Game" mode, to accept and
    // process incoming connections.
    mutable boost::mutex mutex;
    boost::thread background_thread;
    bool exit_flag;  // Tells background thread to exit
    LocalMsg error_msg;  // Non-empty means background thread has reported an error

    Coercri::NetworkDriver *net_driver;  // NULL for a "Local Game"
    boost::shared_ptr<Coercri::Timer> timer;

    // Local server (used for "Host LAN Game" and "Local Game")
    std::unique_ptr<KnightsServer> server;

    // Local ServerConnection (used for "Host LAN Game" and "Local Game")
    ServerConnection *local_server_conn;

    // Outgoing network connection (used for "Join Remote Game")
    boost::shared_ptr<Coercri::NetworkConnection> outgoing_conn;

    // Incoming network connections from remote clients (used for "Host LAN Game")
    struct IncomingConn {
        ServerConnection *server_conn;
        boost::shared_ptr<Coercri::NetworkConnection> remote;
    };
    std::vector<IncomingConn> incoming_conns;
};

#endif

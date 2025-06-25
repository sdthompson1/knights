/*
 * knights_server_wrapper.hpp
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

#ifndef KNIGHTS_SERVER_WRAPPER_HPP
#define KNIGHTS_SERVER_WRAPPER_HPP

/* This class provides a thread-safe wrapper around a KnightsServer
   instance. The server can be created as "local" or "networked". In
   the "local" case, callers can use `openLocalConnection` to create
   new "loopback" connections, or `routeLocalPackets` to "pump"
   messages to/from the server instance. In the "networked" case, the
   server runs fully automatically in a background thread, listening
   on the given port, so nothing else needs to be done by the caller.
*/

#include "boost/shared_ptr.hpp"
#include "boost/thread.hpp"

#include <memory>
#include <string>

namespace Coercri {
    class NetworkConnection;
    class NetworkDriver;
    class Timer;
}

class KnightsClient;
class KnightsConfig;
class KnightsServer;
class ServerConnection;

class KnightsServerWrapper {
public:
    // Note: The KnightsServerWrapper should be destroyed before the
    // net_driver.
    KnightsServerWrapper(Coercri::NetworkDriver &net_driver,
                         boost::shared_ptr<Coercri::Timer> timer);
    ~KnightsServerWrapper();

    // Create a new server. It will immediately start listening for
    // connections on the given port. Packets will automatically be
    // routed between the KnightsServer instance and the network as
    // required.
    void createServer(int port,
                      boost::shared_ptr<KnightsConfig> config,
                      const std::string &game_name);

    // Create a "local" server. This will not access the network, nor
    // will it route packets automatically -- call routeLocalPackets
    // to do that.
    void createLocalServer(boost::shared_ptr<KnightsConfig> config,
                           const std::string &game_name);

    // Destroy the server instance. Any existing connections are
    // disconnected. (If the server is not created yet, this does nothing.)
    void destroyServer();

    // Create a new connection to a local server. (It's safe for the caller to
    // access the returned KnightsClient object without locking any mutex, because
    // the KnightsServerWrapper doesn't create a background thread in the local
    // server case.)
    boost::shared_ptr<KnightsClient> openLocalConnection();

    // Send/receive network messages for a local server. This should
    // be called periodically if the local server is in use.
    void routeLocalPackets();

    // Get current number of players on the server.
    int getNumberOfPlayers() const;


private:
    // prevent copying
    KnightsServerWrapper(const KnightsServerWrapper&);
    void operator=(const KnightsServerWrapper&);

    friend class ServerBackgroundThread;

private:
    mutable boost::mutex mutex;

    boost::thread background_thread;

    Coercri::NetworkDriver &net_driver;
    boost::shared_ptr<Coercri::Timer> timer;

    // the server itself
    std::unique_ptr<KnightsServer> server;

    // incoming network connections from remote clients
    struct IncomingConn {
        ServerConnection *server_conn;
        boost::shared_ptr<Coercri::NetworkConnection> remote;
    };
    std::vector<IncomingConn> incoming_conns;

    // local "loopback" connections
    struct LocalConn {
        boost::shared_ptr<KnightsClient> knights_client;
        ServerConnection * server_conn;
    };
    std::vector<LocalConn> local_conns;
};

#endif

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

#include "misc.hpp"

#include "exception_base.hpp"
#include "knights_client.hpp"
#include "knights_server.hpp"
#include "player_id.hpp"
#include "simple_knights_lobby.hpp"

#include "network/network_connection.hpp"
#include "network/network_driver.hpp"

// Background thread implementation
class SimpleLobbyThread {
public:
    explicit SimpleLobbyThread(SimpleKnightsLobby &lobby)
        : lobby(lobby)
    {}

    void operator()();

private:
    SimpleKnightsLobby &lobby;
};

void SimpleLobbyThread::operator()()
{
    try {
        std::vector<unsigned char> net_msg;

        while (true) {
            net_msg.clear();

            // Update network driver
            while (lobby.net_driver->doEvents()) { }

            {
                // Lock mutex during this section
                boost::unique_lock lock(lobby.mutex);

                // Check if main thread is telling us to exit
                if (lobby.exit_flag) {
                    break;
                }

                // Tell the server what the ping times are
                for (auto &conn : lobby.incoming_conns) {
                    lobby.server->setPingTime(*conn.server_conn, conn.remote->getPingTime());
                }

                // Receive incoming messages from our remote clients
                // Also remove any remote clients who have disconnected
                for (int i = 0; i < lobby.incoming_conns.size(); /* incremented below */) {
                    auto &in = lobby.incoming_conns[i];

                    in.remote->receive(net_msg);
                    if (!net_msg.empty()) {
                        lobby.server->receiveInputData(*in.server_conn, net_msg);
                    }

                    const Coercri::NetworkConnection::State state = in.remote->getState();
                    if (state == Coercri::NetworkConnection::CLOSED) {
                        // Connection lost: remove it from the list, and
                        // inform the server.
                        lobby.server->connectionClosed(*in.server_conn);
                        lobby.incoming_conns.erase(lobby.incoming_conns.begin() + i);
                    } else {
                        ++i;
                    }
                }

                // Send outgoing messages to our remote clients
                for (auto &conn : lobby.incoming_conns) {
                    lobby.server->getOutputData(*conn.server_conn, net_msg);
                    if (!net_msg.empty()) {
                        conn.remote->send(net_msg);
                    }
                }

                // Listen for new incoming connections
                Coercri::NetworkDriver::Connections new_conns = lobby.net_driver->pollIncomingConnections();
                for (auto &conn : new_conns) {
                    SimpleKnightsLobby::IncomingConn in;
                    in.server_conn = &lobby.server->newClientConnection(conn->getAddress(), PlayerID());
                    in.remote = conn;
                    lobby.incoming_conns.push_back(in);
                }
            }

            // Sleep to conserve CPU. We use a relatively short sleep so as not to
            // impact ping times too much.
            lobby.timer->sleepMsec(3);
        }

    } catch (ExceptionBase &e) {
        // Signal error to main thread
        boost::unique_lock lock(lobby.mutex);
        lobby.error_msg = e.getMsg();

    } catch (const std::exception &e) {
        // Signal error to main thread
        boost::unique_lock lock(lobby.mutex);
        lobby.error_msg = {LocalKey("cxx_error_is"), {LocalParam(Coercri::UTF8String::fromUTF8Safe(e.what()))}};

    } catch (...) {
        // Signal error to main thread
        boost::unique_lock lock(lobby.mutex);
        lobby.error_msg = {LocalKey("unknown_error")};
    }
}

// Constructor for "Local Game" mode
SimpleKnightsLobby::SimpleKnightsLobby(boost::shared_ptr<Coercri::Timer> timer,
                                       boost::shared_ptr<KnightsConfig> config,
                                       const std::string &game_name)
    : exit_flag(false),
      net_driver(nullptr),
      timer(timer),
      server(new KnightsServer(timer, true, "", ""))  // allow split-screen
{
    server->startNewGame(config, game_name);
    local_server_conn = &server->newClientConnection("", PlayerID());
}

// Constructor for "Host LAN Game" mode
SimpleKnightsLobby::SimpleKnightsLobby(Coercri::NetworkDriver &net_driver,
                                       boost::shared_ptr<Coercri::Timer> timer,
                                       int port,
                                       boost::shared_ptr<KnightsConfig> config,
                                       const std::string &game_name)
    : exit_flag(false),
      net_driver(&net_driver),
      timer(timer),
      server(new KnightsServer(timer, false, "", ""))  // don't allow split-screen
{
    server->startNewGame(config, game_name);
    local_server_conn = &server->newClientConnection("", PlayerID());

    net_driver.setServerPort(port);
    net_driver.enableServer(true);

    boost::thread new_thread(SimpleLobbyThread(*this));
    background_thread = std::move(new_thread);
}

// Constructor for "Join Remote Game" mode
SimpleKnightsLobby::SimpleKnightsLobby(Coercri::NetworkDriver &net_driver,
                                       boost::shared_ptr<Coercri::Timer> timer,
                                       const std::string &address,
                                       int port)
    : exit_flag(false),
      net_driver(&net_driver),
      timer(timer),
      local_server_conn(nullptr)
{
    outgoing_conn = net_driver.openConnection(address, port);
}

SimpleKnightsLobby::~SimpleKnightsLobby()
{
    // Tell the background thread to close
    {
        boost::unique_lock lock(mutex);
        exit_flag = true;
    }

    // Unlock mutex while we wait for the background thread to close
    // (as it will need to lock mutex to read the exit_flag)
    if (background_thread.joinable()) {
        background_thread.join();
    }

    // Close and clean up incoming connections
    for (auto & conn : incoming_conns) {
        server->connectionClosed(*conn.server_conn);
        conn.remote->close();
    }
    incoming_conns.clear();

    // Clean up other connection(s)
    outgoing_conn.reset();
    if (local_server_conn) {
        server->connectionClosed(*local_server_conn);
        local_server_conn = nullptr;
    }

    // Shut down the server
    server.reset();
    if (net_driver) {
        net_driver->enableServer(false);
    }
}

void SimpleKnightsLobby::readIncomingMessages(KnightsClient &client)
{
    boost::unique_lock lock(mutex);

    // Read data, either from local server (if there is one), or the
    // remote connection (if there is one)
    std::vector<unsigned char> net_msg;
    if (local_server_conn) {
        server->getOutputData(*local_server_conn, net_msg);
    } else if (outgoing_conn) {
        outgoing_conn->receive(net_msg);
    }

    // Forward the received data to the KnightsClient
    if (!net_msg.empty()) {
        client.receiveInputData(net_msg);
    }

    // Now check if the outgoing connection (if applicable) has dropped
    if (outgoing_conn) {
        const auto state = outgoing_conn->getState();
        if (state == Coercri::NetworkConnection::CLOSED || state == Coercri::NetworkConnection::FAILED) {
            if (state == Coercri::NetworkConnection::CLOSED) {
                client.connectionClosed();
            } else {
                client.connectionFailed();
            }
            outgoing_conn.reset();
        }
    }

    // Check if background thread has exited
    if (error_msg.key != LocalKey()) {
        throw ExceptionBase(error_msg);
    }
}

void SimpleKnightsLobby::sendOutgoingMessages(KnightsClient &client)
{
    boost::unique_lock lock(mutex);

    // Pick up any data from the KnightsClient
    std::vector<unsigned char> net_msg;
    client.getOutputData(net_msg);

    // If there was data then send it to the appropriate place
    if (!net_msg.empty()) {
        if (local_server_conn) {
            server->receiveInputData(*local_server_conn, net_msg);
        } else if (outgoing_conn) {
            outgoing_conn->send(net_msg);
        }
    }
}

int SimpleKnightsLobby::getNumberOfPlayers() const
{
    boost::unique_lock lock(mutex);

    if (server) {
        return server->getNumberOfPlayers();
    } else {
        // We don't know, so just return 0
        return 0;
    }
}

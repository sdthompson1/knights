/*
 * knights_server_wrapper.cpp
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

#include "knights_server_wrapper.hpp"

#include "knights_client.hpp"
#include "knights_server.hpp"
#include "my_exceptions.hpp"

// coercri includes
#include "network/network_connection.hpp"
#include "network/network_driver.hpp"

class ServerBackgroundThread {
public:
    explicit ServerBackgroundThread(KnightsServerWrapper &wrap) : wrapper(wrap) {}
    void operator()();
private:
    KnightsServerWrapper &wrapper;
};

KnightsServerWrapper::KnightsServerWrapper(Coercri::NetworkDriver &driver,
                                           boost::shared_ptr<Coercri::Timer> timer_)
    : net_driver(driver), timer(timer_)
{}

KnightsServerWrapper::~KnightsServerWrapper()
{
    destroyServer();
}

void KnightsServerWrapper::createServer(int port,
                                        boost::shared_ptr<KnightsConfig> config,
                                        const std::string &game_name)
{
    boost::unique_lock lock(mutex);

    if (server) {
        throw UnexpectedError("server already created");
    }

    server.reset(new KnightsServer(timer, false, "", "", ""));
    server->startNewGame(config, game_name);

    net_driver.setServerPort(port);
    net_driver.enableServer(true);

    boost::thread new_thread(ServerBackgroundThread(*this));
    background_thread.swap(new_thread);
}

void KnightsServerWrapper::createLocalServer(boost::shared_ptr<KnightsConfig> config,
                                             const std::string &game_name)
{
    boost::unique_lock lock(mutex);

    if (server) {
        throw UnexpectedError("server already created");
    }

    server.reset(new KnightsServer(timer, true, "", "", ""));
    server->startNewGame(config, game_name);
}

void KnightsServerWrapper::destroyServer()
{
    {
        boost::unique_lock lock(mutex);

        // Close and clean up incoming connections
        for (auto& conn : incoming_conns) {
            server->connectionClosed(*conn.server_conn);
            conn.remote->close();
        }
        incoming_conns.clear();

        // Clean up local connections
        local_conns.clear();

        // Shut down the server
        server.reset();
        net_driver.enableServer(false);
    }

    // The background thread should now exit on its own
    if (background_thread.joinable()) {
        background_thread.join();
    }
}

boost::shared_ptr<KnightsClient> KnightsServerWrapper::openLocalConnection()
{
    boost::unique_lock lock(mutex);

    LocalConn conn;
    conn.knights_client.reset(new KnightsClient);
    conn.server_conn = &server->newClientConnection();
    local_conns.push_back(conn);

    return conn.knights_client;
}

void KnightsServerWrapper::routeLocalPackets()
{
    boost::unique_lock lock(mutex);

    std::vector<unsigned char> net_msg;

    for (auto &conn : local_conns) {
        conn.knights_client->getOutputData(net_msg);
        server->receiveInputData(*conn.server_conn, net_msg);

        server->getOutputData(*conn.server_conn, net_msg);
        conn.knights_client->receiveInputData(net_msg);
    }
}

int KnightsServerWrapper::getNumberOfPlayers() const
{
    boost::unique_lock lock(mutex);

    if (server) {
        return server->getNumberOfPlayers();
    } else {
        return 0;
    }
}

void ServerBackgroundThread::operator()()
{
    std::vector<unsigned char> net_msg;

    while (true) {

        net_msg.clear();

        // Make sure the network driver is fully updated
        while (wrapper.net_driver.doEvents()) {}

        {
            // Lock mutex during this section
            boost::unique_lock lock(wrapper.mutex);

            // If server has disappeared then we can exit the thread
            if (!wrapper.server) break;

            // Tell the server what the ping times are
            for (auto &conn : wrapper.incoming_conns) {
                wrapper.server->setPingTime(*conn.server_conn, conn.remote->getPingTime());
            }

            // Send outgoing messages
            for (auto &conn : wrapper.incoming_conns) {
                wrapper.server->getOutputData(*conn.server_conn, net_msg);
                if (!net_msg.empty()) {
                    conn.remote->send(net_msg);
                }
            }

            // Receive incoming messages
            for (int i = 0; i < wrapper.incoming_conns.size(); /* incremented below */) {
                auto &in = wrapper.incoming_conns[i];

                in.remote->receive(net_msg);
                if (!net_msg.empty()) {
                    wrapper.server->receiveInputData(*in.server_conn, net_msg);
                }

                const Coercri::NetworkConnection::State state = in.remote->getState();
                if (state == Coercri::NetworkConnection::CLOSED) {
                    // connection lost: remove it from the list, and inform the server
                    wrapper.server->connectionClosed(*in.server_conn);
                    wrapper.incoming_conns.erase(wrapper.incoming_conns.begin() + i);
                } else {
                    ++i;
                }
            }

            // Listen for new incoming connections
            Coercri::NetworkDriver::Connections new_conns = wrapper.net_driver.pollIncomingConnections();
            for (auto &conn : new_conns) {
                KnightsServerWrapper::IncomingConn in;
                in.server_conn = &wrapper.server->newClientConnection();
                in.remote = conn;
                wrapper.incoming_conns.push_back(in);
            }
        }

        // Sleep to conserve CPU. (This has to be a relatively short sleep so as not
        // to impact ping times too much.)
        wrapper.timer->sleepMsec(3);
    }
}

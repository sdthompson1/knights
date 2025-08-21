/*
 * vm_knights_lobby.cpp
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

#include "follower_state.hpp"
#include "leader_state.hpp"
#include "vm_knights_lobby.hpp"

#include "knights_client.hpp"
#include "knights_vm.hpp"

#include "network/network_driver.hpp"
#include "timer/timer.hpp"

#include "boost/thread.hpp"

#ifdef USE_VM_LOBBY

// Background thread object
struct VMKnightsLobbyThread {
    VMKnightsLobbyImpl &impl;
    VMKnightsLobbyThread(VMKnightsLobbyImpl &impl) : impl(impl) {}
    void operator()();
};

// Pimpl struct
struct VMKnightsLobbyImpl {
    // constructor
    VMKnightsLobbyImpl(Coercri::NetworkDriver &net_driver,
                       Coercri::Timer &timer,
                       const PlayerID &local_user_id,
                       bool new_control_system)
        : net_driver(net_driver),
          timer(timer),
          local_user_id(local_user_id),
          new_control_system(new_control_system),
          exit_flag(false)
    {}

    // fields
    boost::mutex mutex;
    boost::thread background_thread;
    Coercri::NetworkDriver &net_driver;
    Coercri::Timer &timer;
    PlayerID local_user_id;
    bool new_control_system;
    bool exit_flag;
    std::unique_ptr<LeaderState> leader;
    std::unique_ptr<FollowerState> follower;
};

VMKnightsLobby::VMKnightsLobby(Coercri::NetworkDriver &net_driver,
                               Coercri::Timer &timer,
                               const PlayerID &local_user_id,
                               bool new_control_system)
{
    // Initialize pimpl fields
    pimpl = std::make_unique<VMKnightsLobbyImpl>(net_driver, timer, local_user_id, new_control_system);

    // Become leader initially (which starts the VM and runs an
    // initial tick), but do not open the listening port yet
    pimpl->leader = std::make_unique<LeaderState>(timer, local_user_id);

    // Start background thread
    pimpl->background_thread = boost::thread(VMKnightsLobbyThread(*pimpl));
}

VMKnightsLobby::~VMKnightsLobby()
{
    // Signal thread to exit
    {
        boost::unique_lock<boost::mutex> lock(pimpl->mutex);
        pimpl->exit_flag = true;
    }
    
    // Wait for thread to finish
    pimpl->background_thread.join();
}

void VMKnightsLobby::becomeLeader(int port)
{
    bool need_rejoin = false;

    {
        boost::unique_lock<boost::mutex> lock(pimpl->mutex);

        // Promote follower to leader if required
        if (pimpl->follower) {
            std::unique_ptr<KnightsVM> vm = pimpl->follower->migrate();
            pimpl->follower.reset();
            pimpl->leader = std::make_unique<LeaderState>(pimpl->timer,
                                                          pimpl->local_user_id,
                                                          std::move(vm));
            need_rejoin = true;
        }

        // Start listening for incoming connections
        pimpl->net_driver.setServerPort(port);
        pimpl->net_driver.enableServer(true);
    }

    if (need_rejoin) {
        rejoinGame();
    }
}

void VMKnightsLobby::becomeFollower(const std::string &address, int port)
{
    {
        boost::unique_lock<boost::mutex> lock(pimpl->mutex);

        // Stop listening for incoming connections
        pimpl->net_driver.enableServer(false);

        // Close existing follower or leader, extracting the VM
        std::unique_ptr<KnightsVM> vm;
        if (pimpl->follower) {
            vm = std::move(pimpl->follower->migrate());
            pimpl->follower.reset();
        } else {
            vm = std::move(pimpl->leader->migrate());
            pimpl->leader.reset();
        }

        // Create a new follower, connecting to the given address and port
        auto connection = pimpl->net_driver.openConnection(address, port);
        pimpl->follower = std::make_unique<FollowerState>(std::move(vm), connection);
    }

    rejoinGame();
}

void VMKnightsLobby::rejoinGame()
{
    KnightsClient client;
    client.setPlayerIdAndControls(pimpl->local_user_id, pimpl->new_control_system);
    client.joinGame("#VMGame");
    sendOutgoingMessages(client);
}

void VMKnightsLobby::readIncomingMessages(KnightsClient &client)
{
    std::vector<unsigned char> data;

    // Read data from the game
    {
        boost::unique_lock<boost::mutex> lock(pimpl->mutex);

        if (pimpl->leader) {
            pimpl->leader->receiveClientMessages(data);
        } else {
            pimpl->follower->receiveClientMessages(data);
        }
    }

    // Send it to the client
    client.receiveInputData(data);
}

void VMKnightsLobby::sendOutgoingMessages(KnightsClient &client)
{
    // Read data from the client
    std::vector<unsigned char> data;
    client.getOutputData(data);

    // Send it to the game
    boost::unique_lock<boost::mutex> lock(pimpl->mutex);
    if (pimpl->leader) {
        pimpl->leader->sendClientMessages(data);
    } else {
        pimpl->follower->sendClientMessages(data);
    }
}

int VMKnightsLobby::getNumberOfPlayers() const
{
    // This just returns 0 currently
    return 0;
}

Coercri::NetworkConnection::State VMKnightsLobby::getConnectionState() const
{
    if (pimpl->leader) {
        // The leader is permanently "connected"
        return Coercri::NetworkConnection::CONNECTED;
    } else {
        return pimpl->follower->getConnectionState();
    }
}


// The main method for the background thread
void VMKnightsLobbyThread::operator()()
{
    while (true) {

        // Before commencing main loop, ensure net_driver is updated
        while (impl.net_driver.doEvents()) { }

        {
            // Lock mutex while main loop active
            boost::unique_lock<boost::mutex> lock(impl.mutex);

            // Check if we need to exit
            if (impl.exit_flag) break;

            // Tick the VM, send/receive network messages, do other background tasks
            if (impl.leader) {
                impl.leader->update(impl.net_driver, impl.timer);
            } else if (impl.follower) {
                impl.follower->update(impl.net_driver, impl.timer);
            }
        }

        // Sleep for a short interval between loop iterations
        impl.timer.sleepMsec(3);
    }
}

#endif  // USE_VM_LOBBY

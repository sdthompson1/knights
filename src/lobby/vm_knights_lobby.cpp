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

#ifdef USE_VM_LOBBY

#include "follower_state.hpp"
#include "knights_client.hpp"
#include "knights_vm.hpp"
#include "leader_state.hpp"
#include "rng.hpp"
#include "vm_knights_lobby.hpp"

#include "network/network_driver.hpp"
#include "timer/timer.hpp"

#include "boost/thread.hpp"

//#define LOG_VM_LOBBY

#ifdef LOG_VM_LOBBY
#include <iostream>
#endif

// Constants
namespace {
    constexpr int CONNECT_RETRY_MIN_MS = 1000;
    constexpr int CONNECT_RETRY_MAX_MS = 3000;
    constexpr int CONNECT_TIMEOUT_MS = 60 * 1000;
}

// Background thread object
struct VMKnightsLobbyThread {
    VMKnightsLobbyImpl &impl;
    VMKnightsLobbyThread(VMKnightsLobbyImpl &impl) : impl(impl) {}
    void operator()();
};

// Pimpl struct
struct VMKnightsLobbyImpl {
    VMKnightsLobbyImpl(Coercri::NetworkDriver &net_driver,
                       Coercri::Timer &timer,
                       const PlayerID &local_user_id,
                       bool new_control_system)
        : exit_flag(false),
          error_flag(false),
          net_driver(net_driver),
          timer(timer),
          local_user_id(local_user_id),
          new_control_system(new_control_system),
          leader_port(0),
          next_reconnect_time_ms(0),
          give_up_time_ms(0),
          retry_logic_enabled(false),
          failure_reported(false)
    {}

    void disableRetryLogic() {
        next_reconnect_time_ms = give_up_time_ms = 0;
        retry_logic_enabled = false;
    }

    void enableRetryLogic(unsigned int time_now_ms) {
        next_reconnect_time_ms = time_now_ms + g_rng.getInt(CONNECT_RETRY_MIN_MS, CONNECT_RETRY_MAX_MS);
        give_up_time_ms = time_now_ms + CONNECT_TIMEOUT_MS;
        retry_logic_enabled = true;
    }

    void setupNextRetry(unsigned int time_now_ms) {
        next_reconnect_time_ms = time_now_ms + g_rng.getInt(CONNECT_RETRY_MIN_MS, CONNECT_RETRY_MAX_MS);
    }

    // background thread
    boost::mutex mutex;
    boost::thread background_thread;
    bool exit_flag;
    bool error_flag;

    // refs
    Coercri::NetworkDriver &net_driver;
    Coercri::Timer &timer;

    // player info
    PlayerID local_user_id;
    bool new_control_system;

    // leader and follower state (exactly one is non-NULL at any given time)
    std::unique_ptr<LeaderState> leader;
    std::unique_ptr<FollowerState> follower;

    // cached leader address and port (in case we need to reconnect)
    std::string leader_address;
    int leader_port;

    // retry logic
    unsigned int next_reconnect_time_ms;
    unsigned int give_up_time_ms;
    bool retry_logic_enabled;
    bool failure_reported;
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
    boost::unique_lock<boost::mutex> lock(pimpl->mutex);

    if (pimpl->follower) {
        // Promote follower to leader
        std::unique_ptr<KnightsVM> vm = pimpl->follower->migrate();
        pimpl->follower.reset();
        pimpl->leader = std::make_unique<LeaderState>(pimpl->timer,
                                                      pimpl->local_user_id,
                                                      std::move(vm));
        rejoinGame();
    }

    // Start listening for incoming connections
    pimpl->net_driver.enableServer(false);
    pimpl->net_driver.setServerPort(port);
    pimpl->net_driver.enableServer(true);

    // Retry logic is not relevant for leader
    pimpl->disableRetryLogic();

#ifdef LOG_VM_LOBBY
        std::cout << "Became leader." << std::endl;
#endif
}

void VMKnightsLobby::becomeFollower(const std::string &address, int port)
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
    pimpl->leader_address = address;
    pimpl->leader_port = port;
    auto connection = pimpl->net_driver.openConnection(address, port);
    pimpl->follower = std::make_unique<FollowerState>(std::move(vm), connection);

    // Retry logic is needed while we are a follower
    pimpl->enableRetryLogic(pimpl->timer.getMsec());

    rejoinGame();

#ifdef LOG_VM_LOBBY
    std::cout << "Became follower." << std::endl;
#endif

}

// Mutex should be locked when this is called
void VMKnightsLobby::rejoinGame()
{
    KnightsClient client;
    client.setPlayerIdAndControls(pimpl->local_user_id, pimpl->new_control_system);
    client.joinGame("#VMGame");
    std::vector<unsigned char> data;
    client.getOutputData(data);
    if (pimpl->leader) {
        pimpl->leader->sendClientMessages(data);
    } else {
        pimpl->follower->sendClientMessages(data);
    }
}

void VMKnightsLobby::readIncomingMessages(KnightsClient &client)
{
    std::vector<unsigned char> data;
    bool connection_failed = false;

    {
        // Lock mutex while accessing pimpl fields
        boost::unique_lock<boost::mutex> lock(pimpl->mutex);

        // Read data from the game
        if (pimpl->leader) {
            pimpl->leader->receiveClientMessages(data);
        } else {
            pimpl->follower->receiveClientMessages(data);
        }

        // Apply retry logic if applicable
        if (pimpl->follower && !pimpl->failure_reported) {
            connection_failed = applyRetryLogic();
        }

        // Check if thread has closed
        if (pimpl->error_flag) {
            throw std::runtime_error("Error in game thread");
        }
    }

    // Send received data to the client
    client.receiveInputData(data);

    // Send connectionFailed if required
    if (connection_failed) {
        client.connectionFailed();
    }
}

// This is called when we are in follower mode, and failure has not already been reported.
// Mutex should be locked when this is called.
// If this returns true then client.connectionFailed() should be called.
bool VMKnightsLobby::applyRetryLogic()
{
    const auto state = pimpl->follower->getConnectionState();
    unsigned int time_now_ms = pimpl->timer.getMsec();

    // If retry logic is not currently active, then we just need to check whether
    // the connection has dropped, and if so, activate the retry logic.
    if (!pimpl->retry_logic_enabled) {
        if (state != Coercri::NetworkConnection::CONNECTED) {
#ifdef LOG_VM_LOBBY
            std::cout << "No longer connected, enabling retry logic." << std::endl;
#endif
            pimpl->enableRetryLogic(time_now_ms);
        }
        return false;
    }

    // Retry logic is active.

    if (state == Coercri::NetworkConnection::CONNECTED) {
        // We have successfully connected, so disable the retry logic and return.
#ifdef LOG_VM_LOBBY
        std::cout << "Connection established, disabling retry logic." << std::endl;
#endif
        pimpl->disableRetryLogic();
        return false;
    }

    // Retry logic is active, and the connection is either lost/failed, or still pending.

    // First check the "give up" timeout.
    int time_to_give_up_ms = pimpl->give_up_time_ms - time_now_ms;
    if (time_to_give_up_ms <= 0) {
        // Report connection failure
#ifdef LOG_VM_LOBBY
        std::cout << "Timeout reached: reporting connection failure." << std::endl;
#endif
        pimpl->failure_reported = true;
        return true;
    }

    // If state is PENDING we can now just wait
    if (state == Coercri::NetworkConnection::PENDING) {
        return false;
    }

    // State is FAILED or CLOSED. If retry-time has arrived, then reopen a new
    // connection; otherwise just wait.
    int time_to_retry_ms = pimpl->next_reconnect_time_ms - time_now_ms;
    if (time_to_retry_ms <= 0) {
#ifdef LOG_VM_LOBBY
        std::cout << "Retrying connection to leader." << std::endl;
#endif
        // Recreate the follower
        auto vm = std::move(pimpl->follower->migrate());
        auto connection = pimpl->net_driver.openConnection(pimpl->leader_address, pimpl->leader_port);
        pimpl->follower = std::make_unique<FollowerState>(std::move(vm), connection);

        // Make sure we rejoin the game (once we are connected)
        rejoinGame();

        // Setup for next retry
        pimpl->setupNextRetry(time_now_ms);
    }

    return false;
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

bool VMKnightsLobby::connected() const
{
    boost::unique_lock<boost::mutex> lock(pimpl->mutex);

    if (pimpl->leader) {
        return true;
    } else {
        return pimpl->follower->getConnectionState() == Coercri::NetworkConnection::CONNECTED;
    }
}


// The main method for the background thread
void VMKnightsLobbyThread::operator()()
{
    try {
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

    } catch (...) {
        boost::unique_lock<boost::mutex> lock(impl.mutex);
        impl.error_flag = true;
    }
}

#endif  // USE_VM_LOBBY

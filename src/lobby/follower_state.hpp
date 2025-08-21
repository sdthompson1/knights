/*
 * follower_state.hpp
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

#ifndef FOLLOWER_STATE_HPP
#define FOLLOWER_STATE_HPP

#ifdef USE_VM_LOBBY

#include "network/network_connection.hpp"

#include "boost/shared_ptr.hpp"

#include <memory>
#include <vector>

class KnightsVM;
class SyncClient;

namespace Coercri {
    class NetworkDriver;
    class Timer;
}

class FollowerState {
public:
    // Create a new FollowerState, given an open network connection to the leader.
    // An existing KnightsVM state must be provided. The existing KnightsVM should
    // have no active client connections.
    FollowerState(std::unique_ptr<KnightsVM> vm,
                  boost::shared_ptr<Coercri::NetworkConnection> connection_to_leader);

    // Destructor.
    ~FollowerState();

    // Receive messages from the game to the local player.
    void receiveClientMessages(std::vector<unsigned char> &data);

    // Send messages from the local player to the game.
    void sendClientMessages(const std::vector<unsigned char> &data);

    // Run a background update - call every few milliseconds.
    void update(Coercri::NetworkDriver &net_driver, Coercri::Timer &timer);

    // Pull out the KnightsVM, closing all connections to it.
    // This will leave the FollowerState unusable, so it should be destroyed soon after.
    std::unique_ptr<KnightsVM> migrate();

    // Get current network connection state
    Coercri::NetworkConnection::State getConnectionState() const {
        return connection_to_leader->getState();
    }

    int artificial_delay; // TODO remove this

private:
    // VM state
    std::unique_ptr<KnightsVM> knights_vm;

    // Local player
    int local_client_num;  // -1 if not assigned yet
    std::vector<unsigned char> local_player_packets;  // Msgs from game to local player

    // Connection to leader
    boost::shared_ptr<Coercri::NetworkConnection> connection_to_leader;
    std::vector<unsigned char> delayed_packets;  // Msgs to be sent to leader after sync complete

    // Sync client
    std::unique_ptr<SyncClient> sync_client;
};

#endif  // USE_VM_LOBBY

#endif  // FOLLOWER_STATE_HPP

/*
 * vm_knights_lobby.hpp
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

#ifndef VM_KNIGHTS_LOBBY_HPP
#define VM_KNIGHTS_LOBBY_HPP

#ifdef USE_VM_LOBBY

#include "knights_lobby.hpp"

#include "network/network_connection.hpp"

class VMKnightsLobbyImpl;

namespace Coercri {
    class NetworkDriver;
    class Timer;
}

// VMKnightsLobby implements the KnightsLobby interface and provides
// "host-migratable" network games. The Knights server is run inside a
// virtual machine (VM), with the VM memory contents being kept
// synchronized between players at all times. The "leader" of the game
// can be migrated to any of the players at any time. (An external
// source, such as a PlatformLobby, must be used to arbitrate who is
// the leader at any given time.)

// Note: This class starts a background thread and will use the given
// Coercri::NetworkDriver to make or listen for network connections
// (as appropriate).

class VMKnightsLobby : public KnightsLobby {
public:
    // Create a new VM game. This will boot up the VM but the game
    // will neither be a follower nor a leader initially (you will
    // need to call becomeLeader or becomeFollower as appropriate).
    // Note that no KnightsConfig is required; instead, the VM loads
    // the data files "internally" during its own boot-up process.
    VMKnightsLobby(Coercri::NetworkDriver &net_driver,
                   Coercri::Timer &timer,
                   const PlayerID &local_user_id,
                   bool new_control_system);

    // Destructor - this will shut down the VM, signal the background
    // thread to stop (and wait for it to exit), and close any network
    // connections.
    virtual ~VMKnightsLobby() override;

    // Call this to make the VMKnightsLobby become a leader. This will
    // start listening for incoming connections on the given port.
    void becomeLeader(int port);

    // Call this to make the VMKnightsLobby become a follower, or to
    // change who we are currently following. This will open an
    // outgoing connection to the given address and port.
    void becomeFollower(const std::string &address, int port);

    // Message send/receive implementations (from base class)
    virtual void readIncomingMessages(KnightsClient &) override;
    virtual void sendOutgoingMessages(KnightsClient &) override;

    // This just returns 0 currently
    virtual int getNumberOfPlayers() const override;

    // Determine connection status. Returns CONNECTED for the leader, and
    // returns the actual connection status for a follower.
    Coercri::NetworkConnection::State getConnectionState() const;

private:
    void rejoinGame();

private:
    std::unique_ptr<VMKnightsLobbyImpl> pimpl;
};

#endif  // USE_VM_LOBBY

#endif  // VM_KNIGHTS_LOBBY_HPP

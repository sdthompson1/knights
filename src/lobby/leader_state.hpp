/*
 * leader_state.hpp
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

#ifndef LEADER_STATE_HPP
#define LEADER_STATE_HPP

#ifdef USE_VM_LOBBY

#include "player_id.hpp"

class KnightsVM;
class SyncHost;
class TickWriter;

namespace Coercri {
    class NetworkConnection;
    class NetworkDriver;
    class Timer;
}

#include "boost/shared_ptr.hpp"

#include <memory>
#include <vector>

class LeaderState {
public:
    // Create a new LeaderState.
    // This boots up a new KnightsVM (which will take some time). The VM will itself
    // create a new Knights game named "#VMGame" ready for the players to join.
    // It also opens a single connection to the VM representing the local player, but
    // does not issue the CLIENT_JOIN_GAME command (the caller must do that).
    LeaderState(Coercri::Timer &timer,
                const PlayerID &local_user_id);

    // This is the same as the previous constructor except that it starts from an
    // existing KnightsVM state instead of booting up a new one. The existing KnightsVM
    // should have no active client connections when this is called.
    LeaderState(Coercri::Timer &timer,
                const PlayerID &local_user_id,
                std::unique_ptr<KnightsVM> knights_vm);

    // Destructor.
    ~LeaderState();

    // Receive messages from the game to the local player.
    // Note: existing contents of data will be replaced
    void receiveClientMessages(std::vector<unsigned char> &data);

    // Send messages from the local player into the game.
    // Note: data should be non-empty
    void sendClientMessages(const std::vector<unsigned char> &data);

    // Run a background update - called every few milliseconds.
    // This manages incoming connections from the network as "followers" - they will
    // become new clients of the VM and ticks will be forwarded to them as needed.
    // (The caller must call enableServer(true) on the net_driver if required.)
    void update(Coercri::NetworkDriver &net_driver, Coercri::Timer &timer);

    // Pull out the KnightsVM, closing all connections to it.
    // This will leave the LeaderState unusable, so it should be destroyed soon after.
    std::unique_ptr<KnightsVM> migrate();

private:
    void initialize(const PlayerID &local_user_id, int sleep_time_ms);
    bool clientNumInUse(int client_num) const;
    void receiveFollowerMessages(int client_num, Coercri::NetworkConnection &connection);
    bool processVmOutputData(const std::vector<unsigned char> &vm_output_data);
    void flushTickData();

private:
    Coercri::Timer &timer;

    // VM state
    std::unique_ptr<KnightsVM> knights_vm;

    // Local player (always client num 0)
    std::vector<unsigned char> local_player_packets;  // Cached output from VM to local player

    // Followers (indexed by client_num).
    // The network connection will be NULL for the local player (client num 0)
    // or for unassigned client nums.
    std::vector<boost::shared_ptr<Coercri::NetworkConnection>> followers;

    // Sync objects, indexed by client_num. NULL if no sync in progress.
    std::vector<std::unique_ptr<SyncHost>> follower_sync;

    // Current tick(s) being built
    std::vector<unsigned char> tick_data;
    std::unique_ptr<TickWriter> tick_writer;
    size_t current_tick_beginning_index;
    bool tick_data_contains_output;

    // Tick submission to VM
    unsigned int last_tick_time_ms;
    unsigned int next_tick_deadline_ms;

    // Tick flushing to followers
    unsigned int last_flush_time_ms;

    // Ping Times
    unsigned int last_ping_update_ms;
};

#endif  // USE_VM_LOBBY

#endif  // LEADER_STATE_HPP

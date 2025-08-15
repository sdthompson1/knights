/*
 * sync_host.hpp
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

#ifndef SYNC_HOST_HPP
#define SYNC_HOST_HPP

#ifdef USE_VM_LOBBY

#include "knights_vm.hpp"  // for MemoryBlock

namespace Coercri {
    class NetworkConnection;
}

#include <deque>
#include <vector>

class SyncHost {
public:
    // The constructor will snapshot the VM state, and kick off the
    // sync process by sending LEADER_SEND_VM_CONFIG
    SyncHost(Coercri::NetworkConnection &conn,
             KnightsVM &vm);

    // This will read as many msgs as possible from buf until either sync is complete,
    // or buf is exhausted.
    // It will also send any required responses to the follower.
    // Returns true if sync done (and SyncHost should be destroyed) or false if
    // sync is still in progress.
    bool processMessagesFromFollower(Coercri::InputByteBuf &buf);

    // Add additional ticks that occur before the sync has finished. The sync process
    // will take care of sending these ticks (on top of the original VM state).
    void addCatchupTicks(const std::vector<unsigned char> &tick_data);

private:
    void receiveHashes(Coercri::InputByteBuf &buf);
    void receiveMemoryBlockAck(Coercri::InputByteBuf &buf);
    void receiveCatchupTickAck(Coercri::InputByteBuf &buf);
    bool sendResponseToClient();
    void trimMemoryBlocks();

private:
    Coercri::NetworkConnection &connection;

    std::deque<MemoryBlock> memory_blocks;  // VM memory snapshot

    bool hashes_received;

    int num_blocks_outstanding;  // Number of blocks sent but not acked yet

    std::deque<std::vector<unsigned char>> catchup_ticks_to_send;  // In approx 4K chunks
    int num_tick_segments_outstanding;
};

#endif  // USE_VM_LOBBY

#endif  // SYNC_HOST_HPP

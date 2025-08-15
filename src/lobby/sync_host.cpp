/*
 * sync_host.cpp
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

#include "sync_host.hpp"
#include "protocol.hpp"
#include "network/byte_buf.hpp"
#include "network/network_connection.hpp"

//#define LOG_SYNC_MSGS

#ifdef LOG_SYNC_MSGS
#include <iostream>
#endif

namespace {
    constexpr int MAX_BLOCKS_AND_SEGMENTS_OUTSTANDING = 200;
    constexpr int TICK_SEGMENT_SIZE = 4000;   // in bytes
    constexpr int TICK_MARGIN_SEGMENTS = 20;
}

SyncHost::SyncHost(Coercri::NetworkConnection &conn,
                   KnightsVM &vm)
    : connection(conn),
      hashes_received(false),
      num_blocks_outstanding(0),
      num_tick_segments_outstanding(0)
{
    // Send the follower our VM config (i.e. register values and such like)
    std::vector<unsigned char> msg;
    Coercri::OutputByteBuf buf(msg);

#ifdef LOG_SYNC_MSGS
    std::cout << "Initial hash: " << std::hex << vm.getHash() << std::dec << std::endl;
    std::cout << "Sending LEADER_SEND_VM_CONFIG" << std::endl;
#endif

    buf.writeUbyte(LEADER_SEND_VM_CONFIG);
    vm.getVMConfig(buf);

    connection.send(msg);

    // Also save a copy of the VM's memory at this point in time
    memory_blocks = vm.getMemoryContents(HOST_MIGRATION_BLOCK_SHIFT);
}

// Returns true if sync done
bool SyncHost::processMessagesFromFollower(Coercri::InputByteBuf &buf)
{
    while (!buf.eof()) {
        int cmd = buf.readUbyte();
        switch (cmd) {
        case FOLLOWER_SEND_HASHES:
            receiveHashes(buf);
            break;

        case FOLLOWER_ACK_MEMORY_BLOCKS:
            receiveMemoryBlockAck(buf);
            break;

        case FOLLOWER_ACK_CATCHUP_TICKS:
            receiveCatchupTickAck(buf);
            break;

        default:
            throw std::runtime_error("Sync error (Invalid follower command)");
            break;
        }

        if (sendResponseToClient()) {
            return true;
        }
    }

    return false;
}

void SyncHost::addCatchupTicks(const std::vector<unsigned char> &tick_data)
{
    if (catchup_ticks_to_send.empty()
    || catchup_ticks_to_send.back().size() + tick_data.size() > TICK_SEGMENT_SIZE) {
        // Make a new segment
        catchup_ticks_to_send.push_back(tick_data);
    } else {
        // Append to the existing segment
        catchup_ticks_to_send.back().insert(catchup_ticks_to_send.back().end(),
                                            tick_data.begin(),
                                            tick_data.end());
    }
}

void SyncHost::receiveHashes(Coercri::InputByteBuf &buf)
{
#ifdef LOG_SYNC_MSGS
    std::cout << "Received FOLLOWER_SEND_HASHES" << std::endl;
#endif

    if (hashes_received) {
        throw std::runtime_error("Sync error (hashes already received)");
    }

    // Check which hashes match. Clear contents for any block whose
    // hash matches (we don't need to send that block).
    KnightsVM::compareMemoryHashes(buf, memory_blocks, HOST_MIGRATION_BLOCK_SHIFT);

    // Remove any dud (empty) blocks from the front of the queue, so that
    // the first "ready-to-send" block is at the front.
    trimMemoryBlocks();

    hashes_received = true;
}

void SyncHost::receiveMemoryBlockAck(Coercri::InputByteBuf &buf)
{
    int num_blocks_acked = buf.readVarInt();

#ifdef LOG_SYNC_MSGS
    std::cout << "Received FOLLOWER_ACK_MEMORY_BLOCKS for " << num_blocks_acked << " blocks" << std::endl;
#endif

    if (num_blocks_acked < 1 || num_blocks_acked > num_blocks_outstanding) {
        throw std::runtime_error("Sync error (Invalid ack message)");
    }

    num_blocks_outstanding -= num_blocks_acked;
}

void SyncHost::receiveCatchupTickAck(Coercri::InputByteBuf &buf)
{
    int num_tick_segments_acked = buf.readVarInt();

#ifdef LOG_SYNC_MSGS
    std::cout << "Received FOLLOWER_ACK_TICK_SEGMENTS for " << num_tick_segments_acked << " segments" << std::endl;
#endif

    if (num_tick_segments_acked < 1 || num_tick_segments_acked > num_tick_segments_outstanding) {
        throw std::runtime_error("Sync error (Invalid ack message)");
    }

    num_tick_segments_outstanding -= num_tick_segments_acked;
}

// Returns true if sync done
bool SyncHost::sendResponseToClient()
{
    std::vector<unsigned char> msg;
    Coercri::OutputByteBuf buf(msg);

    while (num_blocks_outstanding + num_tick_segments_outstanding < MAX_BLOCKS_AND_SEGMENTS_OUTSTANDING) {
        if (!memory_blocks.empty()) {

#ifdef LOG_SYNC_MSGS
            std::cout << "Sending LEADER_SEND_MEMORY_BLOCK for base addr " <<
                std::hex << memory_blocks.front().base_address << std::dec << std::endl;
#endif

            buf.writeUbyte(LEADER_SEND_MEMORY_BLOCK);
            KnightsVM::outputMemoryBlock(memory_blocks.front(), buf);

            connection.send(msg);
            msg.clear();

            memory_blocks.pop_front();
            trimMemoryBlocks();
            ++num_blocks_outstanding;

        } else if (!catchup_ticks_to_send.empty()) {

#ifdef LOG_SYNC_MSGS
            std::cout << "Sending LEADER_SEND_CATCHUP_TICKS for " << catchup_ticks_to_send.front().size() << " bytes" << std::endl;
#endif

            buf.writeUbyte(LEADER_SEND_CATCHUP_TICKS);
            buf.writeVarInt(catchup_ticks_to_send.front().size());
            for (unsigned char byte : catchup_ticks_to_send.front()) {
                buf.writeUbyte(byte);
            }

            connection.send(msg);
            msg.clear();

            catchup_ticks_to_send.pop_front();
            ++num_tick_segments_outstanding;

        } else {
            // Nothing left to send at this time
            break;
        }
    }

    // Only declare victory once all blocks acknowledged, and all tick segments
    // barring some margin acknowledged
    bool sync_done = false;
    if (num_blocks_outstanding == 0 && num_tick_segments_outstanding < TICK_MARGIN_SEGMENTS) {

#ifdef LOG_SYNC_MSGS
        std::cout << "Sending LEADER_SYNC_DONE" << std::endl;
#endif

        sync_done = true;
        buf.writeUbyte(LEADER_SYNC_DONE);
        connection.send(msg);
        msg.clear();
    }

    return sync_done;
}

// This pops any "empty" memory blocks from the front of the queue
void SyncHost::trimMemoryBlocks()
{
    while (!memory_blocks.empty() && memory_blocks.front().contents.empty()) {
        memory_blocks.pop_front();
    }
}

#endif  // USE_VM_LOBBY

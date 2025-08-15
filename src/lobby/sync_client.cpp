/*
 * sync_client.cpp
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

//#define LOG_SYNC_MSGS

#include "sync_client.hpp"
#include "protocol.hpp"
#include "knights_vm.hpp"
#include "network/byte_buf.hpp"
#include "network/network_connection.hpp"
#include <stdexcept>

#ifdef LOG_SYNC_MSGS
#include <iostream>
#endif

SyncClient::SyncClient(Coercri::NetworkConnection &conn,
                       KnightsVM &vm_)
    : connection(conn),
      vm(vm_),
      vm_config_received(false)
{}

bool SyncClient::processMessagesFromLeader(Coercri::InputByteBuf &buf,
                                           const std::vector<unsigned char> &in_msg,
                                           std::vector<unsigned char> &vm_output_data)
{
    int num_blocks_received = 0;
    int tick_segments_received = 0;
    bool done = false;

    std::vector<unsigned char> out_msg;
    Coercri::OutputByteBuf output(out_msg);

    while (!done && !buf.eof()) {
        int cmd = buf.readUbyte();
        switch (cmd) {
        case LEADER_SEND_VM_CONFIG:

#ifdef LOG_SYNC_MSGS
            std::cout << "Received LEADER_SEND_VM_CONFIG" << std::endl;
#endif

            if (vm_config_received) {
                throw std::runtime_error("Sync error (config already received)");
            }
            vm.putVMConfig(buf);

#ifdef LOG_SYNC_MSGS
            std::cout << "Sending FOLLOWER_SEND_HASHES" << std::endl;
#endif

            output.writeUbyte(FOLLOWER_SEND_HASHES);
            vm.getMemoryHashes(output, HOST_MIGRATION_BLOCK_SHIFT);

            vm_config_received = true;
            break;

        case LEADER_SEND_MEMORY_BLOCK:

#ifdef LOG_SYNC_MSGS
            //std::cout << "Received LEADER_SEND_MEMORY_BLOCK" << std::endl;
#endif

            if (!vm_config_received) {
                throw std::runtime_error("Sync error (config not yet received)");
            }
            vm.inputMemoryBlock(buf, HOST_MIGRATION_BLOCK_SHIFT);
            ++num_blocks_received;
            break;

        case LEADER_SEND_CATCHUP_TICKS:

#ifdef LOG_SYNC_MSGS
            std::cout << "Received LEADER_SEND_CATCHUP_TICKS" << std::endl;
            std::cout << "Hash: " << std::hex << vm.getHash() << std::dec << std::endl;
#endif

            if (!vm_config_received) {
                throw std::runtime_error("Sync error (config not yet received)");
            }
            receiveCatchupTicks(buf, in_msg, vm_output_data);
            ++tick_segments_received;
            break;

        case LEADER_SYNC_DONE:

#ifdef LOG_SYNC_MSGS
            std::cout << "Received LEADER_SYNC_DONE" << std::endl;
            std::cout << "Hash: " << std::hex << vm.getHash() << std::dec << std::endl;
#endif

            if (!vm_config_received) {
                throw std::runtime_error("Sync error (config not yet received)");
            }
            done = true;
            break;

        default:
            throw std::runtime_error("Sync error (Invalid leader command)");
        }
    }

    if (num_blocks_received > 0) {
#ifdef LOG_SYNC_MSGS
        std::cout << "Send FOLLOWER_ACK_MEMORY_BLOCKS " << num_blocks_received << std::endl;
#endif
        output.writeUbyte(FOLLOWER_ACK_MEMORY_BLOCKS);
        output.writeVarInt(num_blocks_received);
    }
    if (tick_segments_received > 0) {
#ifdef LOG_SYNC_MSGS
        std::cout << "Send FOLLOWER_ACK_CATCHUP_TICKS " << tick_segments_received << std::endl;
#endif
        output.writeUbyte(FOLLOWER_ACK_CATCHUP_TICKS);
        output.writeVarInt(tick_segments_received);
    }

    if (!out_msg.empty()) {
#ifdef LOG_SYNC_MSGS
        std::cout << "Sending output, " << out_msg.size() << " bytes" << std::endl;
#endif
        connection.send(out_msg);
    }

    return done;
}

void SyncClient::receiveCatchupTicks(Coercri::InputByteBuf &buf,
                                     const std::vector<unsigned char> &msg,
                                     std::vector<unsigned char> &vm_output_data)
{
    int length = buf.readVarInt();
    if (length <= 0 || buf.getPos() + length > msg.size()) {
        throw std::runtime_error("invalid length");
    }

    vm.runTicks(msg.data() + buf.getPos(),
                msg.data() + buf.getPos() + length,
                &vm_output_data);

    buf.skip(length);
}

#endif

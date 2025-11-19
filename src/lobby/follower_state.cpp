/*
 * follower_state.cpp
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
#include "protocol.hpp"
#include "sync_client.hpp"

// coercri includes
#include "network/byte_buf.hpp"
#include "network/network_connection.hpp"

// virtual_server includes
#include "tick_data.hpp"

FollowerState::FollowerState(std::unique_ptr<KnightsVM> vm,
                             boost::shared_ptr<Coercri::NetworkConnection> conn)
    : knights_vm(std::move(vm)),
      local_client_num(-1),
      connection_to_leader(conn),
      sync_client(std::make_unique<SyncClient>(*conn, *knights_vm))
{ }

FollowerState::~FollowerState()
{
    connection_to_leader->close();
}

void FollowerState::receiveClientMessages(std::vector<unsigned char> &data)
{
    data.clear();
    local_player_packets.swap(data);
}

void FollowerState::sendClientMessages(const std::vector<unsigned char> &data)
{
    if (data.empty()) return;

    std::vector<unsigned char> net_msg;
    Coercri::OutputByteBuf buf(net_msg);
    buf.writeUbyte(FOLLOWER_SEND_CLIENT_COMMANDS);
    buf.writeVarInt(data.size());
    net_msg.insert(net_msg.end(), data.begin(), data.end());

    if (sync_client) {
        delayed_packets.insert(delayed_packets.end(), net_msg.begin(), net_msg.end());
    } else {
        connection_to_leader->send(net_msg);
    }
}

class FollowerTickCallbacks : public TickCallbacks {
public:
    FollowerTickCallbacks(int local_client_num,
                          std::vector<unsigned char> &local_player_packets)
        : local_client_num(local_client_num),
          local_player_packets(local_player_packets)
    {}

    void onServerSendData(uint8_t client_num, std::vector<unsigned char> &data) override {
        if (client_num == local_client_num) {
            local_player_packets.insert(local_player_packets.end(), data.begin(), data.end());
        }
    }

private:
    int local_client_num;
    std::vector<unsigned char> & local_player_packets;
};

void FollowerState::update(Coercri::NetworkDriver &net_driver, Coercri::Timer &timer)
{
    // Receive messages from the leader
    std::vector<unsigned char> net_msg;
    connection_to_leader->receive(net_msg);
    Coercri::InputByteBuf buf(net_msg);

    // If we haven't received local client num yet, then that is the first byte
    // that the leader will send after connection
    if (local_client_num == -1 && !buf.eof()) {
        local_client_num = buf.readUbyte();
    }

    std::vector<unsigned char> vm_output_data;

    // Allow exceptions to propagate here. If we are following and we get some kind of
    // invalid message from the leader then there is little we can do -- we just have to
    // disconnect from the game.

    while (!buf.eof()) {

        if (sync_client) {
            bool done = sync_client->processMessagesFromLeader(buf, net_msg, vm_output_data);
            if (done) {
                sync_client.reset();
            }

        } else {
            switch (buf.readUbyte()) {
            case LEADER_SEND_TICK_DATA:
                {
                    int length = buf.readVarInt();

                    if (length <= 0 || buf.getPos() + length > net_msg.size()) {
                        throw std::runtime_error("invalid length");
                    }

                    // Send the tick(s) to the VM
                    knights_vm->runTicks(net_msg.data() + buf.getPos(),
                                         net_msg.data() + buf.getPos() + length,
                                         &vm_output_data);

                    // Grab latest checksums from the VM
                    std::vector<Checkpoint> checkpoints = knights_vm->getCheckpoints();
                    for (const Checkpoint & checkpoint : checkpoints) {
                        local_checkpoints.push(checkpoint);
                    }

                    buf.skip(length);
                }
                break;

            case LEADER_SEND_CHECKSUM:
                {
                    Checkpoint checkpoint;
                    checkpoint.timer_ms = buf.readUlong();
                    checkpoint.checksum = buf.readUlong();
                    checkpoint.checksum |= (uint64_t(buf.readUlong()) << 32);
                    leader_checkpoints.push(checkpoint);
                }
                break;

            default:
                throw std::runtime_error("invalid message from leader");
            }
        }

        checkForDesync();
    }

    // Now fish out any server messages meant for the local player, and
    // put them in local_player_packets.
    if (!vm_output_data.empty()) {
        const unsigned char *begin = vm_output_data.data();
        const unsigned char *end = begin + vm_output_data.size();
        FollowerTickCallbacks callbacks(local_client_num, local_player_packets);
        while (begin != end) {
            begin = ReadTickData(begin, end, callbacks);
        }
    }

    // Send any delayed packets if necessary
    if (!sync_client && !delayed_packets.empty()) {
        connection_to_leader->send(delayed_packets);
        delayed_packets.clear();
    }
}

void FollowerState::checkForDesync()
{
    while (!local_checkpoints.empty() && !leader_checkpoints.empty()) {
        if (local_checkpoints.front() != leader_checkpoints.front()) {
            throw std::runtime_error("Desync");
        }
        local_checkpoints.pop();
        leader_checkpoints.pop();
    }
}

std::unique_ptr<KnightsVM> FollowerState::migrate()
{
    // Run a final VM tick closing all connections
    std::vector<unsigned char> tick_data;
    TickWriter writer(tick_data);
    writer.writeCloseAllConnections();
    writer.finalize(1);
    knights_vm->runTicks(tick_data.data(), tick_data.data() + tick_data.size(), nullptr);

    // Return the VM to the caller
    return std::move(knights_vm);
}

#endif  // USE_VM_LOBBY

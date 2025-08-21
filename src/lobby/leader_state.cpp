/*
 * leader_state.cpp
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

// this folder includes
#include "follower_state.hpp"
#include "leader_state.hpp"
#include "sync_host.hpp"

// virtual_server includes
#include "knights_vm.hpp"

// protocol includes
#include "protocol.hpp"

// coercri includes
#include "network/byte_buf.hpp"
#include "network/network_connection.hpp"
#include "network/network_driver.hpp"
#include "timer/timer.hpp"

#include <cstring>
#include <random>

namespace {
    constexpr int MAX_FOLLOWERS = 20;
    const int SHORT_FLUSH_DELAY_MS = 30;
    const int LONG_FLUSH_DELAY_MS = 500;
    constexpr int PING_UPDATE_INTERVAL_MS = 3000;
}


//
// Constructors
//

LeaderState::LeaderState(Coercri::Timer &timer,
                         const PlayerID &local_user_id)
    : timer(timer)
{
    // Make a random seed
    std::random_device rd;
    std::vector<unsigned char> seed(32);
    for (int i = 0; i < seed.size(); ++i) {
        seed[i] = rd();
    }

    // Make a new KnightsVM
    knights_vm.reset(new KnightsVM(std::move(seed)));

    // Run an initial tick to force everything to load
    tick_writer.reset(new TickWriter(tick_data));
    tick_writer->finalize(0);
    int sleep_time_ms = knights_vm->runTicks(tick_data.data(), tick_data.data() + tick_data.size(), NULL);

    // Initialize everything else
    initialize(local_user_id, sleep_time_ms);
}

LeaderState::LeaderState(Coercri::Timer &timer,
                         const PlayerID &local_user_id,
                         std::unique_ptr<KnightsVM> vm)
    : timer(timer)
{
    knights_vm = std::move(vm);
    initialize(local_user_id, 1);
}

void LeaderState::initialize(const PlayerID &local_user_id, int sleep_time_ms)
{
    // Initialize tick data
    tick_data.clear();
    tick_writer.reset(new TickWriter(tick_data));
    current_tick_beginning_index = 0;
    tick_data_contains_output = false;

    // Make the local player connect (as client number 0)
    tick_writer->writeNewConnection(0, local_user_id);

    // Setup timers
    last_tick_time_ms = timer.getMsec();
    next_tick_deadline_ms = last_tick_time_ms + sleep_time_ms;
    last_flush_time_ms = last_tick_time_ms;
    last_ping_update_ms = last_tick_time_ms;

    // Add dummy entries to follower vectors
    followers.push_back(boost::shared_ptr<Coercri::NetworkConnection>());
    follower_sync.push_back(std::unique_ptr<SyncHost>());
}

LeaderState::~LeaderState()
{
    for (auto & conn : followers) {
        if (conn) {
            conn->close();
        }
    }
}


//
// Send and receive client data
//

void LeaderState::receiveClientMessages(std::vector<unsigned char> &data)
{
    // Output the cached packets for the local player, and clear the local cache
    data.clear();
    data.swap(local_player_packets);
}

void LeaderState::sendClientMessages(const std::vector<unsigned char> &data)
{
    // Add the local client's commands to the next tick
    tick_writer->writeClientSendData(0, data);
}


//
// Update method
//

void LeaderState::update(Coercri::NetworkDriver &net_driver,
                         Coercri::Timer &timer)
{
    // Check for new incoming connections
    {
        Coercri::NetworkDriver::Connections new_conns = net_driver.pollIncomingConnections();
        for (auto & conn : new_conns) {
            // Only accept the new connection if we have room
            if (followers.size() < MAX_FOLLOWERS) {

                // Find a slot
                int client_num = 0;
                while (clientNumInUse(client_num)) {
                    ++client_num;
                }

                // Send the client_num as the first byte sent to the new connection
                std::vector<unsigned char> client_num_byte(1, (unsigned char)client_num);
                conn->send(client_num_byte);

                // Open a new connection to the VM
                // Note: getAddress, by convention, returns the platform user ID
                // for platform P2P connections
                tick_writer->writeNewConnection(client_num, PlayerID(conn->getAddress()));

                // Add the new follower
                if (client_num >= followers.size()) {
                    followers.push_back(conn);
                    follower_sync.push_back(std::make_unique<SyncHost>(*conn, *knights_vm));
                } else {
                    followers[client_num] = conn;
                    follower_sync[client_num] = std::make_unique<SyncHost>(*conn, *knights_vm);
                }
            }
        }
    }

    // Receive client commands from followers, and check for disconnections
    for (int client_num = 0; client_num < followers.size(); ++client_num) {
        if (followers[client_num]) {

            bool error = false;

            try {
                if (followers[client_num]->getState() == Coercri::NetworkConnection::CONNECTED) {
                    receiveFollowerMessages(client_num, *followers[client_num]);
                } else {
                    error = true;
                }
            } catch (...) {
                error = true;
            }

            if (error) {
                // Remove this follower
                tick_writer->writeCloseConnection(client_num);
                followers[client_num].reset();
                follower_sync[client_num].reset();
            }
        }
    }

    // Run a VM tick if required
    unsigned int time_now_ms = timer.getMsec();
    if (int(time_now_ms - next_tick_deadline_ms) >= 0 || tick_writer->wasMessageWritten()) {

        // Add ping time report if required
        if (int(time_now_ms - last_ping_update_ms) >= PING_UPDATE_INTERVAL_MS) {
            for (int client_num = 0; client_num < followers.size(); ++client_num) {
                if (followers[client_num]) {
                    int ping_time_ms = followers[client_num]->getPingTime();
                    tick_writer->writeClientPingReport(client_num, ping_time_ms);
                }
            }
            last_ping_update_ms = time_now_ms;
        }

        // Finalize the current tick
        tick_writer->finalize(std::min((unsigned int)TickWriter::MAX_TICK_MS,
                                       time_now_ms - last_tick_time_ms));

        // Send the tick to the VM
        std::vector<unsigned char> vm_output_data;
        int sleep_time_ms =
            knights_vm->runTicks(tick_data.data() + current_tick_beginning_index,
                                 tick_data.data() + tick_data.size(),
                                 &vm_output_data);

        // Check the output, routing data to local players if needed
        bool this_tick_contains_output = processVmOutputData(vm_output_data);
        tick_data_contains_output = tick_data_contains_output || this_tick_contains_output;

        // Set up for next tick
        current_tick_beginning_index = tick_data.size();
        tick_writer.reset(new TickWriter(tick_data));
        last_tick_time_ms = time_now_ms;
        next_tick_deadline_ms = time_now_ms + sleep_time_ms;
    }

    // Flush tick data to followers (so that they can keep up their
    // local VM in step with ours), if required.
    int time_since_last_flush_ms = time_now_ms - last_flush_time_ms;
    int required_delay_ms = tick_data_contains_output ? SHORT_FLUSH_DELAY_MS : LONG_FLUSH_DELAY_MS;
    if (time_since_last_flush_ms > required_delay_ms && !tick_data.empty()) {

        // Prepare the message for sending
        std::vector<unsigned char> msg;
        Coercri::OutputByteBuf buf(msg);
        buf.writeUbyte(LEADER_SEND_TICK_DATA);
        buf.writeVarInt(tick_data.size());
        msg.insert(msg.end(), tick_data.begin(), tick_data.end());

        // Send it to all followers
        for (size_t i = 0; i < followers.size(); ++i) {
            if (followers[i]) {
                if (follower_sync[i]) {
                    // Add to sync queue
                    follower_sync[i]->addCatchupTicks(tick_data);
                } else {
                    // Send immediately
                    followers[i]->send(msg);
                }
            }
        }

        // Reset for the next batch of ticks
        tick_data.clear();
        current_tick_beginning_index = 0;
        tick_data_contains_output = false;
        tick_writer.reset(new TickWriter(tick_data));
    }
}

bool LeaderState::clientNumInUse(int client_num) const
{
    return client_num == 0
        || (client_num < followers.size() && followers[client_num]);
}


//
// Interpreting network messages from followers
//

void LeaderState::receiveFollowerMessages(int client_num, Coercri::NetworkConnection &connection)
{
    std::vector<unsigned char> net_msg;
    connection.receive(net_msg);
    Coercri::InputByteBuf buf(net_msg);

    std::vector<unsigned char> tmp;

    while (!buf.eof()) {

        if (follower_sync[client_num]) {
            bool done = follower_sync[client_num]->processMessagesFromFollower(buf);
            if (done) {
                follower_sync[client_num].reset();
            }

        } else {
            switch (buf.readUbyte()) {
            case FOLLOWER_SEND_CLIENT_COMMANDS:
                {
                    // Receive client commands from client
                    int length = buf.readVarInt();

                    if (length < 1) {
                        throw std::runtime_error("invalid length");
                    }

                    tmp.clear();
                    while (length > 0) {
                        tmp.push_back(buf.readUbyte());
                        --length;
                    }

                    // Add client commands to the next tick
                    tick_writer->writeClientSendData(client_num, tmp);
                }
                break;

            case FOLLOWER_ACK_CATCHUP_TICKS:
                // Ignore this message
                buf.readVarInt();
                break;

            default:
                throw std::runtime_error("invalid command byte");
            }
        }
    }
}


//
// Processing VM output data
//

class LeaderTickCallbacks : public TickCallbacks {
public:
    LeaderTickCallbacks(const int local_client_num,
                        std::vector<unsigned char> &local_player_packets)
        : local_client_num(local_client_num),
          local_player_packets(local_player_packets),
          data_sent(false)
    {}

    void onServerSendData(uint8_t client_num, std::vector<unsigned char> &data) override {
        if (client_num == local_client_num) {
            local_player_packets.insert(local_player_packets.end(), data.begin(), data.end());
            data_sent = true;
        }
    }

    bool dataWasSent() const {
        return data_sent;
    }

private:
    int local_client_num;
    std::vector<unsigned char> &local_player_packets;
    bool data_sent;
};

bool LeaderState::processVmOutputData(const std::vector<unsigned char> &vm_output_data)
{
    if (vm_output_data.empty()) {
        return false;
    }

    LeaderTickCallbacks callbacks(0, local_player_packets);
    ReadTickData(vm_output_data.data(), vm_output_data.data() + vm_output_data.size(), callbacks);
    return callbacks.dataWasSent();
}


//
// Migrate method
//

std::unique_ptr<KnightsVM> LeaderState::migrate()
{
    // Run a final tick in which all connections are closed
    tick_writer->writeCloseAllConnections();
    tick_writer->finalize(1);
    knights_vm->runTicks(tick_data.data() + current_tick_beginning_index,
                         tick_data.data() + tick_data.size(),
                         nullptr);

    // Return the KnightsVM to the caller
    return std::move(knights_vm);
}


#endif  // USE_VM_LOBBY

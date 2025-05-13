/*
 * knights_virtual_server.cpp
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

// Test application for the Knights virtual server. This acts as a
// standalone Knights server, except that instead of running the
// server natively, it listens for UDP packets on port 16399 and feeds
// them into a KnightsVM instance.

#include "enet/enet_network_driver.hpp"
#include "network/network_connection.hpp"
#include "timer/generic_timer.hpp"
#include "knights_vm.hpp"
#include "rstream.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>


// TickNetwork class
// This handles all incoming and outgoing packets and creates the tick data
// for the KnightsVM.
class TickNetwork : private TickCallbacks {
public:
    // Constructor - starts listening on the Knights UDP port
    TickNetwork();

    // Handle network events, and prepare next tick if available.
    // If this returns an empty vector then nothing of interest has occurred.
    // If it returns a vector, then that is the tick_data for the next tick.
    // (Set force=true to force it to return a tick regardless of network activity.)
    std::vector<unsigned char> prepareTickData(unsigned int time_since_last_tick,
                                               bool force);

    // Handle the "vm_output_data" that comes back from the VM after a tick.
    // This will send packets out to the network if applicable.
    void handleVMOutput(const std::vector<unsigned char> &vm_output_data);

private:
    // TickCallbacks implementation:
    // This handles data sent from server to clients
    void onServerSendData(uint8_t client_number, std::vector<unsigned char> &data);

private:
    // Network driver and connections
    std::unique_ptr<Coercri::NetworkDriver> net_driver;
    std::vector<boost::shared_ptr<Coercri::NetworkConnection>> client_connections;
};

TickNetwork::TickNetwork()
    : net_driver(new Coercri::EnetNetworkDriver(20, 0, true))
{
    net_driver->setServerPort(16399);
    net_driver->enableServer(true);
}

std::vector<unsigned char> TickNetwork::prepareTickData(unsigned int time_since_last_tick,
                                                        bool force)
{
    // Make an empty tick data buffer
    std::vector<unsigned char> tick_data;
    TickWriter writer(tick_data, time_since_last_tick);

    // Pump network events
    while (net_driver->doEvents()) {}

    // Handle new connections since the previous tick
    std::vector<boost::shared_ptr<Coercri::NetworkConnection>> connections;
    connections = net_driver->pollIncomingConnections();
    for (auto conn : connections) {
        // Find a slot for this new connection
        int idx = 0;
        while (idx < client_connections.size() && client_connections[idx]) {
            ++idx;
        }
        if (idx == client_connections.size()) {
            if (idx == 256) {
                // There is no client number 256 (client numbers must fit in a uint8_t)
                // so throw an error instead
                throw std::runtime_error("Too many client connections");
            }
            client_connections.push_back(conn);
        } else {
            client_connections[idx] = conn;
        }
        // NOTE: We set platform_user_id to "" for ENet connections.
        writer.writeNewConnection(idx, "");
    }

    // Process existing connections
    std::vector<unsigned char> buffer;
    for (int idx = 0; idx < client_connections.size(); ++idx) {
        boost::shared_ptr<Coercri::NetworkConnection> conn = client_connections[idx];
        if (conn) {
            conn->receive(buffer);
            if (!buffer.empty()) {
                writer.writeClientSendData(idx, buffer);
            }
            if (conn->getState() != Coercri::NetworkConnection::CONNECTED) {
                writer.writeCloseConnection(idx);
                client_connections[idx].reset();
            }
        }
    }

    // If we wrote at least one message, or force was true, then accept the new tick,
    // otherwise just clear the tick_data and we will try again next time.
    if (writer.wasMessageWritten() || force) {
        writer.finalize();
    } else {
        tick_data.clear();
    }

    // Return the result.
    return tick_data;
}

void TickNetwork::handleVMOutput(const std::vector<unsigned char> &vm_output_data)
{
    // Process the output data, invoking callbacks as needed
    if (!vm_output_data.empty()) {
        const unsigned char *ptr = ReadTickData(vm_output_data.data(),
                                                vm_output_data.data() + vm_output_data.size(),
                                                *this);
        if (ptr != vm_output_data.data() + vm_output_data.size()) {
            // This is unexpected.
            throw std::runtime_error("Not all VM output data was processed");
        }
    }
}

void TickNetwork::onServerSendData(uint8_t client_number, std::vector<unsigned char> &data)
{
    // This is called by ReadTickData for each data-packet that the server wanted to send
    if (client_number < client_connections.size() && client_connections[client_number]) {
        client_connections[client_number]->send(data);
    }
}


// Main function.

int main(int argc, const char **argv)
{
    boost::filesystem::path data_dir = "knights_data";
    if (argc > 1) {
        data_dir = argv[1];
    }

    RStream::Initialize(data_dir);

    TickNetwork network;

    Coercri::GenericTimer timer;
    unsigned int time_of_last_tick = timer.getMsec();
    unsigned int force_time = 0;  // Time (relative to last tick) at which next tick should be forced.

    KnightsVM vm(time_of_last_tick);

    // Run an initial tick to force everything to load.
    {
        unsigned int time0 = timer.getMsec();
        std::vector<unsigned char> tick_data;
        TickWriter writer(tick_data, 0);
        writer.finalize();
        vm.runTick(tick_data.data(), tick_data.data() + tick_data.size(), NULL);
        unsigned int time1 = timer.getMsec();
        std::cout << "Server running. Init took " << time1 - time0 << " ms." << std::endl;
    }

    while (true) {
        // Calculate how long it has been since the last tick, and
        // whether we should force the next tick.
        unsigned int new_time = timer.getMsec();
        unsigned int time_since_last_tick = new_time - time_of_last_tick;
        bool force = (time_since_last_tick >= force_time);

        // Upper bound on tick length of 1000 ms.
        if (time_since_last_tick > 1000) time_since_last_tick = 1000;

        // Do network operations, maybe extract a tick.
        std::vector<unsigned char> tick_data = network.prepareTickData(time_since_last_tick, force);

        // If a tick was extracted:
        if (!tick_data.empty()) {
            // Run the tick. This gives us the new value of "force_time".
            std::vector<unsigned char> vm_output_data;
            force_time = vm.runTick(tick_data.data(),
                                    tick_data.data() + tick_data.size(),
                                    &vm_output_data);

            // Push any output data to the network.
            network.handleVMOutput(vm_output_data);

            // Update time_of_last_tick.
            time_of_last_tick = new_time;
        }

        // The tick might have taken some time to run, so let's get an updated time
        unsigned int time_now = timer.getMsec();

        // Calculate how long to go until the next forced tick
        int time_until_force = (time_of_last_tick + force_time) - time_now;

        // If it is more than zero, then we should sleep until the forced tick time
        // (but: sleep no more than 10 ms, to remain responsive to network inputs).
        if (time_until_force >= 0) {
            int sleep_time = 10;
            if (time_until_force < sleep_time) {
                sleep_time = time_until_force;
            }

            timer.sleepMsec(sleep_time);
        }
    }
}

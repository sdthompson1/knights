/*
 * vm_main.cpp
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

// Main program for the Knights Virtual Server.

// This should be compiled to a RISC-V executable which is then used
// to produce risc_vm.hpp and risc_vm.cpp
// (see Makefile in this directory).

#include "knights_config.hpp"
#include "knights_server.hpp"
#include "rng.hpp"
#include "rstream.hpp"
#include "syscalls.hpp"
#include "tick_data.hpp"
#include "timer/timer.hpp"

#include <iostream>
#include <random>

// Timer class for the VM
class VMTimer : public Coercri::Timer {
public:
    virtual unsigned int getMsec();
    virtual uint64_t getUsec() { return static_cast<uint64_t>(getMsec()) * UINT64_C(1000); }
    virtual void sleepMsec(int msec) { /* not used */ }
    virtual void sleepUsec(int64_t usec) { /* not used */ }
};

unsigned int VMTimer::getMsec()
{
    struct {
        unsigned int sec;
        unsigned int dummy;
        unsigned int nsec;
    } tspec;
    asm volatile(
        "li a0, 0\n\t"     // Clock number (irrelevant to our implementation)
        "mv a1, %0\n\t"    // Where to place the result
        "li a7, 403\n\t"   // Syscall number (clock_gettime)
        "ecall\n\t"        // Make syscall (we don't care about the result)
        : // No outputs
        : "r" (&tspec)      // Input: &tspec
        : "a0", "a1", "a7"  // Trashed registers
    );
    return tspec.sec * 1000u + tspec.nsec / 1000000u;
}


// Tick callbacks
class VMTickCallbacks : public TickCallbacks {
public:
    VMTickCallbacks(KnightsServer &server_,
                    std::vector<ServerConnection*> & conns)
        : server(server_), connections(conns) {}
    void onNewConnection(uint8_t client_number, const PlayerID &platform_user_id) override;
    void onCloseConnection(uint8_t client_number) override;
    void onCloseAllConnections() override;
    void onClientSendData(uint8_t client_number, std::vector<unsigned char> &data) override;
    void onClientPingReport(uint8_t client_number, uint16_t ping_time_ms) override;
private:
    KnightsServer &server;
    std::vector<ServerConnection*> &connections;
};

void VMTickCallbacks::onNewConnection(uint8_t num, const PlayerID &platform_user_id)
{
    if (connections.size() <= num) {
        connections.resize(int(num) + 1);
    }
    if (connections[num] == NULL) {
        connections[num] = &server.newClientConnection("", platform_user_id);
    } else {
        throw std::runtime_error("onNewConnection error");
    }
}

void VMTickCallbacks::onCloseConnection(uint8_t num)
{
    if (num < connections.size() && connections[num] != NULL) {
        server.connectionClosed(*connections[num]);
        connections[num] = NULL;
    } else {
        throw std::runtime_error("onCloseConnection error");
    }
}

void VMTickCallbacks::onCloseAllConnections()
{
    for (auto& conn : connections) {
        if (conn) {
            server.connectionClosed(*conn);
            conn = NULL;
        }
    }
}

void VMTickCallbacks::onClientSendData(uint8_t num, std::vector<unsigned char> &data)
{
    if (num < connections.size() && connections[num] != NULL) {
        server.receiveInputData(*connections[num], data);
    } else {
        throw std::runtime_error("onClientSendData error");
    }
}

void VMTickCallbacks::onClientPingReport(uint8_t num, uint16_t ping_time_ms)
{
    if (num < connections.size() && connections[num] != NULL) {
        server.setPingTime(*connections[num], ping_time_ms);
    } else {
        throw std::runtime_error("onClientPingReport error (client_num " + std::to_string(num) + ")");
    }
}

int main()
{
    try {

        // Create a Timer.
        boost::shared_ptr<Coercri::Timer> timer(new VMTimer);

        // Seed the RNG
        {
            unsigned char buf[32];
            vs_get_random_data(buf, sizeof(buf));
            g_rng.initialize(buf, sizeof(buf));
        }

        // Create the KnightsServer.
        KnightsServer server(timer,
                             false,    // don't allow split screen
                             "",       // no motd file
                             "");      // no old_motd file

        // Create a single KnightsGame on the server.
        boost::shared_ptr<KnightsConfig> config(new KnightsConfig("main.lua", false));
        server.startNewGame(config, "#VMGame");

        // Create VMTickCallbacks
        std::vector<ServerConnection*> connections;
        VMTickCallbacks callbacks(server, connections);

        // Enter main loop
        while (true) {
            // Read tick data into a vector.
            std::fstream tick_file("TICK:", std::ios::in | std::ios::out | std::ios::binary);
            std::vector<unsigned char> tick_data{std::istreambuf_iterator<char>(tick_file),
                                                 std::istreambuf_iterator<char>()};

            // Clear any error flags
            tick_file.clear();

            // Process the tick data, making callbacks as needed.
            if (tick_data.empty()) throw std::runtime_error("Empty tick data");
            ReadTickData(tick_data.data(), tick_data.data() + tick_data.size(), callbacks);

            // Now run the game thread, if applicable
            unsigned int sleep_time = 200;
            if (vs_game_thread_running()) {
                sleep_time = vs_switch_to_game_thread();
            }

            // Now write outgoing data
            tick_data.clear();  // Re-use same vector for the output
            std::vector<unsigned char> buffer;   // Temp buffer for network packets
            TickWriter writer(tick_data);
            for (int i = 0; i < connections.size(); ++i) {
                if (connections[i]) {
                    server.getOutputData(*connections[i], buffer);
                    if (!buffer.empty()) {
                        writer.writeServerSendData(i, buffer);
                    }
                }
            }
            writer.finalize(0);

            // Send the vm_output_data to "TICK:" file
            tick_file.write((const char*)tick_data.data(), tick_data.size());

            // We can close the TICK: file as we are now done with it
            tick_file.close();

            // End current tick.
            vs_end_tick(sleep_time);
        }

    } catch (const std::exception &e) {
        std::cerr << "VM: Caught exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "VM: Caught unknown exception" << std::endl;
    }

    return 0;
}

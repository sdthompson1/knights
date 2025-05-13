/*
 * tick_data.hpp
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

#ifndef TICK_DATA_HPP
#define TICK_DATA_HPP

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

enum { MAX_TICK_DURATION_MS = 30 * 1000 };

// Callbacks interface for use with TickReader.
class TickCallbacks {
public:
    virtual ~TickCallbacks();

    // Called when a new tick begins.
    // Timer should be advanced by the given amount.
    virtual void onNewTick(unsigned int tick_duration_ms) { }

    // Called when a new client opens a connection to the server.
    virtual void onNewConnection(uint8_t client_number, const std::string &platform_user_id) { }

    // Called when client closes their existing connection.
    virtual void onCloseConnection(uint8_t client_number) { }

    // Called when a client sends data to the server.
    // The implementation of this function may 'swap' the data to
    // another vector if they wish (to avoid copying).
    virtual void onClientSendData(uint8_t client_number, std::vector<unsigned char> &data) { }

    // Called when a client reports their ping time
    virtual void onClientPingReport(uint8_t client_number, uint16_t ping_time_ms) { }

    // Called when the server sends data to a client.
    // The implementation of this function may 'swap' the data to
    // another vector if they wish (to avoid copying).
    virtual void onServerSendData(uint8_t client_number, std::vector<unsigned char> &data) { }
};

// Interpret tick data for a single tick.
// Make callbacks as appropriate based on the data received.
// Returns pointer to start of the next tick.
const unsigned char * ReadTickData(const unsigned char *tick_data_begin,
                                   const unsigned char *tick_data_end,
                                   TickCallbacks &callbacks);


// Class that APPENDS tick data to a given vector<unsigned char>.
class TickWriter {
public:
    // The vector should live for at least as long as the TickWriter, and no-one else
    // should be writing the vector while the TickWriter is alive.
    // Tick duration should be between 0 and 1000 ms inclusive.
    TickWriter(std::vector<unsigned char> &tick_data, int tick_duration_ms);

    // Write tick messages - these are the mirror images of the "on" methods in TickCallbacks.
    void writeNewConnection(uint8_t client_number, const std::string &platform_user_id);
    void writeCloseConnection(uint8_t client_number);
    void writeClientSendData(uint8_t client_number, const std::vector<unsigned char> &data);
    void writeClientPingReport(uint8_t client_number, uint16_t ping_time_ms);
    void writeServerSendData(uint8_t client_number, const std::vector<unsigned char> &data);

    // True if at least one "write" function was called
    bool wasMessageWritten() const { return last_msg_pos != -1; }

    // Finalize - this MUST be called before using the tick_data buffer.
    void finalize();

private:
    void beginNewMessage(int msg_type, int payload_length, uint8_t client_num);

private:
    std::vector<unsigned char> &tick_data;
    int last_msg_pos;      // -1 if no msgs written yet
    int tick_duration_ms;
};

#endif

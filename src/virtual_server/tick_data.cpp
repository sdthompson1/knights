/*
 * tick_data.cpp
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

#include "tick_data.hpp"

#include <stdexcept>

namespace {
    const int MAX_LENGTH = 0x3fffff;   // approx 4 million

    // Byte Format for initial byte of a tick message:
    // Bit 7 = true if more messages follow.
    // Bits 6-3 = payload length if applicable. (All 1's means length is a varInt sent separately.)
    // Bits 2-0 = TickMessage code (room for upto 8 messages).
    enum TickMessage {
        TM_NEW_CONNECTION = 0,
        TM_CLOSE_CONNECTION = 1,
        TM_CLIENT_SEND_DATA = 2,
        TM_CLIENT_PING_REPORT = 3,
        TM_SERVER_SEND_DATA = 4
    };

    unsigned char ReadUbyte(const unsigned char *&first,
                            const unsigned char *last)
    {
        if (first == last) {
            throw std::runtime_error("Tick data format error");
        }
        return *first++;
    }

    // Returns an integer between 0 and MAX_LENGTH inclusive
    int ReadLength(const unsigned char *&first,
                   const unsigned char *last)
    {
        unsigned char x = ReadUbyte(first, last);
        unsigned char y = 0;
        unsigned char z = 0;
        if (x & 0x80) {
            y = ReadUbyte(first, last);
            if (y & 0x80) {
                z = ReadUbyte(first, last);
            }
        }
        return (z << 14) | ((y & 0x7f) << 7) | (x & 0x7f);
    }

    void PushBackLength(std::vector<unsigned char> &vec,
                        unsigned int length)
    {
        if (length > MAX_LENGTH) {
            throw std::runtime_error("invalid length");
        }
        unsigned char x = (length & 0x7f);
        unsigned char y = ((length >> 7) & 0x7f);
        unsigned char z = ((length >> 14) & 0xff);
        if (y != 0 || z != 0) x |= 0x80;
        if (z != 0) y |= 0x80;
        vec.push_back(x);
        if (x & 0x80) vec.push_back(y);
        if (y & 0x80) vec.push_back(z);
    }

    std::string ReadString(const unsigned char *&first,
                           const unsigned char *last,
                           int payload_length)
    {
        std::string s;
        s.resize(payload_length);
        for (std::string::iterator it = s.begin(); it != s.end(); ++it) {
            *it = static_cast<char>(ReadUbyte(first, last));
        }
        return s;
    }

    std::vector<unsigned char> ReadVector(const unsigned char *&first,
                                          const unsigned char *last,
                                          int payload_length)
    {
        std::vector<unsigned char> v;
        v.reserve(payload_length);
        for (int i = 0; i < payload_length; ++i) {
            v.push_back(ReadUbyte(first, last));
        }
        return v;
    }
}

TickCallbacks::~TickCallbacks()
{
    // Defined here (rather than in header file) to avoid link error
}

const unsigned char * ReadTickData(const unsigned char *ptr,
                                   const unsigned char *end,
                                   TickCallbacks &callbacks)
{
    // If ptr==end there is no tick to process.
    if (ptr == end) return ptr;

    // First we expect a Length giving the tick duration, shifted left one bit.
    // Also, if bit 0 is set, this indicates that there are tick messages.
    int duration = ReadLength(ptr, end);
    bool more_messages = (duration & 1);
    duration >>= 1;

    // Sanity check tick duration
    if (duration < 0 || duration > 1000) {
        throw std::runtime_error("Invalid tick duration");
    }

    // Send the "onNewTick" callback.
    callbacks.onNewTick(duration);

    // While there are tick messages to read:
    while (more_messages) {
        // Read initial byte which encodes more_messages flag, payload length
        // and message type.
        int byte = ReadUbyte(ptr, end);

        more_messages = ((byte & 0x80) != 0);
        int payload_length = ((byte >> 3) & 0xf);
        int message_type = (byte & 0x7);

        if (payload_length == 0xf) {
            // Payload length of 0xf means the payload length is encoded separately as a Length
            payload_length = ReadLength(ptr, end);
        }

        // All message types require a client number, so might as well decode this now
        uint8_t client_num = ReadUbyte(ptr, end);

        switch (message_type) {
        case TM_NEW_CONNECTION:
            {
                std::string s = ReadString(ptr, end, payload_length);
                callbacks.onNewConnection(client_num, s);
            }
            break;

        case TM_CLOSE_CONNECTION:
            callbacks.onCloseConnection(client_num);
            break;

        case TM_CLIENT_SEND_DATA:
            {
                std::vector<unsigned char> v = ReadVector(ptr, end, payload_length);
                callbacks.onClientSendData(client_num, v);
            }
            break;

        case TM_CLIENT_PING_REPORT:
            // Ping time is encoded in the payload length field
            callbacks.onClientPingReport(client_num, payload_length);
            break;

        case TM_SERVER_SEND_DATA:
            {
                std::vector<unsigned char> v = ReadVector(ptr, end, payload_length);
                callbacks.onServerSendData(client_num, v);
            }
            break;

        default:
            throw std::runtime_error("Invalid tick message");
        }
    }

    // Return final buffer position
    return ptr;
}

TickWriter::TickWriter(std::vector<unsigned char> &tick_data_, int tick_duration_ms)
    : tick_data(tick_data_),
      last_msg_pos(-1),
      tick_duration_ms(tick_duration_ms)
{
    // Sanity checks
    if (tick_duration_ms < 0) {
        throw std::runtime_error("Tick duration cannot be negative");
    }
    if (tick_duration_ms > 1000) {
        // Clip tick duration at 1 second to prevent overflows etc.
        tick_duration_ms = 1000;
    }
}

void TickWriter::beginNewMessage(int msg_type, int payload_length, uint8_t client_num)
{
    if (last_msg_pos == -1) {
        // No messages written yet, so let's write the tick duration header first.
        // Set bit 0 to indicate presence of messages.
        PushBackLength(tick_data, (tick_duration_ms << 1) | 1);
    }

    // Sanity check that the tick buffer hasn't got too long
    if (tick_data.size() >= MAX_LENGTH) {
        throw std::runtime_error("Tick data too long");
    }

    // Now write the msg byte, on assumption that there will be further messages.
    // Also record the position of this byte in the buffer.
    last_msg_pos = tick_data.size();

    bool need_separate_payload_length = false;

    unsigned char byte = 0x80;  // "more_messages" flag set.
    if (payload_length >= 15) {
        byte |= 0x78;   // Bits 6-3 set
        need_separate_payload_length = true;
    } else {
        byte |= (payload_length << 3);
    }
    byte |= msg_type;

    tick_data.push_back(byte);

    if (need_separate_payload_length) {
        PushBackLength(tick_data, payload_length);
    }

    // Client num byte follows next.
    tick_data.push_back(client_num);
}

void TickWriter::finalize()
{
    if (last_msg_pos == -1) {
        // No messages written, so write the tick duration, with bit 0 clear to
        // indicate no messages present.
        PushBackLength(tick_data, tick_duration_ms << 1);
    } else {
        // At least one message was written, so we need to go back and clear the
        // "more_messages" bit for that message.
        tick_data[last_msg_pos] ^= 0x80;
    }
}

void TickWriter::writeNewConnection(uint8_t client_num, const std::string &platform_user_id)
{
    beginNewMessage(TM_NEW_CONNECTION, platform_user_id.size(), client_num);
    for (char c : platform_user_id) {
        tick_data.push_back(c);
    }
}

void TickWriter::writeCloseConnection(uint8_t client_num)
{
    beginNewMessage(TM_CLOSE_CONNECTION, 0, client_num);
}

void TickWriter::writeClientSendData(uint8_t client_num, const std::vector<unsigned char> &data)
{
    beginNewMessage(TM_CLIENT_SEND_DATA, data.size(), client_num);
    for (unsigned char c : data) {
        tick_data.push_back(c);
    }
}

void TickWriter::writeClientPingReport(uint8_t client_num, uint16_t ping_time_ms)
{
    // We encode the ping time in the "payload length" field
    beginNewMessage(TM_CLIENT_PING_REPORT, ping_time_ms, client_num);
}

void TickWriter::writeServerSendData(uint8_t client_num, const std::vector<unsigned char> &data)
{
    beginNewMessage(TM_SERVER_SEND_DATA, data.size(), client_num);
    for (unsigned char c : data) {
        tick_data.push_back(c);
    }
}

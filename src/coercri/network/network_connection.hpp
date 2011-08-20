/*
 * FILE:
 *   network_connection.hpp
 *
 * PURPOSE:
 *   High level networking interface. Implements a reliable, TCP-like
 *   protocol for games.
 * 
 *   The NetworkConnection object represents a single connection to a
 *   foreign host. Can send and receive packets. Note: data may not be
 *   actually sent/received until NetworkDriver::pollEvents is called.
 *
 * AUTHOR:
 *   Stephen Thompson
 *
 * COPYRIGHT:
 *   Copyright (C) Stephen Thompson, 2008 - 2009.
 *
 *   This file is part of the "Coercri" software library. Usage of "Coercri"
 *   is permitted under the terms of the Boost Software License, Version 1.0, 
 *   the text of which is displayed below.
 *
 *   Boost Software License - Version 1.0 - August 17th, 2003
 *
 *   Permission is hereby granted, free of charge, to any person or organization
 *   obtaining a copy of the software and accompanying documentation covered by
 *   this license (the "Software") to use, reproduce, display, distribute,
 *   execute, and transmit the Software, and to prepare derivative works of the
 *   Software, and to permit third-parties to whom the Software is furnished to
 *   do so, all subject to the following:
 *
 *   The copyright notices in the Software and this entire statement, including
 *   the above license grant, this restriction and the following disclaimer,
 *   must be included in all copies of the Software, in whole or in part, and
 *   all derivative works of the Software, unless such copies or derivative
 *   works are solely in the form of machine-executable object code generated by
 *   a source language processor.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 *   SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 *   FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *   DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef COERCRI_NETWORK_CONNECTION_HPP
#define COERCRI_NETWORK_CONNECTION_HPP

#include <string>
#include <vector>

namespace Coercri {

    class NetworkConnection {
    public:
        virtual ~NetworkConnection() { }

        // Possible connection states
        enum State {
            PENDING,    // (Client side only) Awaiting connection to server
            CONNECTED,  // Ready to send/receive data
            CLOSED,     // Connection closed (either by local or remote close() call, or by a timeout).
            FAILED      // The connection request timed out or was refused.
        };

        
        // Query current connection state
        // NOTE: a CLOSED or FAILED connection may still be able to read data (if there has been buffering).
        // In particular it is always possible to read to the end of the current packet.
        virtual State getState() const = 0;

        
        // Close the connection. Note that we wait for any pending
        // data to be sent before actually closing.
        virtual void close() = 0;

        
        // Read data.
        //
        // Will return all received packets concatenated together into
        // a single vector (in the order they were sent). Packets will
        // never be broken up though (i.e. each packet is always
        // received either whole or not at all).
        //
        // If no data is available to be read, an empty vector is
        // returned.
        //
        // Existing contents of the vector are replaced.
        //
        virtual void receive(std::vector<unsigned char> &) = 0;

        
        // Send data.
        // The contents of the vector are treated as a single packet.
        virtual void send(const std::vector<unsigned char> &) = 0;


        // Find out the address of the other end of this connection.
        virtual std::string getAddress() = 0;

        // Get Average Ping Time
        virtual int getPingTime() = 0;
    };
}

#endif

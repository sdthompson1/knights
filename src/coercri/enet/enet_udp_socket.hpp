/*
 * FILE:
 *   enet_udp_socket.hpp
 *
 * PURPOSE:
 *   ENet implementation of UDPSocket
 *
 * AUTHOR:
 *   Stephen Thompson
 *
 * CREATED:
 *   14-Nov-2008
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

#ifndef COERCRI_ENET_UDP_SOCKET_HPP
#define COERCRI_ENET_UDP_SOCKET_HPP

#include "../network/udp_socket.hpp"

#include "enet/enet.h"

namespace Coercri {

    class EnetUDPSocket : public UDPSocket {
    public:
        explicit EnetUDPSocket(int port, bool reuseaddr);  // set port to -1 for unbound.
        virtual ~EnetUDPSocket();

        // send (to given address/port)
        void send(const std::string &address, int port, const std::string &msg);
        void broadcast(int port, const std::string &msg);

        // receive
        // (for bound sockets -- this will listen on the port set in the ctor)
        // (for unbound sockets -- this will listen on the port assigned by the OS.)
        bool receive(std::string &address, int &port, std::string &msg);
        
    private:
        ENetSocket sock;
        int bound_port;
    };

}

#endif

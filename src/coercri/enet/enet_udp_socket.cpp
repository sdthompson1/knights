/*
 * FILE:
 *   enet_udp_socket.cpp
 *
 * AUTHOR:
 *   Stephen Thompson
 *
 * COPYRIGHT:
 *   Copyright (C) Stephen Thompson, 2008 - 2024.
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

#include "enet_udp_socket.hpp"
#include "../core/coercri_error.hpp"

namespace Coercri {

    EnetUDPSocket::EnetUDPSocket(int p, bool reuseaddr)
        : bound_port(p)
    {
        sock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);

        if (sock == ENET_SOCKET_NULL) throw CoercriError("enet_socket_create failed");

        enet_socket_set_option(sock, ENET_SOCKOPT_REUSEADDR, reuseaddr ? 1 : 0);
        enet_socket_set_option(sock, ENET_SOCKOPT_NONBLOCK, 1);
        enet_socket_set_option(sock, ENET_SOCKOPT_BROADCAST, 1);

        if (bound_port != -1) {
            ENetAddress addr;
            addr.host = ENET_HOST_ANY;
            addr.port = bound_port;
            const int bind_result = enet_socket_bind(sock, &addr);
            if (bind_result != 0) throw CoercriError("enet_socket_bind failed");
        }
    }

    EnetUDPSocket::~EnetUDPSocket()
    {
        enet_socket_destroy(sock);
    }

    bool EnetUDPSocket::receive(std::string &address, int &port, std::string &msg)
    {
        ENetAddress addr;

        char mybuf[2048];
        ENetBuffer buffer;
        buffer.data = mybuf;
        buffer.dataLength = sizeof(mybuf);
        
        int num_bytes = enet_socket_receive(sock, &addr, &buffer, 1);

        if (num_bytes <= 0) {
            return false;
        } else {
            msg.assign(mybuf, num_bytes);
            enet_address_get_host_ip(&addr, mybuf, sizeof(mybuf));
            mybuf[sizeof(mybuf)-1] = 0;
            address = mybuf;
            port = addr.port;
            return true;
        }
    }

    void EnetUDPSocket::send(const std::string &address, int port, const std::string &msg)
    {
        ENetAddress addr;
        enet_address_set_host(&addr, address.c_str());
        addr.port = port;

        ENetBuffer buffer;
        buffer.data = const_cast<char*>(msg.data());
        buffer.dataLength = msg.length();

        const int sent_length = enet_socket_send(sock, &addr, &buffer, 1);
        if (sent_length < 0) throw CoercriError("enet_socket_send failed");
    }
    
    void EnetUDPSocket::broadcast(int port, const std::string &msg)
    {
        ENetAddress addr;
        addr.host = ENET_HOST_BROADCAST;
        addr.port = port;

        ENetBuffer buffer;
        buffer.data = const_cast<char*>(msg.data());
        buffer.dataLength = msg.length();
        
        const int sent_length = enet_socket_send(sock, &addr, &buffer, 1);
        if (sent_length < 0) throw CoercriError("enet_socket_send failed");
    }    
}

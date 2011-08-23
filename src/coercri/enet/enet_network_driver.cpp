/*
 * FILE:
 *   enet_network_driver.cpp
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

#include "enet_network_connection.hpp"
#include "enet_network_driver.hpp"
#include "enet_udp_socket.hpp"

#include "../core/coercri_error.hpp"

namespace Coercri {

    namespace {
        template<class T>
        struct PtrEq {
            PtrEq(T *p_) : p(p_) { }
            bool operator()(const boost::shared_ptr<T> &q) const { return q.get() == p; }
            T* p;
        };
    }

    bool EnetNetworkDriver::is_enet_initialized = false;
    
    EnetNetworkDriver::EnetNetworkDriver(int max_inc, int max_outgoing, bool compress)
        : incoming_host(0), outgoing_host(0), server_port(0), server_enabled(false), max_incoming(max_inc),
          use_compression(compress)
    {
        if (is_enet_initialized) {
            throw CoercriError("enet initialized twice");
        }

        if (enet_initialize() != 0) {
            throw CoercriError("enet initialization failed");
        }
        
        is_enet_initialized = true;

        // create the outgoing_host. (incoming_host only created as needed.)
        outgoing_host = enet_host_create(0, max_outgoing, 0, 0);
        if (!outgoing_host) {
            throw CoercriError("EnetNetworkDriver: enet_host_create failed");
        }
        // compression is an enet 1.3 feature, we have not upgraded to enet 1.3 yet
        //if (use_compression) {
        //    enet_host_compress_with_range_coder(outgoing_host);
        //}
    }

    EnetNetworkDriver::~EnetNetworkDriver()
    {
        // Before deleting the hosts we should tell all the EnetNetworkConnections that they are about
        // to be disconnected. This will stop them trying to call enet_peer_disconnect on a peer
        // that no longer exists.
        for (EnetConnections::iterator it = connections_in.begin(); it != connections_in.end(); ++it) {
            (*it)->onDisconnect();
        }
        for (EnetConnections::iterator it = connections_out.begin(); it != connections_out.end(); ++it) {
            (*it)->onDisconnect();
        }

        // Now close down ENet.
        if (outgoing_host) enet_host_destroy(outgoing_host);
        if (incoming_host) enet_host_destroy(incoming_host);
        enet_deinitialize();
        is_enet_initialized = false;
    }

    boost::shared_ptr<NetworkConnection> EnetNetworkDriver::openConnection(const std::string &hostname, int port_number)
    {
        boost::shared_ptr<EnetNetworkConnection> new_conn(new EnetNetworkConnection(outgoing_host, hostname, port_number));
        connections_out.push_back(new_conn);
        return new_conn;
    }

    void EnetNetworkDriver::setServerPort(int port)
    {
        if (server_enabled) throw CoercriError("Cannot set server port while server is enabled");

        server_port = port;
    }

    void EnetNetworkDriver::createIncomingHostIfNeeded()
    {
        if (!incoming_host && server_enabled) {
            // Create the incoming host
            ENetAddress address;
            address.host = ENET_HOST_ANY;
            address.port = server_port;
            incoming_host = enet_host_create(&address, max_incoming, 0, 0);
            if (!incoming_host) {
                throw CoercriError("EnetNetworkDriver: enet_host_create failed");
            }
            // compression is an enet 1.3 feature, we have not upgraded to 1.3 yet
            //if (use_compression) {
            //    enet_host_compress_with_range_coder(incoming_host);
            //}
        }
    }

    void EnetNetworkDriver::destroyIncomingHost()
    {
        if (incoming_host) {
            enet_host_destroy(incoming_host);
            incoming_host = 0;
        }
    }
    
    void EnetNetworkDriver::enableServer(bool enabled)
    {
        server_enabled = enabled;
        
        createIncomingHostIfNeeded();
        
        if (!server_enabled && incoming_host) {
            if (!connections_in.empty()) {
                // There are existing connections -- close them.
                // We don't destroy the host immediately in this case; instead we wait for
                // ENet to clean things up, and then destroy the host in serviceHost().
                for (EnetConnections::iterator it = connections_in.begin(); it != connections_in.end(); ++it) {
                    (*it)->close();
                }
            } else {
                // No connections to close. We can just destroy the host right away.
                destroyIncomingHost();
            }
        }
    }
    
    EnetNetworkDriver::Connections EnetNetworkDriver::pollIncomingConnections()
    {
        Connections result;
        std::swap(new_connections_in, result);
        return result;
    }

    bool EnetNetworkDriver::serviceHost(ENetHost *host)
    {
        if (!host) return false;

        ENetEvent event;
        int result = enet_host_service(host, &event, 0);
        
        if (result < 0) {
            throw CoercriError("EnetNetworkDriver: enet_host_service failed");
        } else if (result == 0) {
            return false;
        } else {
            // got an event
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                if (event.peer->data) {
                    // One of my outgoing connections has been acknowledged by the remote server.
                    EnetNetworkConnection *conn = static_cast<EnetNetworkConnection*>(event.peer->data);
                    conn->onReceiveAcknowledgment();
                } else {
                    // This is a new incoming connection.
                    if (server_enabled) {
                        boost::shared_ptr<EnetNetworkConnection> new_conn(new EnetNetworkConnection(event.peer));
                        connections_in.push_back(new_conn);
                        new_connections_in.push_back(new_conn);
                    } else {
                        // refuse the connection
                        enet_peer_disconnect(event.peer, 0);
                    }
                }
                break;
                
            case ENET_EVENT_TYPE_DISCONNECT:
                // We've been disconnected. Note that we get this event for both timeouts and explicit disconnects.
                {
                    EnetNetworkConnection *conn = static_cast<EnetNetworkConnection*>(event.peer->data);
                    conn->onDisconnect();  // Goes into disconnected state, and breaks link to ENetPeer object.
                    
                    // Remove the connection from my vectors
                    connections_out.erase(std::remove_if(connections_out.begin(), connections_out.end(),
                                                         PtrEq<EnetNetworkConnection>(conn)),
                                          connections_out.end());
                    connections_in.erase(std::remove_if(connections_in.begin(), connections_in.end(),
                                                        PtrEq<EnetNetworkConnection>(conn)),
                                         connections_in.end());
                    new_connections_in.erase(std::remove_if(new_connections_in.begin(), new_connections_in.end(),
                                                            PtrEq<NetworkConnection>(conn)),
                                             new_connections_in.end());

                    // If server is disabled and there are no incoming connections left, then we may
                    // destroy the server host
                    if (connections_in.empty() && !server_enabled) {
                        destroyIncomingHost();
                    }
                }
                break;
                
            case ENET_EVENT_TYPE_RECEIVE:
                // A packet has been received.
                {
                    EnetNetworkConnection* from_conn = static_cast<EnetNetworkConnection*>(event.peer->data);
                    from_conn->onReceivePacket(event.packet);
                }
                break;
            }
            return true;
        }
    }

    bool EnetNetworkDriver::doEvents()
    {
        bool did_something = false;

        if (!connections_out.empty()) {
            did_something = serviceHost(outgoing_host) || did_something;
        }
        
        did_something = serviceHost(incoming_host) || did_something;
        return did_something;
    }

    bool EnetNetworkDriver::outstandingConnections()
    {
        return !connections_in.empty() || !connections_out.empty() || !new_connections_in.empty();
    }
    
    boost::shared_ptr<UDPSocket> EnetNetworkDriver::createUDPSocket(int port, bool reuseaddr)
    {
        return boost::shared_ptr<UDPSocket>(new EnetUDPSocket(port, reuseaddr));
    }

    std::string EnetNetworkDriver::resolveAddress(const std::string &ip_address)
    {
        ENetAddress address;
        enet_address_set_host(&address, ip_address.c_str());

        char name[256];
        enet_address_get_host(&address, name, sizeof(name));
        return name;
    }
}

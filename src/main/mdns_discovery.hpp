/*
 * mdns_discovery.hpp
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

#ifndef MDNS_DISCOVERY_HPP
#define MDNS_DISCOVERY_HPP

#include <string>
#include <vector>
#include <cstdint>

// Server-side: advertises a Knights game via mDNS
class MdnsAdvertiser {
public:
    MdnsAdvertiser(const std::string &host_username, int game_port);
    ~MdnsAdvertiser();

    MdnsAdvertiser(const MdnsAdvertiser &) = delete;
    MdnsAdvertiser &operator=(const MdnsAdvertiser &) = delete;

    void poll();
    void setQuestKey(const std::string &quest_key);
    void setNumPlayers(int n);

    // Called by the mDNS callback when a query is received
    void respondToQuery(int sock, const struct sockaddr* from, size_t addrlen,
                        uint16_t query_id, uint16_t rtype, uint16_t rclass,
                        const void* data, size_t size, size_t name_offset);

private:
    void announce();
    void goodbye();
    void handleQuery();

    int mdns_sock;
    std::string instance_name;   // e.g. "Player's Game on hostname._knights._udp.local."
    std::string hostname;        // e.g. "myhostname.local."
    int game_port;
    std::string host_username;
    std::string quest_key;
    int num_players;
    std::string players_str;  // std::to_string(num_players), kept in sync
    bool needs_reannounce;

    std::vector<uint8_t> buffer;
    std::vector<uint8_t> response_buffer;
    std::vector<uint32_t> local_interfaces;  // s_addr values for each NIC
};

// Client-side: discovers Knights games via mDNS
class MdnsDiscoverer {
public:
    MdnsDiscoverer();
    ~MdnsDiscoverer();

    MdnsDiscoverer(const MdnsDiscoverer &) = delete;
    MdnsDiscoverer &operator=(const MdnsDiscoverer &) = delete;

    struct ServiceInfo {
        std::string instance_name;
        std::string ip_address;
        std::string hostname;
        std::string host_username;
        std::string quest_key;
        std::string version;
        int game_port;
        int num_players;
        unsigned int last_seen_ms;
    };

    // Process mDNS responses. Returns true if the service list changed.
    bool poll(unsigned int time_now_ms);

    // Force an immediate re-query
    void forceQuery();

    const std::vector<ServiceInfo> & getServices() const;

    bool isValid() const { return mdns_sock >= 0; }

private:
    int mdns_sock;
    unsigned int last_query_time;
    unsigned int query_interval;
    bool first_query_sent;
    std::vector<ServiceInfo> services;
    std::vector<uint8_t> buffer;
    std::vector<uint32_t> local_interfaces;  // s_addr values for each NIC
};

#endif

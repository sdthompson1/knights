/*
 * mdns_discovery.cpp
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

#include "mdns_discovery.hpp"
#include "version.hpp"

#ifdef _WIN32
#include <Winsock2.h>
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#endif

#include "mdns.h"

#include <algorithm>
#include <cstring>
#include <random>

namespace {

    const char SERVICE_TYPE[] = "_knights._udp.local.";
    const size_t BUFFER_SIZE = 2048;
    const unsigned int QUERY_INTERVAL_MIN_MS = 3000;
    const unsigned int QUERY_INTERVAL_MAX_MS = 5000;
    const unsigned int SERVICE_TIMEOUT_MS = 10000;
    const int NUM_ADDITIONAL_RECORDS = 5;  // 1 SRV + 4 TXT

    unsigned int randomQueryInterval()
    {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<unsigned int> dist(QUERY_INTERVAL_MIN_MS,
                                                         QUERY_INTERVAL_MAX_MS);
        return dist(rng);
    }

    std::string getLocalHostname()
    {
        char buf[256] = {};
        if (gethostname(buf, sizeof(buf)) == 0) {
            // Ensure null termination (just in case)
            buf[sizeof(buf) - 1] = 0;
            // Strip any domain part
            char *dot = strchr(buf, '.');
            if (dot) *dot = 0;
            return std::string(buf);
        }
        return "knights-host";
    }

    std::vector<struct in_addr> getLocalIPv4Addresses()
    {
        std::vector<struct in_addr> result;

#ifdef _WIN32
        ULONG bufSize = 15000;
        std::vector<uint8_t> buf(bufSize);
        PIP_ADAPTER_ADDRESSES addrs = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.data());
        ULONG ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                                         nullptr, addrs, &bufSize);
        if (ret == ERROR_BUFFER_OVERFLOW) {
            buf.resize(bufSize);
            addrs = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buf.data());
            ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST,
                                       nullptr, addrs, &bufSize);
        }
        if (ret == NO_ERROR) {
            for (auto *a = addrs; a; a = a->Next) {
                if (a->OperStatus != IfOperStatusUp) continue;
                if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
                if (a->IfType == IF_TYPE_TUNNEL) continue;
                for (auto *ua = a->FirstUnicastAddress; ua; ua = ua->Next) {
                    if (ua->Address.lpSockaddr->sa_family == AF_INET) {
                        auto *sa = reinterpret_cast<struct sockaddr_in*>(ua->Address.lpSockaddr);
                        result.push_back(sa->sin_addr);
                    }
                }
            }
        }
#else
        struct ifaddrs *ifap = nullptr;
        if (getifaddrs(&ifap) == 0) {
            for (auto *ifa = ifap; ifa; ifa = ifa->ifa_next) {
                // The interface must be IPv4 with a valid address
                if (!ifa->ifa_addr) continue;
                if (ifa->ifa_addr->sa_family != AF_INET) continue;
                // The interface must be up and running and support multicast
                if (!(ifa->ifa_flags & IFF_UP)) continue;
                if (!(ifa->ifa_flags & IFF_MULTICAST)) continue;
                // No point querying or advertising on loopback
                if (ifa->ifa_flags & IFF_LOOPBACK) continue;
                // Skip point-to-point interfaces as these are unlikely to support mDNS
                if (ifa->ifa_flags & IFF_POINTOPOINT) continue;
                // Skip well-known isolated virtual bridges (Docker, libvirt)
                if (ifa->ifa_name) {
                    std::string name(ifa->ifa_name);
                    if (name.compare(0, 6, "docker") == 0) continue;
                    if (name.compare(0, 5, "virbr") == 0) continue;
                }
                auto *sa = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
                result.push_back(sa->sin_addr);
            }
            freeifaddrs(ifap);
        }
#endif
        return result;
    }

    void joinMulticastOnAllInterfaces(int sock, const std::vector<struct in_addr> &addrs)
    {
        struct ip_mreq req;
        memset(&req, 0, sizeof(req));
        req.imr_multiaddr.s_addr = htonl((((uint32_t)224U) << 24U) | ((uint32_t)251U));
        for (const auto &addr : addrs) {
            req.imr_interface = addr;
            // Ignore errors — duplicates from the initial INADDR_ANY join are harmless
            setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&req, sizeof(req));
        }
    }

    std::vector<uint32_t> getInterfaceAddrsAsU32(const std::vector<struct in_addr> &addrs)
    {
        std::vector<uint32_t> result;
        result.reserve(addrs.size());
        for (const auto &a : addrs) {
            result.push_back(a.s_addr);
        }
        return result;
    }

    template<typename Func>
    void sendOnAllInterfaces(int sock, const std::vector<uint32_t> &interfaces, Func fn)
    {
        for (auto iface : interfaces) {
            struct in_addr addr;
            addr.s_addr = iface;
            setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (const char*)&addr, sizeof(addr));
            fn();
        }
        if (interfaces.empty()) {
            fn();
        }
    }

    std::string mdnsStringToStd(mdns_string_t s)
    {
        if (s.str && s.length > 0)
            return std::string(s.str, s.length);
        return std::string();
    }

    mdns_record_t makeRecordPTR(const std::string &name, const std::string &instance)
    {
        mdns_record_t rec = {};
        rec.name.str = name.c_str();
        rec.name.length = name.size();
        rec.type = MDNS_RECORDTYPE_PTR;
        rec.data.ptr.name.str = instance.c_str();
        rec.data.ptr.name.length = instance.size();
        return rec;
    }

    mdns_record_t makeRecordSRV(const std::string &instance, const std::string &hostname, uint16_t port)
    {
        mdns_record_t rec = {};
        rec.name.str = instance.c_str();
        rec.name.length = instance.size();
        rec.type = MDNS_RECORDTYPE_SRV;
        rec.data.srv.priority = 0;
        rec.data.srv.weight = 0;
        rec.data.srv.port = port;
        rec.data.srv.name.str = hostname.c_str();
        rec.data.srv.name.length = hostname.size();
        return rec;
    }

    // Build a TXT record from separate key and value strings.
    // Both strings must outlive the returned record (pointers into them).
    mdns_record_t makeRecordTXT(const std::string &instance,
                                const std::string &key, const std::string &value)
    {
        mdns_record_t rec = {};
        rec.name.str = instance.c_str();
        rec.name.length = instance.size();
        rec.type = MDNS_RECORDTYPE_TXT;
        rec.data.txt.key = { key.c_str(), key.size() };
        rec.data.txt.value = { value.c_str(), value.size() };
        return rec;
    }

    // TXT record key names (file-scope so they outlive any records that point into them)
    const std::string KEY_VERSION("version");
    const std::string KEY_PLAYERS("players");
    const std::string KEY_QUEST("quest");
    const std::string KEY_HOST("host");

    // Build the SRV + TXT additional records common to announce, goodbye, and query responses.
    // The value strings (version, players, quest, host) must outlive the returned records.
    // Returns the number of records written (always 5).
    int buildAdditionalRecords(mdns_record_t *out,
                               const std::string &instance_name,
                               const std::string &hostname, uint16_t port,
                               const std::string &version,
                               const std::string &players,
                               const std::string &quest,
                               const std::string &host)
    {
        int n = 0;
        out[n++] = makeRecordSRV(instance_name, hostname, port);
        out[n++] = makeRecordTXT(instance_name, KEY_VERSION, version);
        out[n++] = makeRecordTXT(instance_name, KEY_PLAYERS, players);
        out[n++] = makeRecordTXT(instance_name, KEY_QUEST, quest);
        out[n++] = makeRecordTXT(instance_name, KEY_HOST, host);
        return n;
    }

    // ============================================================
    // Discoverer callback: handles mDNS query responses
    // ============================================================
    struct DiscovererData {
        std::vector<MdnsDiscoverer::ServiceInfo> *services;
        unsigned int time_now_ms;
        bool changed;
        bool query_seen;  // true if a matching QM query was overheard

        // Temp storage for parsing multi-record responses
        char namebuf[256];
        char strbuf[256];
    };

    MdnsDiscoverer::ServiceInfo* findOrCreateService(std::vector<MdnsDiscoverer::ServiceInfo> &services,
                                                     const std::string &instance_name,
                                                     unsigned int time_now_ms,
                                                     bool &changed)
    {
        for (auto &s : services) {
            if (s.instance_name == instance_name) {
                s.last_seen_ms = time_now_ms;
                return &s;
            }
        }
        // Create new entry
        MdnsDiscoverer::ServiceInfo si;
        si.instance_name = instance_name;
        si.game_port = 0;
        si.num_players = 0;
        si.last_seen_ms = time_now_ms;
        services.push_back(si);
        changed = true;
        return &services.back();
    }

    // Extract the source IP address from the 'from' sockaddr
    std::string extractSourceIP(const struct sockaddr* from)
    {
        if (from && from->sa_family == AF_INET) {
            char ip_str[INET_ADDRSTRLEN] = {};
            const struct sockaddr_in *addr4 = reinterpret_cast<const struct sockaddr_in*>(from);
            inet_ntop(AF_INET, &addr4->sin_addr, ip_str, sizeof(ip_str));
            return std::string(ip_str);
        }
        return std::string();
    }

    int discovererCallback(int sock, const struct sockaddr* from, size_t addrlen,
                           mdns_entry_type_t entry, uint16_t query_id, uint16_t rtype,
                           uint16_t rclass, uint32_t ttl, const void* data, size_t size,
                           size_t name_offset, size_t name_length, size_t record_offset,
                           size_t record_length, void* user_data)
    {
        (void)sock; (void)addrlen; (void)query_id;

        DiscovererData *dd = static_cast<DiscovererData*>(user_data);

        if (entry == MDNS_ENTRYTYPE_QUESTION) {
            // Duplicate Question Suppression (RFC 6762 Section 7.3):
            // If we overhear a QM query matching ours, we can reset our timer.
            if (rtype == MDNS_RECORDTYPE_PTR && !(rclass & MDNS_UNICAST_RESPONSE)) {
                size_t off = name_offset;
                mdns_string_t qname = mdns_string_extract(data, size, &off,
                                                          dd->namebuf, sizeof(dd->namebuf));
                std::string query_name = mdnsStringToStd(qname);
                if (query_name == SERVICE_TYPE) {
                    dd->query_seen = true;
                }
            }
            return 0;
        }
        if (ttl == 0 && rtype != MDNS_RECORDTYPE_PTR) return 0;

        // Extract the record name
        size_t name_off = name_offset;
        mdns_string_t name = mdns_string_extract(data, size, &name_off, dd->namebuf, sizeof(dd->namebuf));
        std::string record_name = mdnsStringToStd(name);

        // The source IP of this response packet is the server's actual reachable address
        std::string source_ip = extractSourceIP(from);

        if (rtype == MDNS_RECORDTYPE_PTR) {
            // Only process PTR records for our service type
            if (record_name != SERVICE_TYPE) return 0;

            mdns_string_t ptr_name = mdns_record_parse_ptr(data, size, record_offset, record_length,
                                                           dd->strbuf, sizeof(dd->strbuf));
            std::string instance = mdnsStringToStd(ptr_name);
            if (!instance.empty()) {
                if (ttl == 0) {
                    // Goodbye packet — remove the service immediately
                    auto &svcs = *dd->services;
                    auto it = std::find_if(svcs.begin(), svcs.end(),
                        [&](const MdnsDiscoverer::ServiceInfo &s) {
                            return s.instance_name == instance;
                        });
                    if (it != svcs.end()) {
                        svcs.erase(it);
                        dd->changed = true;
                    }
                } else {
                    auto *si = findOrCreateService(*dd->services, instance, dd->time_now_ms, dd->changed);
                    if (!source_ip.empty() && si->ip_address != source_ip) {
                        si->ip_address = source_ip;
                        dd->changed = true;
                    }
                }
            }
        } else if (rtype == MDNS_RECORDTYPE_SRV) {
            // Only process SRV records for instances of our service type
            const std::string suffix("._knights._udp.local.");
            if (record_name.size() <= suffix.size() ||
                record_name.compare(record_name.size() - suffix.size(), suffix.size(), suffix) != 0)
                return 0;

            mdns_record_srv_t srv = mdns_record_parse_srv(data, size, record_offset, record_length,
                                                          dd->strbuf, sizeof(dd->strbuf));
            // record_name is the instance name for SRV records
            if (!record_name.empty()) {
                auto *si = findOrCreateService(*dd->services, record_name, dd->time_now_ms, dd->changed);
                if (si->game_port != srv.port) {
                    si->game_port = srv.port;
                    dd->changed = true;
                }
                std::string srv_hostname = mdnsStringToStd(srv.name);
                if (si->hostname != srv_hostname) {
                    si->hostname = srv_hostname;
                    dd->changed = true;
                }
                if (!source_ip.empty() && si->ip_address != source_ip) {
                    si->ip_address = source_ip;
                    dd->changed = true;
                }
            }
        } else if (rtype == MDNS_RECORDTYPE_TXT) {
            // Only process TXT records for instances of our service type
            const std::string suffix("._knights._udp.local.");
            if (record_name.size() <= suffix.size() ||
                record_name.compare(record_name.size() - suffix.size(), suffix.size(), suffix) != 0)
                return 0;

            mdns_record_txt_t txt_records[8];
            size_t txt_count = mdns_record_parse_txt(data, size, record_offset, record_length,
                                                     txt_records, 8);

            if (!record_name.empty() && txt_count > 0) {
                auto *si = findOrCreateService(*dd->services, record_name, dd->time_now_ms, dd->changed);
                for (size_t i = 0; i < txt_count; ++i) {
                    std::string key = mdnsStringToStd(txt_records[i].key);
                    std::string value = mdnsStringToStd(txt_records[i].value);

                    if (key == KEY_VERSION) {
                        if (si->version != value) { si->version = value; dd->changed = true; }
                    } else if (key == KEY_PLAYERS) {
                        int n = 0;
                        try { n = std::stoi(value); } catch (...) {}
                        if (si->num_players != n) { si->num_players = n; dd->changed = true; }
                    } else if (key == KEY_QUEST) {
                        if (si->quest_key != value) { si->quest_key = value; dd->changed = true; }
                    } else if (key == KEY_HOST) {
                        if (si->host_username != value) { si->host_username = value; dd->changed = true; }
                    }
                }
            }
        }

        return 0;
    }

} // end anonymous namespace


// ============================================================
// MdnsAdvertiser implementation
// ============================================================

MdnsAdvertiser::MdnsAdvertiser(const std::string &host_user, int port)
    : mdns_sock(-1),
      game_port(port),
      host_username(host_user),
      num_players(0),
      players_str("0"),
      needs_reannounce(false),
      buffer(BUFFER_SIZE, 0),
      response_buffer(BUFFER_SIZE, 0)
{
    // Build instance name: "Username's Game on hostname._knights._udp.local."
    // Include hostname to avoid conflicts when two players have the same name.
    // Strip dots from the username since dots are DNS label separators and mdns.h
    // doesn't support dot escaping (note hostname doesn't contain dots by construction).
    std::string safe_name = host_username;
    safe_name.erase(std::remove(safe_name.begin(), safe_name.end(), '.'), safe_name.end());
    std::string local_hostname = getLocalHostname();
    instance_name = safe_name + "'s Game on " + local_hostname + "._knights._udp.local.";

    // Get local hostname
    hostname = local_hostname + ".local.";

    // Open mDNS socket on port 5353 for listening to queries
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(MDNS_PORT);
    saddr.sin_addr.s_addr = INADDR_ANY;

    try {
        mdns_sock = mdns_socket_open_ipv4(&saddr);
    } catch (...) {
        mdns_sock = -1;
    }

    if (mdns_sock < 0) {
        return;
    }

    auto addrs = getLocalIPv4Addresses();
    joinMulticastOnAllInterfaces(mdns_sock, addrs);
    local_interfaces = getInterfaceAddrsAsU32(addrs);

    announce();
}

MdnsAdvertiser::~MdnsAdvertiser()
{
    if (mdns_sock >= 0) {
        try {
            goodbye();
        } catch (...) {
            // Ignore errors during cleanup
        }
        mdns_socket_close(mdns_sock);
    }
}

void MdnsAdvertiser::poll()
{
    if (mdns_sock < 0) return;

    if (needs_reannounce) {
        announce();
        needs_reannounce = false;
    }

    // Handle incoming queries
    handleQuery();
}

void MdnsAdvertiser::setQuestKey(const std::string &qk)
{
    if (quest_key != qk) {
        quest_key = qk;
        needs_reannounce = true;
    }
}

void MdnsAdvertiser::setNumPlayers(int n)
{
    if (num_players != n) {
        num_players = n;
        players_str = std::to_string(n);
        needs_reannounce = true;
    }
}

void MdnsAdvertiser::announce()
{
    // Send an mDNS announcement (multicast). Discoverers on port 5353 will
    // receive these as well as query responses.

    if (mdns_sock < 0) return;

    mdns_record_t answer = makeRecordPTR(SERVICE_TYPE, instance_name);

    std::string v_version(KNIGHTS_VERSION);
    mdns_record_t additional[NUM_ADDITIONAL_RECORDS];
    int add_count = buildAdditionalRecords(
        additional, instance_name, hostname, (uint16_t)game_port,
        v_version, players_str, quest_key, host_username);

    sendOnAllInterfaces(mdns_sock, local_interfaces, [&]() {
        mdns_announce_multicast(mdns_sock, buffer.data(), buffer.size(),
                                answer, nullptr, 0, additional, add_count);
    });
}

void MdnsAdvertiser::goodbye()
{
    if (mdns_sock < 0) return;

    mdns_record_t answer = makeRecordPTR(SERVICE_TYPE, instance_name);

    std::string v_version(KNIGHTS_VERSION);
    mdns_record_t additional[NUM_ADDITIONAL_RECORDS];
    int add_count = buildAdditionalRecords(
        additional, instance_name, hostname, (uint16_t)game_port,
        v_version, players_str, quest_key, host_username);

    sendOnAllInterfaces(mdns_sock, local_interfaces, [&]() {
        mdns_goodbye_multicast(mdns_sock, buffer.data(), buffer.size(),
                               answer, nullptr, 0, additional, add_count);
    });
}

void MdnsAdvertiser::respondToQuery(int sock, const struct sockaddr* from, size_t addrlen,
                                    uint16_t query_id, uint16_t rtype, uint16_t rclass,
                                    const void* data, size_t size, size_t name_offset)
{
    char namebuf[256];
    mdns_string_t name = mdns_string_extract(data, size, &name_offset, namebuf, sizeof(namebuf));
    std::string query_name = mdnsStringToStd(name);

    std::string service_type(SERVICE_TYPE);
    bool match_service = (query_name == service_type);
    bool match_instance = (query_name == instance_name);
    bool match_any = (rtype == MDNS_RECORDTYPE_ANY);

    if (!match_service && !match_instance && !match_any) return;

    if (rtype == MDNS_RECORDTYPE_PTR || rtype == MDNS_RECORDTYPE_ANY || match_service) {
        mdns_record_t answer = makeRecordPTR(service_type, instance_name);

        std::string v_version(KNIGHTS_VERSION);
        mdns_record_t additional[NUM_ADDITIONAL_RECORDS];
        int add_count = buildAdditionalRecords(
            additional, instance_name, hostname, (uint16_t)game_port,
            v_version, players_str, quest_key, host_username);

        if (rclass & MDNS_UNICAST_RESPONSE) {
            mdns_query_answer_unicast(sock, from, addrlen,
                                      response_buffer.data(), response_buffer.size(),
                                      query_id, MDNS_RECORDTYPE_PTR,
                                      service_type.c_str(), service_type.size(),
                                      answer, nullptr, 0,
                                      additional, add_count);
        } else {
            sendOnAllInterfaces(sock, local_interfaces, [&]() {
                mdns_query_answer_multicast(sock,
                                            response_buffer.data(), response_buffer.size(),
                                            answer, nullptr, 0,
                                            additional, add_count);
            });
        }
    }
}

namespace {
    int advertiserCallback(int sock, const struct sockaddr* from, size_t addrlen,
                           mdns_entry_type_t entry, uint16_t query_id, uint16_t rtype,
                           uint16_t rclass, uint32_t ttl, const void* data, size_t size,
                           size_t name_offset, size_t name_length, size_t record_offset,
                           size_t record_length, void* user_data)
    {
        (void)ttl; (void)name_length; (void)record_offset; (void)record_length;
        if (entry != MDNS_ENTRYTYPE_QUESTION) return 0;
        static_cast<MdnsAdvertiser*>(user_data)->respondToQuery(
            sock, from, addrlen, query_id, rtype, rclass, data, size, name_offset);
        return 0;
    }
}

void MdnsAdvertiser::handleQuery()
{
    if (mdns_sock < 0) return;
    mdns_socket_listen(mdns_sock, buffer.data(), buffer.size(), advertiserCallback, this);
}


// ============================================================
// MdnsDiscoverer implementation
// ============================================================

MdnsDiscoverer::MdnsDiscoverer()
    : mdns_sock(-1),
      last_query_time(0),
      query_interval(randomQueryInterval()),
      first_query_sent(false),
      buffer(BUFFER_SIZE, 0)
{
    // Open socket on port 5353 so that mdns.h clears MDNS_UNICAST_RESPONSE,
    // causing the advertiser to reply via multicast rather than unicast.
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(MDNS_PORT);
    saddr.sin_addr.s_addr = INADDR_ANY;

    try {
        mdns_sock = mdns_socket_open_ipv4(&saddr);
    } catch (...) {
        mdns_sock = -1;
    }

    if (mdns_sock >= 0) {
        auto addrs = getLocalIPv4Addresses();
        joinMulticastOnAllInterfaces(mdns_sock, addrs);
        local_interfaces = getInterfaceAddrsAsU32(addrs);
    }
}

MdnsDiscoverer::~MdnsDiscoverer()
{
    if (mdns_sock >= 0) {
        mdns_socket_close(mdns_sock);
    }
}

bool MdnsDiscoverer::poll(unsigned int time_now_ms)
{
    if (mdns_sock < 0) return false;

    bool changed = !first_query_sent;

    // Send query periodically on all interfaces
    if (!first_query_sent || (time_now_ms - last_query_time) >= query_interval) {
        sendOnAllInterfaces(mdns_sock, local_interfaces, [&]() {
            mdns_query_send(mdns_sock, MDNS_RECORDTYPE_PTR,
                            SERVICE_TYPE, sizeof(SERVICE_TYPE) - 1,
                            buffer.data(), buffer.size(), 0);
        });
        last_query_time = time_now_ms;
        query_interval = randomQueryInterval();
        first_query_sent = true;
    }

    // Receive and process incoming packets (both queries and responses).
    // Using mdns_socket_listen so we can see incoming questions for
    // Duplicate Question Suppression (RFC 6762 Section 7.3).
    DiscovererData dd;
    dd.services = &services;
    dd.time_now_ms = time_now_ms;
    dd.changed = changed;
    dd.query_seen = false;
    memset(dd.namebuf, 0, sizeof(dd.namebuf));
    memset(dd.strbuf, 0, sizeof(dd.strbuf));

    // Keep receiving until no more data
    size_t records;
    do {
        records = mdns_socket_listen(mdns_sock, buffer.data(), buffer.size(),
                                     discovererCallback, &dd);
    } while (records > 0);

    // Duplicate Question Suppression: if we overheard a matching QM query,
    // reset our timer with a new random interval
    if (dd.query_seen) {
        last_query_time = time_now_ms;
        query_interval = randomQueryInterval();
    }

    changed = dd.changed;

    // Remove stale services
    auto it = std::remove_if(services.begin(), services.end(),
        [time_now_ms](const ServiceInfo &si) {
            return (time_now_ms - si.last_seen_ms) > SERVICE_TIMEOUT_MS;
        });
    if (it != services.end()) {
        services.erase(it, services.end());
        changed = true;
    }

    return changed;
}

void MdnsDiscoverer::forceQuery()
{
    services.clear();
    first_query_sent = false;
}

const std::vector<MdnsDiscoverer::ServiceInfo> & MdnsDiscoverer::getServices() const
{
    return services;
}

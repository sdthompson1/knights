/*
 * dummy_online_platform.hpp
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

#ifndef DUMMY_ONLINE_PLATFORM_HPP
#define DUMMY_ONLINE_PLATFORM_HPP

#ifdef ONLINE_PLATFORM_DUMMY

#include "online_platform.hpp"

// DummyOnlinePlatform implements the OnlinePlatform interface by
// connecting to a TCP lobby server running on the local machine. This
// is useful for testing the Knights client without needing to connect
// to an actual online platform (such as Steam).

// The server side of this is implemented in python/lobby_server.py.

class DummyOnlinePlatform : public OnlinePlatform {
public:
    DummyOnlinePlatform();
    virtual ~DummyOnlinePlatform() = default;

    // Returns a randomly generated user ID
    virtual std::string getCurrentUserId() const override;
    
    // Returns the platform_user_id as a UTF8String
    virtual Coercri::UTF8String lookupUserName(const std::string& platform_user_id) const override;

    // Returns a new DummyPlatformLobby
    virtual std::unique_ptr<PlatformLobby> createLobby(Visibility vis) override;

    // Queries the lobby server for the latest lobby list
    virtual std::vector<std::string> getLobbyList() override;

    // Get lobby info
    virtual LobbyInfo getLobbyInfo(const std::string &lobby_id) override;

    // Returns a new DummyPlatformLobby
    virtual std::unique_ptr<PlatformLobby> joinLobby(const std::string& lobby_id) override;

    // P2P connections (faked using normal Coercri NetworkConnections)
    virtual Coercri::NetworkDriver & getNetworkDriver() override;
    
    // Public methods for DummyPlatformLobby to use
    bool sendMessage(unsigned char msg_type, const std::string& payload);
    bool receiveResponse(std::string& response_data);
    
    // Protocol constants for DummyPlatformLobby to use
    static constexpr unsigned char MSG_LEAVE_LOBBY = 0x05;
    static constexpr unsigned char MSG_GET_LOBBY_INFO = 0x06;
    static constexpr unsigned char MSG_SET_LOBBY_INFO = 0x07;

private:
    std::string current_user_id;
    std::string current_lobby_id;
    std::string current_lobby_leader;
    int socket_fd;
    bool connected;

    std::unique_ptr<Coercri::NetworkDriver> network_driver;

    bool connect_to_server();

    void create_network_driver(const std::string & my_user_id);
};

#endif

#endif

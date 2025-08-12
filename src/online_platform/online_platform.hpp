/*
 * online_platform.hpp
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

#ifndef ONLINE_PLATFORM_HPP
#define ONLINE_PLATFORM_HPP

#ifdef ONLINE_PLATFORM

#include <memory>
#include <string>
#include <vector>

#include "platform_lobby.hpp"

#include "utf8string.hpp"

namespace Coercri {
    class NetworkDriver;
}

// The OnlinePlatform interface represents an online gaming platform such as Steam.

class OnlinePlatform {
public:
    virtual ~OnlinePlatform() = default;


    // User Ids:

    // Return the platform user ID of the current user (e.g. their Steam ID)
    virtual std::string getCurrentUserId() const = 0;
    
    // Convert a platform user ID to a human-readable user name
    virtual Coercri::UTF8String lookupUserName(const std::string& platform_user_id) const = 0;


    // Lobbies:

    enum class Visibility {
        PRIVATE,
        FRIENDS_ONLY,
        PUBLIC
    };

    struct LobbyInfo {
        // Current lobby leader.
        // Note: this might be slightly out of date. Use
        // PlatformLobby::getLeaderId for the latest information (but
        // you can only do that once you join the lobby).
        std::string leader_id;

        // Number of players currently in the lobby
        int num_players;

        // Current game status code
        // (Used for showing the current quest on the lobby search screen.
        // See also KnightsApp::setQuestMessageCode.)
        int status_code;
    };

    // Create a new lobby
    virtual std::unique_ptr<PlatformLobby> createLobby(Visibility vis) = 0;

    // Search for lobbies to join. Returns a vector of lobby ID strings.
    // (TODO: maybe add more sorting or filtering options.)
    virtual std::vector<std::string> getLobbyList() = 0;

    // Get lobby info (by lobby ID)
    virtual LobbyInfo getLobbyInfo(const std::string &lobby_id) = 0;

    // Join a lobby (by lobby ID)
    virtual std::unique_ptr<PlatformLobby> joinLobby(const std::string& lobby_id) = 0;


    // P2P Network Connections:

    // We assume that an online platform implements the
    // Coercri::NetworkDriver interface for peer-to-peer connections.
    // Port numbers are ignored and should be set to zero, and
    // addresses should be the platform_user_id of the player you want
    // to connect to.
    virtual Coercri::NetworkDriver & getNetworkDriver() = 0;

};

#endif  // ONLINE_PLATFORM

#endif  // ONLINE_PLATFORM_HPP

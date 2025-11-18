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

#include "localization.hpp"
#include "player_id.hpp"
#include "utf8string.hpp"

namespace Coercri {
    class NetworkDriver;
}

// The OnlinePlatform interface represents an online gaming platform such as Steam.

class OnlinePlatform {
public:
    virtual ~OnlinePlatform() = default;


    // Update:

    // This should be called every frame (or at least every 100ms or so) to
    // keep the online platform backend up to date
    virtual void update() = 0;


    // User Ids:

    // Return the platform user ID of the current user (e.g. their Steam ID)
    virtual PlayerID getCurrentUserId() = 0;
    
    // Convert a platform user ID to a human-readable user name
    virtual Coercri::UTF8String lookupUserName(const PlayerID& platform_user_id) = 0;


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
        PlayerID leader_id;

        // Number of players currently in the lobby
        int num_players;

        // Current game status (as local key + params)
        // (Used for showing the current quest on the lobby search screen.
        // See also KnightsApp::setQuestMessageCode.)
        LocalKey game_status_key;
        std::vector<LocalParam> game_status_params;

        // Game data and build checksum (uint64_t)
        uint64_t checksum;
    };

    // Create a new lobby
    virtual std::unique_ptr<PlatformLobby> createLobby(Visibility vis, uint64_t checksum) = 0;

    // Set lobby filter. If filters are enabled, getLobbyList will return results filtered
    // accordingly, but note that the list might not update immediately.
    virtual void clearLobbyFilters() = 0;
    virtual void addChecksumFilter(uint64_t checksum) = 0;

    // Search for lobbies to join. Returns a vector of lobby ID strings.
    // Note: this can be called frequently (e.g. every 200ms) as results are cached.
    // (TODO: Add a method to request a manual refresh of the list.)
    virtual std::vector<std::string> getLobbyList() = 0;

    // Force a manual refresh of the cached getLobbyList results.
    // (Note: this can be called as often as desired - the underlying implementation
    // will rate-limit if necessary.)
    virtual void refreshLobbyList() = 0;

    // Get lobby info (by lobby ID)
    virtual LobbyInfo getLobbyInfo(const std::string &lobby_id) = 0;

    // Join a lobby (by lobby ID)
    virtual std::unique_ptr<PlatformLobby> joinLobby(const std::string& lobby_id) = 0;

    // This returns a non-empty string (lobby id) if the online platform is requesting that
    // the game should join a particular lobby (because of accepting an invite or similar).
    // The non-empty string will only be returned once - subsequent calls will return an
    // empty string again (until the next invite is accepted, that is).
    virtual std::string getRequestedLobbyToJoin() { return ""; }


    // P2P Network Connections:

    // We assume that an online platform implements the
    // Coercri::NetworkDriver interface for peer-to-peer connections.
    // Port numbers are ignored and should be set to zero, and
    // addresses should be the platform_user_id of the player you want
    // to connect to.
    virtual Coercri::NetworkDriver & getNetworkDriver() = 0;


    // Overlay:

    // For platforms that have an overlay (i.e. Steam) this function
    // will return true if the overlay needs to be repainted right
    // now. For other platforms it always returns false.
    virtual bool needsRepaint() { return false; }


    // Build ID:

    // Get a build ID string for the current build, from the online platform.
    // If this is not available, an empty string will be returned.
    virtual std::string getBuildId() { return ""; }
};

#endif  // ONLINE_PLATFORM

#endif  // ONLINE_PLATFORM_HPP

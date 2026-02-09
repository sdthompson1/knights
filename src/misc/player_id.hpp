/*
 * player_id.hpp
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

#ifndef PLAYER_ID_HPP
#define PLAYER_ID_HPP

#include "core/utf8string.hpp"

#include <functional>
#include <stdexcept>
#include <string>

// Class representing a user/player ID.

// This contains a platform (such as "steam"), a platform user ID
// (such as a Steam ID), and a user name.

// If the platform user ID is available then we generally use that in
// favour of the raw user name, but the user name is available as a
// backup in case no online platform is available.

class PlayerID {
public:
    // Construct an empty PlayerID
    PlayerID() { }

    // Construct from platform user ID and name
    // (By convention, platform and platform_user_id should be non-empty
    // in this case, but user_name might or might not be empty)
    PlayerID(std::string platform,
             std::string platform_user_id,
             Coercri::UTF8String user_name)
        : platform(platform),
          platform_user_id(platform_user_id),
          user_name(user_name)
    {
        if (platform.empty() || platform_user_id.empty()) {
            throw std::runtime_error("invalid player id");
        }
    }

    // Construct from name only (should be non-empty)
    PlayerID(Coercri::UTF8String user_name)
        : user_name(user_name)
    {
        if (user_name.empty()) {
            throw std::runtime_error("invalid player id");
        }
    }

    // Check if empty
    bool empty() const { return platform.empty() && user_name.empty(); }

    // Get the ID and name strings
    const std::string & getPlatform() const { return platform; }
    const std::string & getPlatformUserId() const { return platform_user_id; }
    const Coercri::UTF8String & getUserName() const { return user_name; }

    // Format a player ID for logging etc.
    std::string getDebugString() const { return platform + ":" + platform_user_id + ":" + user_name.asUTF8(); }

    // Equality: Compare by platform user ID if available, otherwise user name
    bool operator==(const PlayerID &other) const {
        if (!platform.empty() && !other.platform.empty()) {
            return platform == other.platform
                && platform_user_id == other.platform_user_id;
        } else if (platform.empty() && other.platform.empty()) {
            return user_name == other.user_name;
        } else {
            return false;
        }
    }
    bool operator!=(const PlayerID &other) const {
        return !(*this == other);
    }

    // Less-than: for use in ordered sets and the like
    bool operator<(const PlayerID &other) const {
        if (!platform.empty() && !other.platform.empty()) {
            // Sort by platform first, then platform user id
            return platform < other.platform
                || (platform == other.platform && platform_user_id < other.platform_user_id);
        } else if (platform.empty() && other.platform.empty()) {
            // Sort by user name
            return user_name < other.user_name;
        } else {
            // Put platform users before non-platform users
            return !platform.empty();
        }
    }

private:
    std::string platform;
    std::string platform_user_id;
    Coercri::UTF8String user_name;
};

// Hash function specialization to allow PlayerID to be used as a key in unordered_map
namespace std {
    template<>
    struct hash<PlayerID> {
        std::size_t operator()(const PlayerID &p) const {
            if (!p.getPlatform().empty()) {
                return std::hash<std::string>()(p.getPlatformUserId());
            } else {
                return std::hash<std::string>()(p.getUserName().asUTF8());
            }
        }
    };
}

#endif      // PLAYER_ID_HPP

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

#include <string>

// Class representing a user/player ID.

// If we are using an online platform such as Steam, then the player
// ID is the online platform user ID (e.g. Steam ID), formatted as
// text.

// If we are not using an online platform (e.g. Host/Join LAN games)
// then the player ID is just the player's name, encoded as a UTF-8
// string.

class PlayerID {
public:
    PlayerID() { }
    explicit PlayerID(const std::string &id) : id(id) { }

    const std::string & asString() const { return id; }

    bool empty() const { return id.empty(); }

    bool operator==(const PlayerID &other) const { return id == other.id; }
    bool operator!=(const PlayerID &other) const { return id != other.id; }

    // < operator for use in ordered sets and the like
    bool operator<(const PlayerID &other) const { return id < other.id; }

private:
    std::string id;
};

#endif      // PLAYER_ID_HPP

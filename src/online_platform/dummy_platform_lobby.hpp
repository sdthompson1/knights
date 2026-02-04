/*
 * dummy_platform_lobby.hpp
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

#ifndef DUMMY_PLATFORM_LOBBY_HPP
#define DUMMY_PLATFORM_LOBBY_HPP

#ifdef ONLINE_PLATFORM_DUMMY

#include "platform_lobby.hpp"

#include <chrono>

class DummyOnlinePlatform; // Forward declaration

class DummyPlatformLobby : public PlatformLobby {
public:
    DummyPlatformLobby(DummyOnlinePlatform* platform, const std::string& lobby_id);
    virtual ~DummyPlatformLobby();

    // Returns the current lobby state
    virtual State getState() override;
    
    // Returns the lobby leader's platform user ID
    virtual PlayerID getLeaderId() override;

    // Set game status for the lobby - only the leader can do this
    virtual void setGameStatus(const LocalKey &key, const std::vector<LocalParam> &params) override;

    // Send a chat message to all players in the lobby
    virtual void sendChatMessage(const Coercri::UTF8String &msg) override;

    // Returns all chat messages since the last call (or since joining if first call)
    virtual std::vector<ChatMessage> receiveChatMessages() override;

private:
    DummyOnlinePlatform* platform;
    std::string lobby_id;
    PlayerID leader_id;
    State current_state;
    std::chrono::steady_clock::time_point last_query_time;
    
    void updateCachedInfo();
};

#endif

#endif

/*
 * dummy_platform_lobby.cpp
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

#ifdef ONLINE_PLATFORM_DUMMY

#include "misc.hpp"
#include "dummy_platform_lobby.hpp"
#include "dummy_online_platform.hpp"
#include <chrono>

DummyPlatformLobby::DummyPlatformLobby(DummyOnlinePlatform* platform, const std::string& lobby_id)
    : platform(platform), lobby_id(lobby_id), current_state(State::JOINED)
{
}

DummyPlatformLobby::~DummyPlatformLobby()
{
    // Send MSG_LEAVE_LOBBY when the lobby is destroyed
    if (platform) {
        platform->sendMessage(DummyOnlinePlatform::MSG_LEAVE_LOBBY, "");
        std::string response;
        platform->receiveResponse(response);
    }
}

PlatformLobby::State DummyPlatformLobby::getState()
{
    return current_state;
}

std::string DummyPlatformLobby::getLeaderId()
{
    auto now = std::chrono::steady_clock::now();
    
    // Only query the server at infrequent intervals
    constexpr int QUERY_TIME_MS = 3000;
    if (leader_id.empty() ||
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_leader_query_time).count() >= QUERY_TIME_MS) {
        
        if (platform) {
            if (platform->sendMessage(DummyOnlinePlatform::MSG_GET_LOBBY_INFO, lobby_id)) {
                std::string response_data;
                if (platform->receiveResponse(response_data)) {
                    // Parse response: leader_id (null-terminated) + num_players (4 bytes) + status_code (4 bytes)
                    size_t null_pos = response_data.find('\0');
                    if (null_pos != std::string::npos) {
                        leader_id = response_data.substr(0, null_pos);
                    }
                }
            }
        }
        
        last_leader_query_time = now;
    }
    
    return leader_id;
}

void DummyPlatformLobby::setStatusCode(int status_code)
{
    if (platform) {
        // Send status_code - leader_id and num_players are managed server-side
        std::string payload;
        payload.resize(4); // 4 bytes
        *reinterpret_cast<int32_t*>(&payload[0]) = status_code;
        
        platform->sendMessage(DummyOnlinePlatform::MSG_SET_LOBBY_INFO, payload);
        std::string response;
        platform->receiveResponse(response);
    }
}

#endif

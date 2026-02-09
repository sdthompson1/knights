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
#include "localization.hpp"
#include "utf8string.hpp"
#include <chrono>
#include <cstring>

#define PLATFORM_NAME "dummy"

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
    updateCachedInfo();
    return current_state;
}

PlayerID DummyPlatformLobby::getLeaderId()
{
    updateCachedInfo();
    return leader_id;
}

void DummyPlatformLobby::setGameStatus(const LocalKey &key, const std::vector<LocalParam> &params)
{
    if (platform) {
        // Build payload: status_key (null-terminated) + has_param (1 byte) + 
        //               [optional param_key (null-terminated)]
        std::string payload;
        
        // Add status key
        payload += key.getKey();
        payload += '\0';
        
        // Check if we have exactly one LocalKey parameter (as per assumption)
        bool has_param = false;
        std::string param_key;
        if (!params.empty() && params[0].getType() == LocalParam::Type::LOCAL_KEY) {
            has_param = true;
            param_key = params[0].getLocalKey().getKey();
        }
        
        // Add has_param flag
        payload += static_cast<char>(has_param ? 1 : 0);
        
        // Add parameter key if present
        if (has_param) {
            payload += param_key;
            payload += '\0';
        }
        
        platform->sendMessage(DummyOnlinePlatform::MSG_SET_LOBBY_INFO, payload);
        std::string response;
        platform->receiveResponse(response);
    }
}

void DummyPlatformLobby::updateCachedInfo()
{
    auto now = std::chrono::steady_clock::now();
    
    // Only query the server at infrequent intervals
    constexpr int QUERY_TIME_MS = 3000;
    if (leader_id.empty() ||
    std::chrono::duration_cast<std::chrono::milliseconds>(now - last_query_time).count() >= QUERY_TIME_MS) {
        
        if (platform) {
            if (platform->sendMessage(DummyOnlinePlatform::MSG_GET_LOBBY_INFO, lobby_id)) {
                std::string response_data;
                if (platform->receiveResponse(response_data)) {
                    // Parse response: leader_id (null-terminated) + num_players (4 bytes) +
                    //                checksum (null-terminated) +
                    //                status_key (null-terminated) + has_param (1 byte) +
                    //                [optional param_key (null-terminated)] + lobby_state (null-terminated)
                    size_t pos = 0;

                    // Parse leader_id
                    size_t null_pos = response_data.find('\0', pos);
                    if (null_pos != std::string::npos) {
                        leader_id = PlayerID(PLATFORM_NAME,
                                             response_data.substr(pos, null_pos - pos),
                                             UTF8String());
                        pos = null_pos + 1;

                        // Skip num_players (4 bytes)
                        pos += 4;

                        // Skip checksum (null-terminated)
                        null_pos = response_data.find('\0', pos);
                        if (null_pos != std::string::npos) {
                            pos = null_pos + 1;

                            // Skip status_key (null-terminated)
                            null_pos = response_data.find('\0', pos);
                            if (null_pos != std::string::npos) {
                                pos = null_pos + 1;

                                // Skip has_param (1 byte) and optional param_key
                                if (pos < response_data.length()) {
                                    bool has_param = response_data[pos] != 0;
                                    pos += 1;

                                    if (has_param) {
                                        // Skip param_key (null-terminated)
                                        null_pos = response_data.find('\0', pos);
                                        if (null_pos != std::string::npos) {
                                            pos = null_pos + 1;
                                        }
                                    }

                                    // Parse lobby_state
                                    if (pos < response_data.length()) {
                                        null_pos = response_data.find('\0', pos);
                                        if (null_pos != std::string::npos) {
                                            std::string state_str = response_data.substr(pos, null_pos - pos);
                                            if (state_str == "JOINED") {
                                                current_state = State::JOINED;
                                            } else if (state_str == "FAILED") {
                                                current_state = State::FAILED;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        last_query_time = now;
    }
}

void DummyPlatformLobby::sendChatMessage(const Coercri::UTF8String &msg)
{
    if (platform) {
        // Build payload: message string + null terminator
        std::string payload = msg.asUTF8();
        payload += '\0';

        platform->sendMessage(DummyOnlinePlatform::MSG_SEND_CHAT, payload);
        std::string response;
        platform->receiveResponse(response);
        // Fire-and-forget, no error handling needed
    }
}

std::vector<ChatMessage> DummyPlatformLobby::receiveChatMessages()
{
    std::vector<ChatMessage> messages;

    if (!platform) {
        return messages;
    }

    if (platform->sendMessage(DummyOnlinePlatform::MSG_GET_CHAT, "")) {
        std::string response_data;
        if (platform->receiveResponse(response_data)) {
            // Parse response: num_messages (4 bytes) + array of (sender_id + message, both null-terminated)
            size_t pos = 0;

            // Read num_messages
            if (response_data.length() >= 4) {
                uint32_t num_messages;
                std::memcpy(&num_messages, response_data.data(), 4);
                pos = 4;

                // Parse each message
                for (uint32_t i = 0; i < num_messages && pos < response_data.length(); ++i) {
                    // Parse sender_id (null-terminated)
                    size_t null_pos = response_data.find('\0', pos);
                    if (null_pos == std::string::npos) {
                        break;  // Malformed response
                    }

                    std::string sender_id_str = response_data.substr(pos, null_pos - pos);
                    PlayerID sender_id(PLATFORM_NAME, sender_id_str, UTF8String());
                    pos = null_pos + 1;

                    // Parse message_text (null-terminated)
                    null_pos = response_data.find('\0', pos);
                    if (null_pos == std::string::npos) {
                        break;  // Malformed response
                    }

                    std::string message_text = response_data.substr(pos, null_pos - pos);
                    Coercri::UTF8String message = Coercri::UTF8String::fromUTF8Safe(message_text);
                    pos = null_pos + 1;

                    // Create ChatMessage and add to vector
                    messages.emplace_back(sender_id, message);
                }
            }
        }
    }

    return messages;
}

#endif

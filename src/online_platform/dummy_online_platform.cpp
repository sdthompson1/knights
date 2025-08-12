/*
 * dummy_online_platform.cpp
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
#include "dummy_online_platform.hpp"
#include "dummy_platform_lobby.hpp"

#include "enet/enet_network_driver.hpp"

#include <random>
#include <sstream>
#include <cstring>
#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Protocol constants
constexpr uint8_t MSG_LOGIN = 0x01;
constexpr uint8_t MSG_CREATE_LOBBY = 0x02;
constexpr uint8_t MSG_GET_LOBBY_LIST = 0x03;
constexpr uint8_t MSG_JOIN_LOBBY = 0x04;
constexpr uint8_t MSG_LEAVE_LOBBY = 0x05;
constexpr uint8_t MSG_GET_LOBBY_INFO = 0x06;
constexpr uint8_t MSG_SET_LOBBY_INFO = 0x07;

constexpr uint8_t STATUS_SUCCESS = 0x00;
constexpr uint8_t STATUS_ERROR = 0x01;

constexpr uint16_t LOBBY_SERVER_PORT = 12345;

DummyOnlinePlatform::DummyOnlinePlatform() : socket_fd(-1), connected(false)
{
    // Generate a random user ID (random integer as string)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(100000, 999999);
    
    std::ostringstream oss;
    oss << dis(gen);
    current_user_id = oss.str();

    // Create dummy network driver
    create_network_driver(current_user_id);

    // Connect to server and login
    if (connect_to_server()) {
        sendMessage(MSG_LOGIN, current_user_id);
        std::string response;
        receiveResponse(response);
    }
}

std::string DummyOnlinePlatform::getCurrentUserId() const
{
    return current_user_id;
}

Coercri::UTF8String DummyOnlinePlatform::lookupUserName(const std::string& platform_user_id) const
{
    return Coercri::UTF8String::fromUTF8(platform_user_id);
}

std::unique_ptr<PlatformLobby> DummyOnlinePlatform::createLobby(Visibility vis)
{
    // Note: 'vis' parameter is ignored for DummyOnlinePlaform

    if (!connected) return nullptr;
    
    if (sendMessage(MSG_CREATE_LOBBY, "")) {
        std::string lobby_id;
        if (receiveResponse(lobby_id)) {
            current_lobby_id = lobby_id;
            current_lobby_leader = current_user_id;
            return std::make_unique<DummyPlatformLobby>(this, lobby_id, current_user_id);
        }
    }
    return nullptr;
}

std::vector<std::string> DummyOnlinePlatform::getLobbyList()
{
    std::vector<std::string> result;
    
    if (!connected) return result;
    
    if (sendMessage(MSG_GET_LOBBY_LIST, "")) {
        std::string response_data;
        if (receiveResponse(response_data)) {
            // Parse response: number of lobbies (4 bytes) + null-terminated strings
            if (response_data.size() >= 4) {
                // Note: reinterpret_cast-ing to uint32_t* is not technically
                // valid, because of possible alignment and/or endianness issues,
                // but it should be fine here because this is debug-only code!
                uint32_t count = *reinterpret_cast<const uint32_t*>(response_data.data());
                size_t pos = 4;
                
                for (uint32_t i = 0; i < count && pos < response_data.size(); ++i) {
                    size_t null_pos = response_data.find('\0', pos);
                    if (null_pos != std::string::npos) {
                        result.push_back(response_data.substr(pos, null_pos - pos));
                        pos = null_pos + 1;
                    } else {
                        break;
                    }
                }
            }
        }
    }
    
    return result;
}

std::unique_ptr<PlatformLobby> DummyOnlinePlatform::joinLobby(const std::string& lobby_id)
{
    if (!connected) return nullptr;
    
    if (sendMessage(MSG_JOIN_LOBBY, lobby_id)) {
        std::string leader_id;
        if (receiveResponse(leader_id)) {
            current_lobby_id = lobby_id;
            current_lobby_leader = leader_id;
            return std::make_unique<DummyPlatformLobby>(this, lobby_id, leader_id);
        }
    }
    return nullptr;
}

OnlinePlatform::LobbyInfo DummyOnlinePlatform::getLobbyInfo(const std::string &lobby_id)
{
    LobbyInfo info;
    info.leader_id = "";
    info.num_players = 0;
    info.status_code = 0;
    
    if (!connected) return info;
    
    if (sendMessage(MSG_GET_LOBBY_INFO, lobby_id)) {
        std::string response_data;
        if (receiveResponse(response_data)) {
            // Parse response: leader_id (null-terminated) + num_players (4 bytes) + status_code (4 bytes)
            size_t pos = 0;
            size_t null_pos = response_data.find('\0', pos);
            if (null_pos != std::string::npos) {
                info.leader_id = response_data.substr(pos, null_pos - pos);
                pos = null_pos + 1;
                
                if (pos + 8 <= response_data.size()) { // 4 + 4 bytes
                    info.num_players = *reinterpret_cast<const uint32_t*>(response_data.data() + pos);
                    pos += 4;
                    
                    info.status_code = *reinterpret_cast<const int32_t*>(response_data.data() + pos);
                }
            }
        }
    }
    
    return info;
}

bool DummyOnlinePlatform::connect_to_server()
{
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        return false;
    }
    
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(LOBBY_SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (connect(socket_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    
    connected = true;
    return true;
}

bool DummyOnlinePlatform::sendMessage(uint8_t msg_type, const std::string& payload)
{
    if (!connected) return false;
    
    // Create message: type (1 byte) + length (4 bytes) + payload
    uint32_t payload_length = payload.length();
    
    char header[5];
    header[0] = msg_type;
    memcpy(header + 1, &payload_length, 4);
    
    // Send header
    if (send(socket_fd, header, 5, 0) != 5) {
        return false;
    }
    
    // Send payload if any
    if (payload_length > 0) {
        if (send(socket_fd, payload.data(), payload_length, 0) != (ssize_t)payload_length) {
            return false;
        }
    }
    
    return true;
}

bool DummyOnlinePlatform::receiveResponse(std::string& response_data)
{
    if (!connected) return false;
    
    // Read response header: status (1 byte) + data length (4 bytes)
    char header[5];
    if (recv(socket_fd, header, 5, MSG_WAITALL) != 5) {
        return false;
    }
    
    uint8_t status = header[0];
    uint32_t data_length;
    memcpy(&data_length, header + 1, 4);
    
    // Read response data
    response_data.resize(data_length);
    if (data_length > 0) {
        if (recv(socket_fd, &response_data[0], data_length, MSG_WAITALL) != (ssize_t)data_length) {
            return false;
        }
    }
    
    return status == STATUS_SUCCESS;
}


// NetworkDriver implementation (modification of EnetNetworkDriver - makes
// "fake" connections to localhost)

class DummyNetworkDriver : public Coercri::EnetNetworkDriver {
public:
    explicit DummyNetworkDriver(const std::string & my_user_id)
        : EnetNetworkDriver(20, 1, true)
    {
        // Serve on the port corresponding to our user id
        Coercri::EnetNetworkDriver::setServerPort(userNameToPortNum(my_user_id));
    }

    boost::shared_ptr<Coercri::NetworkConnection> openConnection(const std::string &username, int port)
    {
        // Override port
        port = userNameToPortNum(username);

        // Connect to localhost
        return Coercri::EnetNetworkDriver::openConnection("localhost", port);
    }

    void setServerPort(int port) {
        // Ignore application-set port, use our own port number
    }

private:
    static int userNameToPortNum(const std::string &name) {
        // Use port numbers 10000 - 12000
        int p = std::stoi(name);
        return 10000 + (p % 2000);
    }

    int my_port_num;
};

Coercri::NetworkDriver & DummyOnlinePlatform::getNetworkDriver()
{
    return *network_driver;
}

void DummyOnlinePlatform::create_network_driver(const std::string &my_user_id)
{
    network_driver.reset(new DummyNetworkDriver(my_user_id));
}

#endif

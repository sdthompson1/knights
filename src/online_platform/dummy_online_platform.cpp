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
#include "localization.hpp"

#include "enet/enet_network_driver.hpp"
#include "network/byte_buf.hpp"
#include "network/network_connection.hpp"

#include <random>
#include <sstream>
#include <cstring>

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
    current_user_id = PlayerID(oss.str());

    // Create dummy network driver
    create_network_driver(current_user_id);

    // Connect to server and login
    if (connect_to_server()) {
        sendMessage(MSG_LOGIN, current_user_id.asString());
        std::string response;
        receiveResponse(response);
    }
}

PlayerID DummyOnlinePlatform::getCurrentUserId()
{
    return current_user_id;
}

Coercri::UTF8String DummyOnlinePlatform::lookupUserName(const PlayerID& platform_user_id)
{
    return Coercri::UTF8String::fromUTF8("@" + platform_user_id.asString());
}

std::unique_ptr<PlatformLobby> DummyOnlinePlatform::createLobby(Visibility vis, uint64_t checksum)
{
    // Note: 'vis' parameter is ignored for DummyOnlinePlaform

    if (!connected) return nullptr;

    std::ostringstream oss;
    oss << checksum;
    if (sendMessage(MSG_CREATE_LOBBY, oss.str())) {
        std::string lobby_id;
        if (receiveResponse(lobby_id)) {
            return std::make_unique<DummyPlatformLobby>(this, lobby_id);
        }
    }
    return nullptr;
}

void DummyOnlinePlatform::clearLobbyFilters()
{
    filter_checksum.reset();
}

void DummyOnlinePlatform::addChecksumFilter(uint64_t checksum)
{
    filter_checksum = checksum;
}

std::vector<std::string> DummyOnlinePlatform::getLobbyList()
{
    std::vector<std::string> result;

    if (!connected) return result;

    // Check cache: if we have data from less than 5 seconds ago, return cached result
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_lobby_list_time);

    if (elapsed.count() < 5) {
        return cached_lobby_list;
    }

    // Otherwise, fetch from server
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
                        std::string lobby_id = response_data.substr(pos, null_pos - pos);
                        pos = null_pos + 1;
                        null_pos = response_data.find('\0', pos);
                        if (null_pos != std::string::npos) {
                            std::string checksum_str = response_data.substr(pos, null_pos - pos);
                            pos = null_pos + 1;
                            std::istringstream iss(checksum_str);
                            uint64_t checksum = 0;
                            iss >> checksum;
                            if (!filter_checksum.has_value() || *filter_checksum == checksum) {
                                result.push_back(lobby_id);
                            }
                        } else {
                            break;
                        }
                    } else {
                        break;
                    }
                }
            }

            // Cache the result and timestamp
            cached_lobby_list = result;
            last_lobby_list_time = now;
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
            return std::make_unique<DummyPlatformLobby>(this, lobby_id);
        }
    }
    return nullptr;
}

OnlinePlatform::LobbyInfo DummyOnlinePlatform::getLobbyInfo(const std::string &lobby_id)
{
    LobbyInfo info;
    info.num_players = 0;
    info.game_status_key = LocalKey("waiting");

    if (!connected) return info;

    if (sendMessage(MSG_GET_LOBBY_INFO, lobby_id)) {
        std::string response_data;
        if (receiveResponse(response_data)) {
            // Parse response: leader_id (null-terminated) + num_players (4 bytes) +
            //                 checksum (null-terminated) +
            //                 status_key (null-terminated) + has_param (1 byte) + 
            //                 [optional param_key (null-terminated)]
            size_t pos = 0;
            size_t null_pos = response_data.find('\0', pos);
            if (null_pos != std::string::npos) {
                info.leader_id = PlayerID(response_data.substr(pos, null_pos - pos));
                pos = null_pos + 1;
            }

            if (pos + 4 <= response_data.size()) {
                info.num_players = *reinterpret_cast<const uint32_t*>(response_data.data() + pos);
                pos += 4;
            }

            // Parse checksum
            null_pos = response_data.find('\0', pos);
            if (null_pos != std::string::npos) {
                std::string checksum_str = response_data.substr(pos, null_pos - pos);
                pos = null_pos + 1;
                std::istringstream iss(checksum_str);
                iss >> info.checksum;
            }

            // Parse status key
            null_pos = response_data.find('\0', pos);
            if (null_pos != std::string::npos) {
                info.game_status_key = LocalKey(response_data.substr(pos, null_pos - pos));
                pos = null_pos + 1;
            }

            // Check if there's a parameter
            if (pos < response_data.size()) {
                uint8_t has_param = response_data[pos];
                pos += 1;

                if (has_param && pos < response_data.size()) {
                    null_pos = response_data.find('\0', pos);
                    if (null_pos != std::string::npos) {
                        LocalKey param_key(response_data.substr(pos, null_pos - pos));
                        pos = null_pos + 1;
                        info.game_status_params.push_back(LocalParam(param_key));
                    }
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

class DummyNetworkConnectionWrapper : public Coercri::NetworkConnection {
public:
    DummyNetworkConnectionWrapper(boost::shared_ptr<Coercri::NetworkConnection> underlying,
                                  const PlayerID &user_id,
                                  const std::vector<unsigned char> &received_data)
        : underlying(underlying),
          user_id(user_id),
          received_data(received_data)
    {}

    virtual State getState() const { return underlying->getState(); }
    virtual void close() { underlying->close(); }
    virtual void receive(std::vector<unsigned char> &data) {
        if (!received_data.empty()) {
            data.swap(received_data);
            received_data.clear();
        } else {
            underlying->receive(data);
        }
    }
    virtual void send(const std::vector<unsigned char> &data) { underlying->send(data); }
    virtual std::string getAddress() { return user_id.asString(); }
    virtual int getPingTime() { return underlying->getPingTime(); }

private:
    boost::shared_ptr<Coercri::NetworkConnection> underlying;
    PlayerID user_id;
    std::vector<unsigned char> received_data;
};

class DummyNetworkDriver : public Coercri::EnetNetworkDriver {
public:
    explicit DummyNetworkDriver(const PlayerID & my_user_id)
        : EnetNetworkDriver(20, 1, true),
          my_user_id(my_user_id)
    {
        // Serve on the port corresponding to our user id
        Coercri::EnetNetworkDriver::setServerPort(userIdToPortNum(my_user_id));
    }

    boost::shared_ptr<Coercri::NetworkConnection> openConnection(const std::string &addr, int port) override
    {
        // Connect to localhost
        PlayerID user_id(addr);  // Interpret address as platform user id
        auto conn = Coercri::EnetNetworkDriver::openConnection("localhost", userIdToPortNum(user_id));

        // A real online platform would authenticate users, but here we
        // fake that by sending our user_id as the first message
        std::vector<unsigned char> msg;
        Coercri::OutputByteBuf buf(msg);
        buf.writeString(my_user_id.asString());
        conn->send(msg);

        std::vector<unsigned char> empty;

        return boost::shared_ptr<Coercri::NetworkConnection>(new DummyNetworkConnectionWrapper(conn, user_id, empty));
    }

    Coercri::NetworkDriver::Connections pollIncomingConnections() override
    {
        auto connections = Coercri::EnetNetworkDriver::pollIncomingConnections();

        for (boost::shared_ptr<Coercri::NetworkConnection> & conn : connections) {
            std::vector<unsigned char> msg;
            while (msg.empty()) {
                conn->receive(msg);
            }

            // Read out the transmitted user id
            Coercri::InputByteBuf buf(msg);
            PlayerID id = PlayerID(buf.readString());

            // Remove the consumed bytes from the msg
            msg.erase(msg.begin(), msg.begin() + buf.getPos());

            // Wrap the connection
            boost::shared_ptr<DummyNetworkConnectionWrapper> wrapped_conn
                ( new DummyNetworkConnectionWrapper(conn, id, msg) );

            // Replace connection with its wrapped equivalent
            conn = wrapped_conn;
        }

        return connections;
    }

    void setServerPort(int port) override {
        // Ignore application-set port, use our own port number
    }

private:
    static int userIdToPortNum(const PlayerID &id) {
        // Use port numbers 10000 - 12000
        int p = std::stoi(id.asString());
        return 10000 + (p % 2000);
    }

    PlayerID my_user_id;
};

Coercri::NetworkDriver & DummyOnlinePlatform::getNetworkDriver()
{
    return *network_driver;
}

void DummyOnlinePlatform::create_network_driver(const PlayerID &my_user_id)
{
    network_driver.reset(new DummyNetworkDriver(my_user_id));
}

#endif

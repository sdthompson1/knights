/*
 * lobby_controller.cpp
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

#include "misc.hpp"

#include "knights_client.hpp"
#include "lobby_controller.hpp"
#include "simple_knights_lobby.hpp"
#include "vm_knights_lobby.hpp"

LobbyController::LobbyController()
{
#ifdef USE_VM_LOBBY
    vm_knights_lobby = nullptr;
    host_migration_state = HostMigrationState::NOT_IN_GAME;
#endif
#ifdef ONLINE_PLATFORM
    created_by_me = false;
#endif
}

void LobbyController::resetAll()
{
    knights_client.reset();
    knights_lobby.reset();

#ifdef ONLINE_PLATFORM
    platform_lobby.reset();
    created_by_me = false;
#endif

#ifdef USE_VM_LOBBY
    vm_knights_lobby = nullptr;
    vm_lobby_leader_id = PlayerID();
    host_migration_state = HostMigrationState::NOT_IN_GAME;
#endif
}

boost::shared_ptr<KnightsClient>
    LobbyController::startLocalGame(boost::shared_ptr<Coercri::Timer> timer,
                                    boost::shared_ptr<KnightsConfig> config,
                                    const std::string &game_name)
{
    knights_lobby.reset(new SimpleKnightsLobby(timer, config, game_name));
    knights_client.reset(new KnightsClient(true)); // Allow untrusted strings for local game
    return knights_client;
}

boost::shared_ptr<KnightsClient>
    LobbyController::hostLanGame(Coercri::NetworkDriver &net_driver,
                                 boost::shared_ptr<Coercri::Timer> timer,
                                 int port,
                                 boost::shared_ptr<KnightsConfig> config,
                                 const std::string &game_name)
{
    knights_lobby.reset(new SimpleKnightsLobby(net_driver, timer, port, config, game_name));
    knights_client.reset(new KnightsClient(true)); // Allow untrusted strings for LAN game
    return knights_client;
}

boost::shared_ptr<KnightsClient>
    LobbyController::joinRemoteServer(Coercri::NetworkDriver &net_driver,
                                      boost::shared_ptr<Coercri::Timer> timer,
                                      const std::string &address,
                                      int port)
{
    knights_lobby.reset(new SimpleKnightsLobby(net_driver, timer, address, port));
    knights_client.reset(new KnightsClient(true)); // Allow untrusted strings (we assume that players trust any server that they choose to connect to)
    return knights_client;
}

#if defined(ONLINE_PLATFORM) && defined(USE_VM_LOBBY)
// If lobby_id empty this creates a new lobby (with given visibility),
// otherwise it joins an existing lobby
boost::shared_ptr<KnightsClient>
    LobbyController::createVMGame(OnlinePlatform &online_platform,
                                  const std::string &lobby_id,
                                  OnlinePlatform::Visibility vis,
                                  std::unique_ptr<VMKnightsLobby> kts_lobby,
                                  uint64_t my_checksum)
{
    // Create the platform lobby
    if (lobby_id.empty()) {
        platform_lobby = online_platform.createLobby(vis, my_checksum);
        created_by_me = true;

        if (platform_lobby) {
            // Set initial status
            std::vector<LocalParam> no_params;
            platform_lobby->setGameStatus(LocalKey("selecting_quest"), no_params);
        } else {
            throw std::runtime_error("Failed to create lobby");
        }

    } else {
        platform_lobby = online_platform.joinLobby(lobby_id);
        created_by_me = false;

        if (!platform_lobby) {
            throw std::runtime_error("Failed to join lobby");
        }
    }

    // Install the given knights lobby
    vm_knights_lobby = kts_lobby.get();
    knights_lobby = std::move(kts_lobby);
    vm_lobby_leader_id = PlayerID();

    // Create the client
    knights_client.reset(new KnightsClient(false)); // Don't trust arbitrary strings coming from VM
    return knights_client;
}
#endif

#if defined(ONLINE_PLATFORM) && defined(USE_VM_LOBBY)
HostMigrationState LobbyController::getHostMigrationState() const
{
    return host_migration_state;
}

void LobbyController::setHostMigrationStateInGame()
{
    host_migration_state = HostMigrationState::IN_GAME;
}

PlayerID LobbyController::getCurrentLeader() const
{
    if (platform_lobby) {
        return platform_lobby->getLeaderId();
    } else {
        return PlayerID();
    }
}

void LobbyController::inviteFriendToLobby()
{
    if (platform_lobby) {
        platform_lobby->openInviteUI();
    }
}

void LobbyController::checkHostMigration(OnlinePlatform &online_platform,
                                         bool new_control_system,
                                         UTF8String &err_msg,
                                         LocalKey &host_migration_key,
                                         bool &del_gfx_sounds)
{
    if (platform_lobby && platform_lobby->getState() == PlatformLobby::State::FAILED) {
        // We were kicked out of the lobby for some reason. Go to ErrorScreen.
        if (host_migration_state == HostMigrationState::NOT_IN_GAME) {
            if (created_by_me) {
                err_msg = UTF8String::fromUTF8("Failed to create lobby");
            } else {
                err_msg = UTF8String::fromUTF8("Failed to join lobby");
            }
        } else {
            err_msg = UTF8String::fromUTF8("Lobby connection lost");
        }

    } else if (platform_lobby
               && platform_lobby->getState() == PlatformLobby::State::JOINED
               && !platform_lobby->getLeaderId().empty()
               && platform_lobby->getLeaderId() != vm_lobby_leader_id) {

        // Platform lobby has finished setting up, OR leader has changed

        if (host_migration_state == HostMigrationState::NOT_IN_GAME) {
            // Now that platform lobby is ready, we can send the initial login messages
            knights_client->setPlayerIdAndControls(online_platform.getCurrentUserId(),
                                                   new_control_system);
            knights_client->joinGame("#VMGame");
            sendOutgoingMessages();
        }
        
        vm_lobby_leader_id = platform_lobby->getLeaderId();

        if (vm_lobby_leader_id == online_platform.getCurrentUserId()) {
            vm_knights_lobby->becomeLeader(0);  // Dummy port number
        } else {
            vm_knights_lobby->becomeFollower(vm_lobby_leader_id.asString(), 0);  // Use leader's PlayerID as the address
        }

        // Unload graphics and sounds, as they will be reloaded when we rejoin the
        // new server
        del_gfx_sounds = true;

        // Show the "Host migration in progress" screen while we wait
        const char *msg_key;
        if (host_migration_state == HostMigrationState::NOT_IN_GAME) {
            host_migration_state = HostMigrationState::CONNECTING;
            msg_key = "connecting_to_game";
        } else {
            host_migration_state = HostMigrationState::MIGRATING;
            msg_key = "host_migration_in_progress";
        }
        host_migration_key = LocalKey(msg_key);

    } else if (platform_lobby
               && host_migration_state == HostMigrationState::IN_GAME
               && !vm_knights_lobby->connected()) {

        // Lost connection, and the VMKnightsLobby is attempting to reconnect.
        // Here we just change the screen.
        host_migration_state = HostMigrationState::RECONNECTING;
        host_migration_key = LocalKey("connection_lost_reconnecting");
    }
}
#endif

#ifdef ONLINE_PLATFORM
void LobbyController::setQuestMessageCode(OnlinePlatform &online_platform, const LocalKey &quest_key)
{
    if (platform_lobby
    && platform_lobby->getLeaderId() == online_platform.getCurrentUserId()) {
        LocalKey key;
        std::vector<LocalParam> params;
        if (quest_key == LocalKey()) {
            key = LocalKey("selecting_quest");
        } else {
            key = LocalKey("playing_x");
            params.push_back(LocalParam(quest_key));
        }
        platform_lobby->setGameStatus(key, params);
    }
}
#endif

void LobbyController::readIncomingMessages()
{
    if (knights_lobby) {
        knights_lobby->readIncomingMessages(*knights_client);
    }
}

void LobbyController::sendOutgoingMessages()
{
    if (knights_lobby) {
        knights_lobby->sendOutgoingMessages(*knights_client);
    }
}

int LobbyController::getNumberOfPlayers() const
{
    if (knights_lobby) {
        return knights_lobby->getNumberOfPlayers();
    } else {
        return 0;
    }
}

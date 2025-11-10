/*
 * lobby_controller.hpp
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

#ifndef LOBBY_CONTROLLER_HPP
#define LOBBY_CONTROLLER_HPP

#include "knights_lobby.hpp"
#include "online_platform.hpp"
#include "player_id.hpp"
#include "utf8string.hpp"

#include "boost/shared_ptr.hpp"

#include <memory>

class KnightsClient;
class KnightsConfig;
class LocalKey;
class VMKnightsLobby;

namespace Coercri {
    class NetworkDriver;
    class Timer;
}

// Host migration states
enum class HostMigrationState {
    NOT_IN_GAME,   // Not in any game and not attempting to join
    IN_GAME,       // Currently in a game
    CONNECTING,    // Initial attempt to connect
    RECONNECTING,  // We lost connection to leader, attempting to restore
    MIGRATING      // We are migrating to a new leader
};

// This class manages the KnightsLobby and PlatformLobby on behalf of
// the KnightsApp. Basically it brings together the functionality
// provided by KnightsLobby and PlatformLobby in order to manage the
// lifecycle of a Knights game. It supports both VM games (with host
// migration) and simple local or LAN games (without host migration
// support).

class LobbyController {
public:
    LobbyController();


    // Reset

    void resetAll();


    // Creating new games

    boost::shared_ptr<KnightsClient> startLocalGame(boost::shared_ptr<Coercri::Timer> timer,
                                                    boost::shared_ptr<KnightsConfig> config,
                                                    const std::string &game_name);

    boost::shared_ptr<KnightsClient> hostLanGame(Coercri::NetworkDriver &net_driver,
                                                 boost::shared_ptr<Coercri::Timer> timer,
                                                 int port,
                                                 boost::shared_ptr<KnightsConfig> config,
                                                 const std::string &game_name);

    boost::shared_ptr<KnightsClient> joinRemoteServer(Coercri::NetworkDriver &net_driver,
                                                      boost::shared_ptr<Coercri::Timer> timer,
                                                      const std::string &address,
                                                      int port);

#if defined(ONLINE_PLATFORM) && defined(USE_VM_LOBBY)
    // If lobby_id empty this creates a new lobby (with given visibility),
    // otherwise it joins an existing lobby
    boost::shared_ptr<KnightsClient> createVMGame(OnlinePlatform &online_platform,
                                                  const std::string &lobby_id,
                                                  OnlinePlatform::Visibility vis,
                                                  std::unique_ptr<VMKnightsLobby> kts_lobby);
#endif


    // Host migration
#if defined(ONLINE_PLATFORM) && defined(USE_VM_LOBBY)
    HostMigrationState getHostMigrationState() const;
    void setHostMigrationStateInGame();  // Called by GameManager to notify that game has begun
    PlayerID getCurrentLeader() const;

    // This is called periodically (by KnightsApp::updateOnlinePlatform)
    // On output, if err_msg non-empty, should go to ErrorScreen
    // On output, if host_migration_key non-empty, should go to HostMigrationScreen
    // On output, if del_gfx_sounds is true, should clear gfx_manager and sound_manager
    // [If these parameters seem a little weird, it is because this class was refactored out
    // from some other code, and not everything has been fully disentangled at this time!]
    void checkHostMigration(OnlinePlatform &online_platform,
                            bool new_control_system,
                            UTF8String &err_msg,
                            LocalKey &host_migration_key,
                            bool &del_gfx_sounds);
#endif


    // Set quest message code (updates platform lobby status info if applicable)
#ifdef ONLINE_PLATFORM
    void setQuestMessageCode(OnlinePlatform &online_platform, const LocalKey &quest_key);
#endif


    // Called during main game loop

    void readIncomingMessages();
    void sendOutgoingMessages();


    // Get number of players, if known (returns 0 if not known)

    int getNumberOfPlayers() const;


private:
    boost::shared_ptr<KnightsClient> knights_client;
    std::unique_ptr<KnightsLobby> knights_lobby;

#ifdef ONLINE_PLATFORM
    std::unique_ptr<PlatformLobby> platform_lobby;
    bool created_by_me;  // True if the platform lobby was originally created by the current user
#endif

#ifdef USE_VM_LOBBY
    VMKnightsLobby *vm_knights_lobby;
    PlayerID vm_lobby_leader_id;
    HostMigrationState host_migration_state;
#endif
};

#endif   // LOBBY_CONTROLLER_HPP

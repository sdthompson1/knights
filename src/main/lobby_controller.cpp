/*
 * lobby_controller.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2026.
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

#include "client_callbacks.hpp"
#include "exception_base.hpp"
#include "knights_client.hpp"
#include "knights_config.hpp"
#include "lobby_controller.hpp"
#include "module_manager.hpp"
#include "simple_knights_lobby.hpp"
#include "vfs.hpp"
#include "vm_knights_lobby.hpp"

namespace {
    struct LoaderBase {
        LoaderBase(boost::mutex &mutex,
                   bool &done,
                   std::unique_ptr<KnightsLobby> &knights_lobby_out,
#ifdef USE_VM_LOBBY
                   VMKnightsLobby *& vm_knights_lobby_out,
#endif
                   LocalMsg &error_msg_out,
                   std::unique_ptr<LuaError> &lua_error_out)
            : done(done),
              knights_lobby_out(knights_lobby_out),
#ifdef USE_VM_LOBBY
              vm_knights_lobby_out(vm_knights_lobby_out),
#endif
              error_msg_out(error_msg_out),
              lua_error_out(lua_error_out),
              mutex(mutex)
        {
            done = false;
            knights_lobby_out.reset();
#ifdef USE_VM_LOBBY
            vm_knights_lobby_out = nullptr;
#endif
            error_msg_out = LocalMsg();
            lua_error_out.reset();
        }

        virtual void load(std::unique_ptr<KnightsLobby> &knights_lobby
#ifdef USE_VM_LOBBY
                          , VMKnightsLobby *& vm_knights_lobby
#endif
                          ) = 0;

        void operator()() {
            try {
                std::unique_ptr<KnightsLobby> kl;
#ifdef USE_VM_LOBBY
                VMKnightsLobby *vkl = nullptr;
#endif
                load(kl
#ifdef USE_VM_LOBBY
                     , vkl
#endif
                     );
                boost::unique_lock lock(mutex);
                done = true;
                knights_lobby_out = std::move(kl);
#ifdef USE_VM_LOBBY
                vm_knights_lobby_out = vkl;
#endif

            } catch (LuaError &err) {
                boost::unique_lock lock(mutex);
                done = true;
                lua_error_out.reset(new LuaError(err));

            } catch (ExceptionBase &err) {
                boost::unique_lock lock(mutex);
                done = true;
                error_msg_out = err.getMsg();

            } catch (std::exception &e) {
                boost::unique_lock lock(mutex);
                done = true;
                error_msg_out = {LocalKey("cxx_error_is"), {LocalParam(Coercri::UTF8String::fromUTF8Safe(e.what()))}};

            } catch (...) {
                boost::unique_lock lock(mutex);
                done = true;
                error_msg_out = {LocalKey("unknown_error")};
            }
        }

        boost::mutex &mutex;
        bool &done;
        std::unique_ptr<KnightsLobby> &knights_lobby_out;
#ifdef USE_VM_LOBBY
        VMKnightsLobby *& vm_knights_lobby_out;
#endif
        LocalMsg &error_msg_out;
        std::unique_ptr<LuaError> &lua_error_out;
    };

    struct SimpleLoader : LoaderBase {
        SimpleLoader(boost::shared_ptr<Coercri::Timer> timer,
                     std::vector<std::string> module_names,
                     VFS module_vfs,
                     Coercri::NetworkDriver *net_driver,
                     int port,
                     boost::mutex &mutex,
                     bool &done,
                     std::unique_ptr<KnightsLobby> &knights_lobby_out,
#ifdef USE_VM_LOBBY
                     VMKnightsLobby *& dummy,
#endif
                     LocalMsg &error_msg_out,
                     std::unique_ptr<LuaError> &lua_error_out)
            : LoaderBase(mutex,
                         done,
                         knights_lobby_out,
#ifdef USE_VM_LOBBY
                         dummy,
#endif
                         error_msg_out,
                         lua_error_out),
              timer(std::move(timer)),
              module_names(std::move(module_names)),
              module_vfs(std::move(module_vfs)),
              net_driver(net_driver),
              port(port)
        { }

        void load(std::unique_ptr<KnightsLobby> &knights_lobby
#ifdef USE_VM_LOBBY
                  , VMKnightsLobby *& dummy
#endif
                  ) override
        {
            bool menu_strict = (net_driver == nullptr);
            boost::shared_ptr<KnightsConfig> config(new KnightsConfig(module_vfs, module_names, menu_strict));

            if (net_driver) {
                // LAN mode
                knights_lobby.reset(new SimpleKnightsLobby(*net_driver, timer, port, config, "#LanGame"));
            } else {
                // Single player or split screen mode (both use the name "#SplitScreenGame")
                knights_lobby.reset(new SimpleKnightsLobby(timer, config, "#SplitScreenGame"));
            }
        }

        boost::shared_ptr<Coercri::Timer> timer;
        std::vector<std::string> module_names;
        VFS module_vfs;
        Coercri::NetworkDriver *net_driver;
        int port;
    };

#ifdef USE_VM_LOBBY
    struct VMLoader : LoaderBase {
        VMLoader(Coercri::NetworkDriver &net_driver,
                 Coercri::Timer &timer,
                 PlayerID local_user_id,
                 bool new_control_system,
                 std::vector<std::string> module_names,
                 VFS module_vfs,
                 boost::mutex &mutex,
                 bool &done,
                 std::unique_ptr<KnightsLobby> &knights_lobby_out,
                 VMKnightsLobby* &vm_knights_lobby_out,
                 LocalMsg &error_msg_out,
                 std::unique_ptr<LuaError> &lua_error_out)
            : LoaderBase(mutex,
                         done,
                         knights_lobby_out,
                         vm_knights_lobby_out,
                         error_msg_out,
                         lua_error_out),
              net_driver(net_driver),
              timer(timer),
              local_user_id(std::move(local_user_id)),
              new_control_system(new_control_system),
              module_names(std::move(module_names)),
              module_vfs(std::move(module_vfs))
        { }

        void load(std::unique_ptr<KnightsLobby> &knights_lobby,
                  VMKnightsLobby *& vm_knights_lobby) override
        {
            std::unique_ptr<VMKnightsLobby> lobby =
                std::make_unique<VMKnightsLobby>(net_driver,
                                                 timer,
                                                 local_user_id,
                                                 new_control_system,
                                                 std::move(module_names),
                                                 std::move(module_vfs));
            vm_knights_lobby = lobby.get();
            knights_lobby = std::move(lobby);
        }

        Coercri::NetworkDriver &net_driver;
        Coercri::Timer &timer;
        PlayerID local_user_id;
        bool new_control_system;
        std::vector<std::string> module_names;
        VFS module_vfs;
    };
#endif  // USE_VM_LOBBY
}

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
    // If the loader thread is still running, we must wait for it to
    // stop at this point.
    if (loader_thread.joinable()) {
        loader_thread.join();
    }

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

    loader_done = false;
    loader_error_msg = LocalMsg();
    loader_lua_error.reset();
}

boost::shared_ptr<KnightsClient>
    LobbyController::startLocalGame(boost::shared_ptr<Coercri::Timer> timer,
                                    const std::vector<std::string> &modules_to_load)
{
    resetAll();

    // Get the currently selected modules
    module_manager->update();  // Will block for short time, probably acceptable
    std::vector<std::string> module_names = module_manager->resolveModuleList(modules_to_load);
    VFS vfs = module_manager->getVFS(module_names);

    // Start loading the server in the background
    loader_thread = boost::thread(SimpleLoader(timer,
                                               std::move(module_names),
                                               std::move(vfs),
                                               nullptr,
                                               -1,
                                               loader_mutex,
                                               loader_done,
                                               knights_lobby,
#ifdef USE_VM_LOBBY
                                               vm_knights_lobby,
#endif
                                               loader_error_msg,
                                               loader_lua_error));

    // Make the KnightsClient
    knights_client.reset(new KnightsClient(true));  // Allow untrusted strings for local game
    return knights_client;
}

boost::shared_ptr<KnightsClient>
    LobbyController::hostLanGame(Coercri::NetworkDriver &net_driver,
                                 boost::shared_ptr<Coercri::Timer> timer,
                                 int port)
{
    resetAll();

    // Get the currently selected modules
    module_manager->update();  // Will block for short time, probably acceptable
    std::vector<std::string> module_names =
        module_manager->resolveModuleList(module_manager->getEnabledModules());
    VFS vfs = module_manager->getVFS(module_names);

    // Start loading the server in the background
    loader_thread = boost::thread(SimpleLoader(timer,
                                               std::move(module_names),
                                               std::move(vfs),
                                               &net_driver,
                                               port,
                                               loader_mutex,
                                               loader_done,
                                               knights_lobby,
#ifdef USE_VM_LOBBY
                                               vm_knights_lobby,
#endif
                                               loader_error_msg,
                                               loader_lua_error));

    // Make the KnightsClient
    knights_client.reset(new KnightsClient(true)); // Allow untrusted strings for LAN game
    return knights_client;
}

boost::shared_ptr<KnightsClient>
    LobbyController::joinLanGame(Coercri::NetworkDriver &net_driver,
                                 boost::shared_ptr<Coercri::Timer> timer,
                                 const std::string &address,
                                 int port)
{
    resetAll();

    // Ensure that we have up-to-date modules catalogue (joinGameAccepted
    // will need this)
    module_manager->update();  // Will block for short time, probably acceptable

    // Since we are joining we can just create the SimpleKnightsLobby directly here
    knights_lobby.reset(new SimpleKnightsLobby(net_driver, timer, address, port));
    knights_client.reset(new KnightsClient(true)); // Allow untrusted strings (we assume that players trust any LAN game that they choose to connect to)
    return knights_client;
}

#if defined(ONLINE_PLATFORM) && defined(USE_VM_LOBBY)
boost::shared_ptr<KnightsClient>
    LobbyController::hostOnlineGame(OnlinePlatform &online_platform,
                                    Coercri::Timer &timer,
                                    bool new_control_system,
                                    OnlinePlatform::Visibility vis)
{
    resetAll();

    // Get the modules we are using and their checksums
    module_manager->update();  // Will block for short time, probably acceptable
    GameModuleSpec spec;
    spec.module_names = module_manager->resolveModuleList(module_manager->getEnabledModules());
    spec.checksum = module_manager->computeCombinedChecksum(spec.module_names);
    VFS vfs = module_manager->getVFS(spec.module_names);

    // Create the platform lobby
    platform_lobby = online_platform.createLobby(vis, spec);
    created_by_me = true;

    if (platform_lobby) {
        // Set initial status
        std::vector<LocalParam> no_params;
        platform_lobby->setGameStatus(LocalKey("selecting_quest"), no_params);
    } else {
        throw ExceptionBase(LocalKey("failed_create_lobby"));
    }

    // Start creating the VMKnightsLobby in the background
    loader_thread = boost::thread(VMLoader(online_platform.getNetworkDriver(),
                                           timer,
                                           online_platform.getCurrentUserId(),
                                           new_control_system,
                                           spec.module_names,
                                           std::move(vfs),
                                           loader_mutex,
                                           loader_done,
                                           knights_lobby,
                                           vm_knights_lobby,
                                           loader_error_msg,
                                           loader_lua_error));

    // Make the KnightsClient
    knights_client.reset(new KnightsClient(false));  // Don't trust strings in an online platform game
    return knights_client;
}

boost::shared_ptr<KnightsClient>
    LobbyController::joinOnlineGame(OnlinePlatform &online_platform,
                                    const std::string &platform_lobby_id)
{
    resetAll();

    // Ensure we have up-to-date module catalogue (joinGameAccepted will need this,
    // as will the isCompatible check on completion of the platform lobby join)
    module_manager->update();  // Will block for short time, probably acceptable

    // Join the platform lobby
    platform_lobby = online_platform.joinLobby(platform_lobby_id);
    created_by_me = false;

    if (!platform_lobby) {
        throw ExceptionBase(LocalKey("failed_join_lobby"));
    }

    // Create the client
    knights_client.reset(new KnightsClient(false));  // Don't trust arbitrary strings coming from VM
    return knights_client;
}
#endif

void LobbyController::update()
{
    if (loader_thread.joinable()) {
        boost::unique_lock lock(loader_mutex);
        if (!loader_done) return;
        loader_thread.join();

        // If loader returned an error, signal it back to the main app code
        if (loader_error_msg.key != LocalKey()) {
            throw ExceptionBase(loader_error_msg);
        }
        if (loader_lua_error.get()) {
            throw *loader_lua_error;
        }
    }
}

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
#endif

#if defined(ONLINE_PLATFORM) && defined(USE_VM_LOBBY)
void LobbyController::checkHostMigration(OnlinePlatform &online_platform,
                                         Coercri::Timer &timer,
                                         bool new_control_system,
                                         LocalKey &host_migration_key,
                                         bool &del_gfx_sounds)
{
    // If loading in progress, wait for it before doing anything else
    if (loader_thread.joinable()) {
        return;
    }

    // If we were kicked out of the platform lobby, go to ErrorScreen
    if (platform_lobby && platform_lobby->getState() == PlatformLobby::State::FAILED) {
        if (host_migration_state == HostMigrationState::NOT_IN_GAME) {
            if (created_by_me) {
                throw ExceptionBase(LocalKey("failed_create_lobby"));
            } else {
                throw ExceptionBase(LocalKey("failed_join_lobby"));
            }
        } else {
            throw ExceptionBase(LocalKey("lobby_connection_lost"));
        }
    }

    // Check if we have a new leader
    if (platform_lobby
    && platform_lobby->getState() == PlatformLobby::State::JOINED
    && !platform_lobby->getLeaderId().empty()
    && platform_lobby->getLeaderId() != vm_lobby_leader_id) {

        // Platform lobby has finished setting up, OR leader has changed

        if (host_migration_state == HostMigrationState::NOT_IN_GAME) {

            if (vm_knights_lobby == nullptr) {
                // Now that we have joined the lobby, we can check that game is compatible
                GameModuleSpec spec = platform_lobby->getModuleSpec();
                std::vector<std::string> missing_modules;
                if (!module_manager->isCompatible(spec, missing_modules)) {
                    throw ExceptionBase(LocalKey("incompatible_game"));
                }

                // We haven't booted our local VM yet - do so now.
                VFS vfs = module_manager->getVFS(spec.module_names);
                loader_thread = boost::thread(VMLoader(online_platform.getNetworkDriver(),
                                                       timer,
                                                       online_platform.getCurrentUserId(),
                                                       new_control_system,
                                                       spec.module_names,
                                                       vfs,
                                                       loader_mutex,
                                                       loader_done,
                                                       knights_lobby,
                                                       vm_knights_lobby,
                                                       loader_error_msg,
                                                       loader_lua_error));

                // Return, and come back when the loader thread is finished!
                return;
            }

            // The local VM has finished booting. Join the game now.
            knights_client->setPlayerIdAndControls(online_platform.getCurrentUserId(),
                                                   new_control_system);
            knights_client->joinGame("#VMGame");
            sendOutgoingMessages();
        }
        
        vm_lobby_leader_id = platform_lobby->getLeaderId();

        if (vm_lobby_leader_id == online_platform.getCurrentUserId()) {
            vm_knights_lobby->becomeLeader(0);  // Dummy port number
        } else {
            // Note: we currently assume all players in the VM game are part of the
            // same online platform, and that we are using a NetworkDriver provided
            // by that platform. The convention (for now) is that we use the
            // platform_user_id of the player we wish to connect to as the address.
            // Here, we want to connect to the current lobby leader, so we use
            // vm_lobby_leader_id.getPlatformUserId().
            vm_knights_lobby->becomeFollower(vm_lobby_leader_id.getPlatformUserId(), 0);
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

        return;
    }

    // Check if we have lost connection
    if (platform_lobby
    && host_migration_state == HostMigrationState::IN_GAME
    && !vm_knights_lobby->connected()) {
        // Lost connection, and the VMKnightsLobby is attempting to reconnect.
        // Here we just change the screen.
        host_migration_state = HostMigrationState::RECONNECTING;
        host_migration_key = LocalKey("connection_lost_reconnecting");
        return;
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
    if (!loader_thread.joinable() && knights_lobby) {
        knights_lobby->readIncomingMessages(*knights_client);
    }

#ifdef ONLINE_PLATFORM
    // Read chat from platform lobby, if applicable
    if (platform_lobby) {
        std::vector<ChatMessage> msgs = platform_lobby->receiveChatMessages();
        if (knights_client) {
            ClientCallbacks *cb = knights_client->getClientCallbacks();
            if (cb) {
                for (const auto &msg : msgs) {
                    cb->chat(msg.sender, msg.message);
                }
            }
        }
    }
#endif
}

void LobbyController::sendOutgoingMessages()
{
    if (!loader_thread.joinable() && knights_lobby) {
        knights_lobby->sendOutgoingMessages(*knights_client);
    }

#ifdef ONLINE_PLATFORM
    // Write chat to platform lobby, if applicable
    if (knights_client) {
        std::vector<UTF8String> msgs = knights_client->getPendingChatMessages();
        if (platform_lobby) {
            for (const auto &msg : msgs) {
                platform_lobby->sendChatMessage(msg);
            }
        }
    }
#endif
}

int LobbyController::getNumberOfPlayers() const
{
    if (!loader_thread.joinable() && knights_lobby) {
        return knights_lobby->getNumberOfPlayers();
    } else {
        return 0;
    }
}

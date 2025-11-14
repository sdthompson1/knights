/*
 * loading_screen.cpp
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

#include "config_map.hpp"
#include "error_screen.hpp"
#include "game_manager.hpp"
#include "knights_app.hpp"
#include "knights_client.hpp"
#include "knights_config.hpp"
#include "loading_screen.hpp"
#include "options.hpp"
#include "x_centre.hpp"

void LoadingScreen::Loader::operator()()
{
    try {
        // the loading of the config is slow, especially in a debug
        // build, so we do it in a separate thread. the loading of
        // everything else is faster, and requires calls into coercri
        // (which complicates thread safety) so we do it in the main
        // thread.
        
        knights_config.reset(new KnightsConfig(knights_config_filename, menu_strict));

    } catch (LuaError &err) {
        lua_error.reset(new LuaError(err));
    } catch (std::exception &e) {
        error_msg = e.what();
        if (error_msg.empty()) error_msg = " ";
    } catch (...) {
        error_msg = "Unknown Error";
    }
}

LoadingScreen::Loader::Loader(const std::string & config_filename, bool menu_strict_)
: knights_config_filename(config_filename), menu_strict(menu_strict_)
{ }

bool LoadingScreen::start(KnightsApp &ka, boost::shared_ptr<Coercri::Window>, gcn::Gui &)
{
    knights_app = &ka;
    loader.reset(new Loader(
        tutorial_mode ? "main_tutorial.lua" :
                        ka.getKnightsConfigFilename(), 
        menu_strict_mode));

    boost::thread new_thread(boost::ref(*loader));
    loader_thread.swap(new_thread);
    return false;
}

LoadingScreen::LoadingScreen(int port, const PlayerID &id, bool single_player, bool menu_strict,
                             bool tutorial, bool autostart)
    : knights_app(0), server_port(port), player_id(id), single_player_mode(single_player),
      menu_strict_mode(menu_strict),
      tutorial_mode(tutorial),
      autostart_mode(autostart)
{ }

LoadingScreen::~LoadingScreen()
{
    // The loading screen might potentially be destroyed while the loader thread is
    // still running (e.g. if someone accepts an invite while they are on the loading screen).
    // In that case, we must wait for the loader thread to finish (as there is no way
    // to interrupt it).
    if (loader_thread.joinable()) {
        loader_thread.join();
    }
}

void LoadingScreen::update()
{
    if (!knights_app || !loader) return;

    if (loader_thread.joinable()) {
        loader_thread.timed_join(boost::posix_time::milliseconds(10));
        return;
    }

    // LuaErrors should be re-thrown in this thread. Other errors should just go directly
    // to ErrorScreen.
    if (loader->lua_error.get()) {
        throw *loader->lua_error;
    }
    if (!loader->error_msg.empty()) {
        UTF8String msg = UTF8String::fromUTF8Safe("Loading Failed: " + loader->error_msg);
        std::unique_ptr<Screen> error_screen(new ErrorScreen(msg));
        knights_app->requestScreenChange(std::move(error_screen));
        return;
    }

    if (server_port < 0) {
        // Local Game

        // create server & game
        boost::shared_ptr<KnightsClient> client =
            knights_app->startLocalGame(loader->knights_config, "#SplitScreenGame");

        // set dummy player ID.
        PlayerID dummy_player_id;
        bool action_bar_controls;
        if (single_player_mode || tutorial_mode) {
            dummy_player_id = PlayerID("Player 1");
            action_bar_controls = knights_app->getOptions().new_control_system || tutorial_mode;
        } else {
            dummy_player_id = PlayerID("#SplitScreenPlayer");
            action_bar_controls = false;
        }

        // set up the local client
        knights_app->createGameManager(client, single_player_mode, tutorial_mode, autostart_mode,
                                       false,  // allow_lobby_screen
                                       false,  // can_invite
                                       dummy_player_id);
        client->setClientCallbacks(&knights_app->getGameManager());
        client->setPlayerIdAndControls(dummy_player_id, action_bar_controls);

        // Join the game in split screen mode. This will take us to MenuScreen automatically.
        if (single_player_mode || tutorial_mode) {
            knights_app->getGameManager().tryJoinGame("#SplitScreenGame");
            if (autostart_mode) {
                // go straight into the game
                client->setReady(true);
            }
        } else {
            knights_app->getGameManager().tryJoinGameSplitScreen("#SplitScreenGame");
        }

    } else {
        // Host LAN Game

        // create server & game
        boost::shared_ptr<KnightsClient> client =
            knights_app->hostLanGame(server_port, loader->knights_config, "#LanGame");

        // set up the local client
        knights_app->createGameManager(client,
                                       false,  // single_player
                                       false,  // tutorial
                                       false,  // autostart
                                       false,  // allow_lobby_screen
                                       false,  // can_invite
                                       player_id);
        client->setClientCallbacks(&knights_app->getGameManager());

        // Start responding to broadcasts
        knights_app->startBroadcastReplies(server_port);

        // Set our player ID.
        client->setPlayerIdAndControls(player_id, knights_app->getOptions().new_control_system);
        
        // Join the game -- this will take us to MenuScreen automatically.
        knights_app->getGameManager().tryJoinGame("#LanGame");
    }

    loader.reset();
}

void LoadingScreen::draw(uint64_t timestamp_us, Coercri::GfxContext &gc)
{
    if (!knights_app) return;
    XCentre(gc, *knights_app->getFont(), knights_app->getFont()->getTextHeight() + 15, UTF8String::fromUTF8("LOADING"));
}

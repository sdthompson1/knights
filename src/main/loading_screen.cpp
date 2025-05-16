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
#include "knights_server.hpp"
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

LoadingScreen::LoadingScreen(int port, const UTF8String &name, bool single_player, bool menu_strict,
                             bool tutorial, bool autostart)
    : knights_app(0), server_port(port), player_name(name), single_player_mode(single_player), 
      menu_strict_mode(menu_strict),
      tutorial_mode(tutorial),
      autostart_mode(autostart)
{ }

LoadingScreen::~LoadingScreen()
{
    // We shouldn't really be able to get here unless the loader has finished,
    // but just in case it is still running:
    loader_thread.join();
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
        std::unique_ptr<Screen> error_screen(new ErrorScreen("Loading Failed: " + loader->error_msg));
        knights_app->requestScreenChange(std::move(error_screen));
        return;
    }
    
    if (server_port < 0) {
        // create server & game
        KnightsServer *server = knights_app->createLocalServer();
        server->startNewGame(loader->knights_config, "#SplitScreenGame");

        // set dummy player name.
        UTF8String dummy_player_name;
        bool action_bar_controls;
        if (single_player_mode || tutorial_mode) {
            dummy_player_name = UTF8String::fromUTF8("Player 1");
            action_bar_controls = knights_app->getOptions().new_control_system || tutorial_mode;
        } else {
            dummy_player_name = UTF8String::fromUTF8("#SplitScreenPlayer");
            action_bar_controls = false;
        }

        // create a local client
        boost::shared_ptr<KnightsClient> client = knights_app->openLocalConnection();
        knights_app->createGameManager(client, single_player_mode, tutorial_mode, autostart_mode, dummy_player_name);
        client->setClientCallbacks(&knights_app->getGameManager());
        client->setPlayerNameAndControls(dummy_player_name, action_bar_controls);
        
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
        // create server & game
        KnightsServer *server = knights_app->createServer(server_port);
        server->startNewGame(loader->knights_config, "#LanGame");

        // create a local client
        boost::shared_ptr<KnightsClient> client = knights_app->openLocalConnection();
        knights_app->createGameManager(client, false, false, false, player_name);
        client->setClientCallbacks(&knights_app->getGameManager());

        // Start responding to broadcasts
        knights_app->startBroadcastReplies(server_port);

        // Set our player name.
        client->setPlayerNameAndControls(player_name, knights_app->getOptions().new_control_system);
        
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

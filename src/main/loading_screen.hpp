/*
 * loading_screen.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2013.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#ifndef LOADING_SCREEN_HPP
#define LOADING_SCREEN_HPP

#include "screen.hpp"

#include "boost/thread.hpp"
#include <string>

class KnightsConfig;
class KnightsServer;
class LuaError;

class LoadingScreen : public Screen {
public:
    // set port to -1 and player_name to "" for local game.
    explicit LoadingScreen(int port, const UTF8String & player_name, bool single_player, 
                           bool menu_strict,  // set for single player & split screen games, where there is a fixed no of players
                           bool tutorial,
                           bool autostart);
    virtual ~LoadingScreen();
    virtual bool start(KnightsApp &knights_app, boost::shared_ptr<Coercri::Window>, gcn::Gui &);
    virtual unsigned int getUpdateInterval() { return 50; }
    virtual void update();
    virtual void draw(Coercri::GfxContext &gc);

private:
    struct Loader {
        explicit Loader(const std::string & config_filename, bool menu_strict_);
        void operator()();
        
        std::string error_msg;
        std::unique_ptr<LuaError> lua_error;

        std::string knights_config_filename;
        bool menu_strict;
        boost::shared_ptr<KnightsConfig> knights_config;
    };

    KnightsApp *knights_app;
    int server_port;
    UTF8String player_name;
    bool single_player_mode;
    bool menu_strict_mode;
    bool tutorial_mode;
    bool autostart_mode;
    boost::shared_ptr<Loader> loader;
    boost::thread loader_thread;
};

#endif

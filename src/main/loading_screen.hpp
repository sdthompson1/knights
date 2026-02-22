/*
 * loading_screen.hpp
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

#ifndef LOADING_SCREEN_HPP
#define LOADING_SCREEN_HPP

#include "localization.hpp"
#include "my_exceptions.hpp"
#include "player_id.hpp"
#include "screen.hpp"
#include "vfs.hpp"

#include "boost/thread.hpp"
#include <string>

class KnightsConfig;
class KnightsServer;

class LoadingScreen : public Screen {
public:
    // Set port to -1 and player_id to "" for local game.
    LoadingScreen(int port, const PlayerID & player_id, bool single_player,
                  bool menu_strict,  // set for single player & split screen games, where there is a fixed no of players
                  bool tutorial,
                  bool autostart);
    virtual ~LoadingScreen();
    virtual bool start(KnightsApp &knights_app, boost::shared_ptr<Coercri::Window>, gcn::Gui &) override;
    virtual void update() override;
    virtual void draw(uint64_t, Coercri::GfxContext &gc) override;

private:
    struct Loader {
        Loader(const std::vector<std::string> & module_names,
               const VFS &module_vfs,
               bool menu_strict);
        void operator()();

        LocalMsg error_msg;
        std::unique_ptr<LuaError> lua_error;

        std::vector<std::string> module_names;
        VFS module_vfs;
        bool menu_strict;
        boost::shared_ptr<KnightsConfig> knights_config;
    };

    KnightsApp *knights_app;
    int server_port;
    PlayerID player_id;
    bool single_player_mode;
    bool menu_strict_mode;
    bool tutorial_mode;
    bool autostart_mode;
    boost::shared_ptr<Loader> loader;
    boost::thread loader_thread;
};

#endif

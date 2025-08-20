/*
 * vm_loading_screen.hpp
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

#ifndef VM_LOADING_SCREEN_HPP
#define VM_LOADING_SCREEN_HPP

#if defined(USE_VM_LOBBY) && defined(ONLINE_PLATFORM)

#include "screen.hpp"
#include "online_platform.hpp"
#include "boost/thread.hpp"
#include <string>

class VMKnightsLobby;

namespace Coercri {
    class Timer;
}

class VMLoadingScreen : public Screen {
public:
    VMLoadingScreen(const std::string &lobby_id, OnlinePlatform::Visibility vis);
    ~VMLoadingScreen();

    virtual bool start(KnightsApp &ka, boost::shared_ptr<Coercri::Window>, gcn::Gui&) override;
    virtual void update() override;
    virtual void draw(uint64_t, Coercri::GfxContext &gc) override;

private:
    struct Loader {
        Loader(Coercri::NetworkDriver &net_driver,
               Coercri::Timer &timer,
               const PlayerID &local_user_id,
               bool new_control_system);
        void operator()();

        // Inputs
        Coercri::NetworkDriver &net_driver;
        Coercri::Timer &timer;
        PlayerID local_user_id;
        bool new_control_system;

        // Outputs
        std::string error_msg;
        std::unique_ptr<VMKnightsLobby> vm_knights_lobby;
    };

    KnightsApp *knights_app;
    std::string lobby_id;
    OnlinePlatform::Visibility visibility;

    std::unique_ptr<Loader> loader;
    boost::thread loader_thread;
};

#endif  // USE_VM_LOBBY and ONLINE_PLATFORM

#endif  // VM_LOADING_SCREEN_HPP

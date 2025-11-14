/*
 * vm_loading_screen.cpp
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

#if defined(USE_VM_LOBBY) && defined(ONLINE_PLATFORM)

#include "error_screen.hpp"
#include "game_manager.hpp"
#include "knights_app.hpp"
#include "knights_client.hpp"
#include "options.hpp"
#include "vm_knights_lobby.hpp"
#include "vm_loading_screen.hpp"
#include "x_centre.hpp"

VMLoadingScreen::Loader::Loader(Coercri::NetworkDriver &net_driver,
                                Coercri::Timer &timer,
                                const PlayerID &local_user_id,
                                bool new_control_system)
    : net_driver(net_driver), timer(timer), local_user_id(local_user_id),
      new_control_system(new_control_system)
{}

void VMLoadingScreen::Loader::operator()()
{
    // Creating a VMKnightsLobby is time-consuming, so we do it in a separate thread
    try {
        vm_knights_lobby = std::make_unique<VMKnightsLobby>(net_driver, timer, local_user_id, new_control_system);

    } catch (std::exception &e) {
        error_msg = e.what();
        if (error_msg.empty()) error_msg = " ";

    } catch (...) {
        error_msg = "Unknown Error";
    }
}

VMLoadingScreen::VMLoadingScreen(const std::string &lobby_id, OnlinePlatform::Visibility vis)
    : knights_app(nullptr),
      lobby_id(lobby_id),
      visibility(vis)
{ }

VMLoadingScreen::~VMLoadingScreen()
{ }

bool VMLoadingScreen::start(KnightsApp &ka, boost::shared_ptr<Coercri::Window>, gcn::Gui&)
{
    knights_app = &ka;
    PlayerID local_user_id = ka.getOnlinePlatform().getCurrentUserId();
    bool new_control_system = ka.getOptions().new_control_system;
    loader = std::make_unique<Loader>(ka.getNetworkDriver(), ka.getTimer(), local_user_id, new_control_system);
    loader_thread = boost::thread(boost::ref(*loader));
    return false;  // no gui required
}

void VMLoadingScreen::update()
{
    if (!knights_app || !loader) return;

    if (loader_thread.joinable()) {
        loader_thread.timed_join(boost::posix_time::milliseconds(10));
        return;
    }

    if (!loader->error_msg.empty()) {
        UTF8String msg = UTF8String::fromUTF8Safe("Loading failed: " + loader->error_msg);
        std::unique_ptr<Screen> error_screen(new ErrorScreen(msg));
        knights_app->requestScreenChange(std::move(error_screen));
        return;
    }

    // Create the game

    boost::shared_ptr<KnightsClient> client =
        knights_app->createVMGame(lobby_id, visibility, std::move(loader->vm_knights_lobby));

    PlayerID player_id = knights_app->getOnlinePlatform().getCurrentUserId();

    knights_app->createGameManager(client,
                                   false,  // single_player
                                   false,  // tutorial
                                   false,  // autostart
                                   false,  // allow_lobby_screen
                                   true,   // can_invite (We assume VM games are using an online platform, and hence can invite)
                                   player_id);
    client->setClientCallbacks(&knights_app->getGameManager());

    // Note: We don't send the "request join game" message initially; this happens later,
    // when the platform lobby becomes JOINED. See LobbyController::checkHostMigration.

    // Reset 'loader' variable so that VMLoadingScreen::update no
    // longer does anything. We just wait for KnightsApp to move us to
    // another screen at this point.
    loader.reset();
}

void VMLoadingScreen::draw(uint64_t, Coercri::GfxContext &gc)
{
    if (!knights_app) return;
    XCentre(gc, *knights_app->getFont(), knights_app->getFont()->getTextHeight() + 15, UTF8String::fromUTF8("LOADING"));
}

#endif  // USE_VM_LOBBY and ONLINE_PLATFORM

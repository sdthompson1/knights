/*
 * connecting_screen.cpp
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

#include "connecting_screen.hpp"
#include "game_manager.hpp"
#include "knights_app.hpp"
#include "knights_client.hpp"
#include "options.hpp"
#include "player_id.hpp"
#include "start_game_screen.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"

#include "boost/scoped_ptr.hpp"

#include <sstream>

using std::unique_ptr;
using std::string;

class ConnectingScreenImpl : public gcn::ActionListener {
public:
    ConnectingScreenImpl(const std::string &addr,
                         int port,
                         bool join_lan,
                         const PlayerID &player_id);
    void setupGui(KnightsApp &ka, gcn::Gui &gui);
    void setupConnection();
    void action(const gcn::ActionEvent &event);

private:
    std::string address;
    int port;
    bool join_lan_game;
    PlayerID player_id;
    
    bool setup_done;

    KnightsApp *knights_app;
    
    boost::scoped_ptr<GuiCentre> centre;
    boost::scoped_ptr<GuiPanel> panel;
    boost::scoped_ptr<gcn::Container> container;
    boost::scoped_ptr<gcn::Label> label;
    boost::scoped_ptr<gcn::Button> cancel_button;
};

ConnectingScreenImpl::ConnectingScreenImpl(const std::string &addr,
                                           int port,
                                           bool join_lan,
                                           const PlayerID &id)
    : address(addr),
      port(port),
      join_lan_game(join_lan),
      player_id(id),
      setup_done(false),
      knights_app(nullptr)
{ }

void ConnectingScreenImpl::setupGui(KnightsApp &ka, gcn::Gui &gui)
{
    knights_app = &ka;

    container.reset(new gcn::Container);
    container->setOpaque(false);
    
    const int pad = 10;
    int y = pad;

    label.reset(new gcn::Label("Connecting to " + address + "..."));
    container->add(label.get(), pad, y);
    y += label->getHeight() + pad;
    
    cancel_button.reset(new GuiButton("Cancel"));
    cancel_button->addActionListener(this);
    container->add(cancel_button.get(), pad + label->getWidth() / 2 - cancel_button->getWidth() / 2, y);

    container->setSize(2*pad + label->getWidth(), y + cancel_button->getHeight() + pad);
    
    panel.reset(new GuiPanel(container.get()));
    centre.reset(new GuiCentre(panel.get()));
    gui.setTop(centre.get());
}

void ConnectingScreenImpl::setupConnection()
{
    if (setup_done) return;
    setup_done = true;

    boost::shared_ptr<KnightsClient> client;
    client = knights_app->joinRemoteServer(address, port);
    knights_app->createGameManager(client, false, false, false, player_id);
    client->setClientCallbacks(&knights_app->getGameManager());
    client->setPlayerIdAndControls(player_id, knights_app->getOptions().new_control_system);

    std::ostringstream str;
    if (address.find(':') != std::string::npos) {
        str << "[" << address << "]:" << port;
    } else {
        str << address << ":" << port;
    }
    knights_app->getGameManager().setServerName(str.str());

    if (join_lan_game) {
        knights_app->getGameManager().tryJoinGame("#LanGame");
    }
}

void ConnectingScreenImpl::action(const gcn::ActionEvent &event)
{
    if (event.getSource() == cancel_button.get()) {
        // Go back to 'start game' screen
        unique_ptr<Screen> start_screen(new StartGameScreen);
        knights_app->requestScreenChange(std::move(start_screen));
    }
}

ConnectingScreen::ConnectingScreen(const string &addr, int port, bool join_lan, const PlayerID &id)
    : pimpl(new ConnectingScreenImpl(addr, port, join_lan, id))
{
}

bool ConnectingScreen::start(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui)
{
    pimpl->setupGui(ka, gui);
    return true;
}

void ConnectingScreen::update()
{
    pimpl->setupConnection();
}

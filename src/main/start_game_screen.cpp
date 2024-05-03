/*
 * start_game_screen.cpp
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

#include "misc.hpp"

#include "find_server_screen.hpp"
#include "host_lan_screen.hpp"
#include "knights_app.hpp"
#include "loading_screen.hpp"
#include "start_game_screen.hpp"
#include "title_screen.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"

#include "guichan.hpp"

#include "boost/scoped_ptr.hpp"
using namespace boost;

class StartGameScreenImpl : public gcn::ActionListener {
public:
    StartGameScreenImpl(KnightsApp &app, gcn::Gui &gui);
    void action(const gcn::ActionEvent &event);
    
private:
    KnightsApp &knights_app;
    scoped_ptr<GuiCentre> centre;
    scoped_ptr<gcn::Container> outer_container;
    scoped_ptr<GuiPanel> panel1, panel2;
    scoped_ptr<gcn::Container> container;
    scoped_ptr<gcn::Label> title;
    scoped_ptr<gcn::Button> join_internet_game;    
    scoped_ptr<gcn::Button> host_lan_game;
    scoped_ptr<gcn::Button> join_lan_game;
    scoped_ptr<gcn::Button> split_screen_mode;
    scoped_ptr<gcn::Button> single_player_mode;
    scoped_ptr<gcn::Button> exit;
};

StartGameScreenImpl::StartGameScreenImpl(KnightsApp &app, gcn::Gui &gui)
    : knights_app(app)
{
    const int w = 300, h = 40, vspace = 15, pad = 15, vspace2 = 40;
    
    container.reset(new gcn::Container);
    container->setOpaque(false);
    
    const int x = pad;
    int y = pad;
    const int yinc = h + vspace;
    
    join_internet_game.reset(new GuiButton("Connect to Server"));
    join_internet_game->setSize(w,h);
    join_internet_game->addActionListener(this);
    container->add(join_internet_game.get(), x, y);
    y += yinc;

    host_lan_game.reset(new GuiButton("Host LAN Game"));
    host_lan_game->setSize(w,h);
    host_lan_game->addActionListener(this);
    container->add(host_lan_game.get(), x, y);
    y += yinc;
    
    join_lan_game.reset(new GuiButton("Join LAN Game"));
    join_lan_game->setSize(w,h);
    join_lan_game->addActionListener(this);
    container->add(join_lan_game.get(), x, y);
    y += yinc;

    split_screen_mode.reset(new GuiButton("Split Screen Mode"));
    split_screen_mode->setSize(w,h);
    split_screen_mode->addActionListener(this);
    container->add(split_screen_mode.get(), x, y);
    y += yinc;

    single_player_mode.reset(new GuiButton("Single Player Mode"));
    single_player_mode->setSize(w,h);
    single_player_mode->addActionListener(this);
    container->add(single_player_mode.get(), x, y);
    y += yinc;
    
    exit.reset(new GuiButton("Cancel"));
    exit->setSize(w,h);
    exit->addActionListener(this);
    container->add(exit.get(), x, y);
    y += yinc;
    
    container->setSize(w + 2*pad, y - vspace + pad);
    panel2.reset(new GuiPanel(container.get()));

    title.reset(new gcn::Label("START GAME"));
    title->setWidth(w+2*pad);
    title->setHeight(title->getHeight() + 2*pad);
    title->setAlignment(gcn::Graphics::CENTER);
    panel1.reset(new GuiPanel(title.get()));

    outer_container.reset(new gcn::Container);
    outer_container->setOpaque(false);
    outer_container->add(panel1.get(), 0, 0);
    outer_container->add(panel2.get(), 0, panel1->getHeight() + vspace2);
    outer_container->setSize(panel2->getWidth(), panel1->getHeight() + vspace2 + panel2->getHeight());

    centre.reset(new GuiCentre(outer_container.get()));
    gui.setTop(centre.get());
}

void StartGameScreenImpl::action(const gcn::ActionEvent &event)
{
    std::unique_ptr<Screen> new_screen;

    if (event.getSource() == split_screen_mode.get()) {
        // Go to LoadingScreen in split-screen mode
        new_screen.reset(new LoadingScreen(-1, UTF8String(), false, true, false, false));
        
    } else if (event.getSource() == host_lan_game.get()) {
        // Go to HostLanScreen
        new_screen.reset(new HostLanScreen);

    } else if (event.getSource() == join_lan_game.get()) {
        // Go to FindServerScreen in LAN mode.
        new_screen.reset(new FindServerScreen("Join LAN Game", false));

    } else if (event.getSource() == join_internet_game.get()) {
        // Go to FindServerScreen in Internet mode.
        new_screen.reset(new FindServerScreen("Connect to Server", true));

    } else if (event.getSource() == single_player_mode.get()) {
        // Go to LoadingScreen in single player mode
        new_screen.reset(new LoadingScreen(-1, UTF8String(), true, true, false, false));
        
    } else if (event.getSource() == exit.get()) {
        // Go back to title screen
        new_screen.reset(new TitleScreen);
    }
    
    if (new_screen.get()) {
        knights_app.requestScreenChange(std::move(new_screen));
    }
}        

bool StartGameScreen::start(KnightsApp &knights_app, shared_ptr<Coercri::Window> w, gcn::Gui &gui)
{
    knights_app.resetAll();
    pimpl.reset(new StartGameScreenImpl(knights_app, gui));
    return true;
}

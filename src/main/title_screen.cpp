/*
 * title_screen.cpp
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

#include "credits_screen.hpp"
#include "knights_app.hpp"
#include "loading_screen.hpp"
#include "options_screen.hpp"
#include "start_game_screen.hpp"
#include "title_screen.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"

#include "guichan.hpp"

#include "boost/scoped_ptr.hpp"
using namespace boost;

class TitleScreenImpl : public gcn::ActionListener {
public:
    TitleScreenImpl(KnightsApp &app, gcn::Gui &gui);
    void action(const gcn::ActionEvent &event);
    
private:
    KnightsApp &knights_app;
    scoped_ptr<GuiCentre> centre;
    scoped_ptr<gcn::Container> outer_container;
    scoped_ptr<GuiPanel> panel1, panel2;
    scoped_ptr<gcn::Container> container;
    scoped_ptr<gcn::Label> title;
    scoped_ptr<gcn::Button> start_game;
    scoped_ptr<gcn::Button> tutorial;
    scoped_ptr<gcn::Button> options;
    scoped_ptr<gcn::Button> credits;
    scoped_ptr<gcn::Button> exit;
};

TitleScreenImpl::TitleScreenImpl(KnightsApp &app, gcn::Gui &gui)
    : knights_app(app)
{
    const int w = 280, h = 40, vspace = 15, pad = 15, vspace2 = 40;
    
    container.reset(new gcn::Container);
    container->setOpaque(false);
    
    const int x = pad;
    int y = pad;
    const int yinc = h + vspace;
    
    start_game.reset(new GuiButton("Start Game"));
    start_game->setSize(w,h);
    start_game->addActionListener(this);
    container->add(start_game.get(), x, y);
    y += yinc;
    
    tutorial.reset(new GuiButton("Tutorial"));
    tutorial->setSize(w,h);
    tutorial->addActionListener(this);
    container->add(tutorial.get(), x, y);
    y += yinc;

    options.reset(new GuiButton("Options"));
    options->setSize(w, h);
    options->addActionListener(this);
    container->add(options.get(), x, y);
    y += yinc;

    credits.reset(new GuiButton("Credits"));
    credits->setSize(w, h);
    credits->addActionListener(this);
    container->add(credits.get(), x, y);
    y += yinc;
    
    exit.reset(new GuiButton("Quit"));
    exit->setSize(w,h);
    exit->addActionListener(this);
    container->add(exit.get(), x, y);
    y += yinc;
    
    container->setSize(w + 2*pad, y - vspace + pad);
    panel2.reset(new GuiPanel(container.get()));

    title.reset(new gcn::Label("KNIGHTS"));
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

void TitleScreenImpl::action(const gcn::ActionEvent &event)
{
    std::unique_ptr<Screen> new_screen;

    if (event.getSource() == start_game.get()) {
        // Go to Start Game sub-menu
        new_screen.reset(new StartGameScreen);
        
    } else if (event.getSource() == tutorial.get()) {
        // Go to LoadingScreen in tutorial mode
        new_screen.reset(new LoadingScreen(-1, PlayerID(), true, true, true, true));

    } else if (event.getSource() == options.get()) {
        new_screen.reset(new OptionsScreen);
    } else if (event.getSource() == credits.get()) {
        new_screen.reset(new CreditsScreen("client/credits.txt"));
    } else if (event.getSource() == exit.get()) {
        knights_app.requestQuit();
    }
    
    if (new_screen.get()) {
        knights_app.requestScreenChange(std::move(new_screen));
    }
}        

bool TitleScreen::start(KnightsApp &knights_app, shared_ptr<Coercri::Window> w, gcn::Gui &gui)
{
    knights_app.resetAll();
    pimpl.reset(new TitleScreenImpl(knights_app, gui));
    return true;
}

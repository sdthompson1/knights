/*
 * host_lan_screen.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2012.
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

#include "config_map.hpp"
#include "host_lan_screen.hpp"
#include "knights_app.hpp"
#include "loading_screen.hpp"
#include "start_game_screen.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"

#include "gcn/cg_font.hpp" // coercri

#include "boost/scoped_ptr.hpp"

class HostLanScreenImpl : public gcn::ActionListener {
public:
    HostLanScreenImpl(KnightsApp &ka, gcn::Gui &gui);
    void action(const gcn::ActionEvent &event);
private:
    KnightsApp &knights_app;
    
    boost::scoped_ptr<GuiCentre> centre;
    boost::scoped_ptr<GuiPanel> panel;
    boost::scoped_ptr<gcn::Container> container;
    boost::scoped_ptr<gcn::Label> name_label;
    boost::scoped_ptr<gcn::Label> label1;    
    boost::scoped_ptr<gcn::TextField> name_field;
    boost::scoped_ptr<gcn::Button> ok_button;
    boost::scoped_ptr<gcn::Button> cancel_button;
};

HostLanScreenImpl::HostLanScreenImpl(KnightsApp &ka, gcn::Gui &gui)
    : knights_app(ka)
{
    container.reset(new gcn::Container);
    container->setOpaque(false);

    const int pad = 10, ypad = 20, small_pad = 5;
    int y = small_pad;

    label1.reset(new gcn::Label("Host LAN Game"));
    label1->setForegroundColor(gcn::Color(0,0,128));

    name_label.reset(new gcn::Label("Player Name: "));
    name_field.reset(new gcn::TextField);
    name_field->adjustSize();
    name_field->setWidth(250);
    name_field->setText(knights_app.getPlayerName());
    name_field->addActionListener(this);
    
    const int total_width = 2*pad + name_label->getWidth() + name_field->getWidth();
    
    container->add(label1.get(), total_width/2 - label1->getWidth()/2, y);
    y += label1->getHeight() + ypad;

    container->add(name_label.get(), pad, y);
    container->add(name_field.get(), pad + name_label->getWidth(), y);
    y += name_field->getHeight() + ypad;

    ok_button.reset(new GuiButton("Start"));
    ok_button->addActionListener(this);
    cancel_button.reset(new GuiButton("Cancel"));
    cancel_button->addActionListener(this);
    container->add(ok_button.get(), pad, y);
    container->add(cancel_button.get(), total_width - pad - cancel_button->getWidth(), y);
    y += ok_button->getHeight() + small_pad;

    container->setSize(total_width, y);
    panel.reset(new GuiPanel(container.get()));
    centre.reset(new GuiCentre(panel.get()));
    gui.setTop(centre.get());

    // make sure the text field is focused initially
    name_field->requestFocus();
}

void HostLanScreenImpl::action(const gcn::ActionEvent &event)
{
    if (event.getSource() == cancel_button.get()) {
        // Go back to start game screen
        std::auto_ptr<Screen> start_screen(new StartGameScreen);
        knights_app.requestScreenChange(start_screen);
        
    } else if (event.getSource() == name_field.get() || event.getSource() == ok_button.get()) {
        std::string name = "Player 1";
        if (!name_field->getText().empty()) {
            name = name_field->getText();
            knights_app.setPlayerName(name);  // make sure the name gets saved when we exit
        }

        // Go to LoadingScreen in LAN mode
        const int server_port = knights_app.getConfigMap().getInt("port_number");
        std::auto_ptr<Screen> loading_screen(new LoadingScreen(server_port, name, false, false, false));
        knights_app.requestScreenChange(loading_screen);
    }
}

bool HostLanScreen::start(KnightsApp &knights_app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui)
{
    pimpl.reset(new HostLanScreenImpl(knights_app, gui));
    return true;
}

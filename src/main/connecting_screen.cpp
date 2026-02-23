/*
 * connecting_screen.cpp
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

#include "misc.hpp"

#include "connecting_screen.hpp"
#include "knights_app.hpp"
#include "start_game_screen.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"

#include <sstream>

class ConnectingScreenImpl : public gcn::ActionListener {
public:
    ConnectingScreenImpl(const std::string &addr,
                         const std::string &addr_display_name);
    void setupGui(KnightsApp &ka, gcn::Gui &gui);
    void action(const gcn::ActionEvent &event);

private:
    std::string address;
    std::string addr_display_name;

    KnightsApp *knights_app;
    
    std::unique_ptr<GuiCentre> centre;
    std::unique_ptr<GuiPanel> panel;
    std::unique_ptr<gcn::Container> container;
    std::unique_ptr<gcn::Label> label;
    std::unique_ptr<gcn::Button> cancel_button;
};

ConnectingScreenImpl::ConnectingScreenImpl(const std::string &addr,
                                           const std::string &addr_display_name)
    : address(addr),
      addr_display_name(addr_display_name),
      knights_app(nullptr)
{ }

void ConnectingScreenImpl::setupGui(KnightsApp &ka, gcn::Gui &gui)
{
    knights_app = &ka;

    container.reset(new gcn::Container);
    container->setOpaque(false);
    
    const int pad = 10;
    int y = pad;

    std::vector<LocalParam> params;
    params.push_back(LocalParam(UTF8String::fromUTF8Safe(address)));
    params.push_back(LocalParam(UTF8String::fromUTF8Safe(addr_display_name)));
    LocalKey key(addr_display_name.empty() ? "connecting_to" : "connecting_to_alt");
    UTF8String text = ka.getLocalization().get(key, params);
    label.reset(new gcn::Label(text.asUTF8()));
    container->add(label.get(), pad, y);
    y += label->getHeight() + pad;
    
    cancel_button.reset(new GuiButton(ka.getLocalization().get(LocalKey("cancel")).asUTF8()));
    cancel_button->addActionListener(this);
    container->add(cancel_button.get(), pad + label->getWidth() / 2 - cancel_button->getWidth() / 2, y);

    container->setSize(2*pad + label->getWidth(), y + cancel_button->getHeight() + pad);
    
    panel.reset(new GuiPanel(container.get()));
    centre.reset(new GuiCentre(panel.get()));
    gui.setTop(centre.get());
}

void ConnectingScreenImpl::action(const gcn::ActionEvent &event)
{
    if (event.getSource() == cancel_button.get()) {
        // Go back to 'start game' screen
        knights_app->requestScreenChange(std::make_unique<StartGameScreen>());
    }
}

ConnectingScreen::ConnectingScreen(const std::string &addr, const std::string &addr_display_name)
    : pimpl(new ConnectingScreenImpl(addr, addr_display_name))
{ }

ConnectingScreen::~ConnectingScreen()
{ }

bool ConnectingScreen::start(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui)
{
    pimpl->setupGui(ka, gui);
    return true;
}

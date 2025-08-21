/*
 * host_migration_screen.cpp
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

#include "host_migration_screen.hpp"
#include "knights_app.hpp"
#include "title_screen.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"

#include "gcn/cg_font.hpp" // coercri

class HostMigrationScreenImpl : public gcn::ActionListener {
public:
    explicit HostMigrationScreenImpl(const LocalKey &msg) : msg(msg), knights_app(nullptr) { }
    void createGui(KnightsApp &ka, gcn::Gui &gui);
    void action(const gcn::ActionEvent &event);
private:
    LocalKey msg;
    KnightsApp *knights_app;
    std::unique_ptr<GuiCentre> centre;
    std::unique_ptr<GuiPanel> panel;
    std::unique_ptr<gcn::Container> container;
    std::unique_ptr<gcn::Label> label1;
    std::unique_ptr<gcn::Button> cancel_button;
};

void HostMigrationScreenImpl::createGui(KnightsApp &ka, gcn::Gui &gui)
{
    knights_app = &ka;

    container.reset(new gcn::Container);
    container->setOpaque(false);

    const int pad = 10, small_pad = 5;
    int y = small_pad;

    std::string msg_latin1 = ka.getLocalization().get(msg).asLatin1();
    label1.reset(new gcn::Label(msg_latin1));

    const int total_width = 2*pad + label1->getWidth();

    container->add(label1.get(), total_width/2 - label1->getWidth()/2, y);
    y += label1->getHeight() + small_pad;

    cancel_button.reset(new GuiButton("Cancel"));
    cancel_button->addActionListener(this);
    container->add(cancel_button.get(), total_width/2 - cancel_button->getWidth()/2, y);
    y += cancel_button->getHeight() + small_pad;

    container->setSize(total_width, y);
    panel.reset(new GuiPanel(container.get()));
    centre.reset(new GuiCentre(panel.get()));
    gui.setTop(centre.get());
}

void HostMigrationScreenImpl::action(const gcn::ActionEvent &event)
{
    if (event.getSource() == cancel_button.get()) {
        // Go back to title screen
        std::unique_ptr<Screen> title_screen(new TitleScreen);
        knights_app->requestScreenChange(std::move(title_screen));
    }
}

HostMigrationScreen::HostMigrationScreen(const LocalKey &msg)
{
    pimpl.reset(new HostMigrationScreenImpl(msg));
}

bool HostMigrationScreen::start(KnightsApp &knights_app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui)
{
    pimpl->createGui(knights_app, gui);
    return true;
}

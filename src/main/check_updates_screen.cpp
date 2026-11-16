/*
 * check_updates_screen.cpp
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

#ifdef ONLINE_PLATFORM

#include "check_updates_screen.hpp"
#include "knights_app.hpp"
#include "localization.hpp"
#include "missing_mods_screen.hpp"
#include "mods_screen.hpp"
#include "online_platform.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"
#include "gui_text_wrap.hpp"

#include "guichan.hpp"

#include <memory>
#include <string>
#include <vector>

namespace {
    // How often to poll the online platform for query results
    const unsigned int POLL_INTERVAL_MS = 200;

    const int PAD = 10;
    const int WIDTH = 400;
}

class CheckUpdatesScreenImpl : public gcn::ActionListener {
public:
    CheckUpdatesScreenImpl(KnightsApp &app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui);
    void action(const gcn::ActionEvent &event) override;
    void update();

private:
    void showMessage(const char *msg_key, const char *button_key);
    void pollResults();

    KnightsApp &knights_app;
    boost::shared_ptr<Coercri::Window> window;
    gcn::Gui &gui;

    std::vector<std::string> mod_names;   // Installed Workshop mods (vfs names)
    bool checking;                        // true while waiting for query results
    unsigned int last_poll_msec;

    std::unique_ptr<GuiCentre> centre;
    std::unique_ptr<GuiPanel> panel;
    std::unique_ptr<gcn::Container> container;
    std::unique_ptr<gcn::Label> title_label;
    std::unique_ptr<GuiTextWrap> message;
    std::unique_ptr<gcn::Button> button;
};

CheckUpdatesScreenImpl::CheckUpdatesScreenImpl(KnightsApp &app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g)
    : knights_app(app), window(win), gui(g), checking(false), last_poll_msec(0)
{
    const Localization &loc = knights_app.getLocalization();

    title_label.reset(new gcn::Label(loc.get(LocalKey("check_updates_title")).asUTF8()));
    title_label->setForegroundColor(gcn::Color(0, 0, 128));

    message.reset(new GuiTextWrap);
    message->setForegroundColor(gcn::Color(30, 30, 30));
    message->setWidth(WIDTH);
    //message->setCentred(true);

    OnlinePlatform &op = knights_app.getOnlinePlatform();
    for (const OnlinePlatform::InstalledMod &mod : op.getInstalledMods()) {
        mod_names.push_back(mod.vfs_name);
    }

    if (mod_names.empty()) {
        showMessage("check_updates_no_mods", "ok");
    } else {
        op.sendModQuery(mod_names);
        checking = true;
        showMessage("check_updates_checking", "cancel");
    }
}

// Set the message and button caption, and (re-)create the panel at the appropriate size.
// (This shouldn't be called from within action(), because it destroys gui objects.)
void CheckUpdatesScreenImpl::showMessage(const char *msg_key, const char *button_key)
{
    const Localization &loc = knights_app.getLocalization();

    gui.setTop(nullptr);
    centre.reset();
    panel.reset();
    container.reset(new gcn::Container);
    container->setOpaque(false);

    int y = PAD;
    container->add(title_label.get(), PAD + WIDTH/2 - title_label->getWidth()/2, y);
    y += title_label->getHeight() + 2*PAD;

    message->setText(loc.get(LocalKey(msg_key)));
    message->adjustHeight();
    container->add(message.get(), PAD, y);
    y += message->getHeight() + 2*PAD;

    button.reset(new GuiButton(loc.get(LocalKey(button_key)).asUTF8()));
    button->addActionListener(this);
    container->add(button.get(), PAD + WIDTH/2 - button->getWidth()/2, y);
    y += button->getHeight() + PAD;

    container->setSize(2*PAD + WIDTH, y);

    panel.reset(new GuiPanel(container.get()));
    centre.reset(new GuiCentre(panel.get()));

    int w, h;
    window->getSize(w, h);
    centre->setSize(w, h);
    gui.setTop(centre.get());
    gui.logic();
    window->invalidateAll();
}

void CheckUpdatesScreenImpl::action(const gcn::ActionEvent &event)
{
    // Cancel or OK: go back to the mods screen
    if (event.getSource() == button.get()) {
        checking = false;
        std::unique_ptr<Screen> new_screen(new ModsScreen);
        knights_app.requestScreenChange(std::move(new_screen));
    }
}

// Check whether all query results are in, and if so, show the result
// (or go to the MissingModsScreen if updates are required).
void CheckUpdatesScreenImpl::pollResults()
{
    OnlinePlatform &op = knights_app.getOnlinePlatform();
    std::vector<std::string> needs_update;
    bool any_success = false;

    for (const std::string &name : mod_names) {
        OnlinePlatform::ModDetails details;
        switch (op.getModQueryResult(name, details)) {
        case OnlinePlatform::MQR_WAITING:
            return;  // Still waiting; try again later
        case OnlinePlatform::MQR_SUCCESS:
            any_success = true;
            if (details.requires_update) needs_update.push_back(name);
            break;
        case OnlinePlatform::MQR_FAILED:
            // For now, failures are ignored (unless all queries failed)
            break;
        }
    }

    checking = false;

    if (!needs_update.empty()) {
        std::unique_ptr<Screen> new_screen(new MissingModsScreen(std::move(needs_update),
                                                                 MissingModsScreen::UPDATES));
        knights_app.requestScreenChange(std::move(new_screen));
    } else if (any_success) {
        showMessage("check_updates_up_to_date", "ok");
    } else {
        showMessage("check_updates_failed", "ok");
    }
}

void CheckUpdatesScreenImpl::update()
{
    if (!checking) return;

    const unsigned int now = knights_app.getTimer().getMsec();
    if (now - last_poll_msec < POLL_INTERVAL_MS) return;
    last_poll_msec = now;

    pollResults();
}


//
// CheckUpdatesScreen
//

bool CheckUpdatesScreen::start(KnightsApp &knights_app, boost::shared_ptr<Coercri::Window> w, gcn::Gui &gui)
{
    pimpl.reset(new CheckUpdatesScreenImpl(knights_app, w, gui));
    return true;
}

void CheckUpdatesScreen::update()
{
    pimpl->update();
}

#endif  // ONLINE_PLATFORM

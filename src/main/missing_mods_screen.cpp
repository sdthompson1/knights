/*
 * missing_mods_screen.cpp
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

#include "knights_app.hpp"
#include "localization.hpp"
#include "missing_mods_screen.hpp"
#include "online_platform.hpp"
#include "title_screen.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"
#include "gui_text_wrap.hpp"

#include "guichan.hpp"

#include <memory>
#include <string>
#include <vector>

namespace {
    // How often to poll the online platform for mod titles, and how long to wait
    // for the titles before giving up (and showing the vfs names instead)
    const unsigned int TITLE_POLL_INTERVAL_MS = 200;
    const unsigned int TITLE_TIMEOUT_MS = 5000;
}

class MissingModsScreenImpl : public gcn::ActionListener {
public:
    MissingModsScreenImpl(KnightsApp &app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui,
                          const std::vector<std::string> &mod_vfs_names);
    void action(const gcn::ActionEvent &event) override;
    void update();

private:
    bool pollTitles();
    void createGui();

    KnightsApp &knights_app;
    boost::shared_ptr<Coercri::Window> window;
    gcn::Gui &gui;

    std::vector<std::string> mod_vfs_names;
    std::vector<std::string> titles;   // empty string = title not known yet
    bool gui_created;
    unsigned int start_msec, last_poll_msec;

    std::unique_ptr<GuiCentre> centre;
    std::unique_ptr<GuiPanel> panel;
    std::unique_ptr<gcn::Container> container;
    std::unique_ptr<gcn::Label> title_label;
    std::unique_ptr<GuiTextWrap> message;
    std::unique_ptr<gcn::Button> retry_button;
    std::unique_ptr<gcn::Button> browse_button;
    std::unique_ptr<gcn::Button> exit_button;
};

MissingModsScreenImpl::MissingModsScreenImpl(KnightsApp &app, boost::shared_ptr<Coercri::Window> win,
                                             gcn::Gui &g, const std::vector<std::string> &names)
    : knights_app(app), window(win), gui(g),
      mod_vfs_names(names), titles(names.size()), gui_created(false),
      start_msec(app.getTimer().getMsec()), last_poll_msec(0)
{
    // Show a blank screen (guichan requires a top widget) until the titles come in
    container.reset(new gcn::Container);
    container->setOpaque(false);
    gui.setTop(container.get());

#ifdef ONLINE_PLATFORM
    knights_app.getOnlinePlatform().sendModQuery(mod_vfs_names);
#endif
}

// Check for title query results. Returns true if all titles are now known.
bool MissingModsScreenImpl::pollTitles()
{
    bool all_known = true;
    for (size_t i = 0; i < mod_vfs_names.size(); ++i) {
        if (!titles[i].empty()) continue;
#ifdef ONLINE_PLATFORM
        OnlinePlatform::ModDetails details;
        switch (knights_app.getOnlinePlatform().getModQueryResult(mod_vfs_names[i], details)) {
        case OnlinePlatform::MQR_WAITING:
            all_known = false;
            break;
        case OnlinePlatform::MQR_SUCCESS:
            titles[i] = details.title.asUTF8();
            break;
        case OnlinePlatform::MQR_FAILED:
            // Probably a local (non-Workshop) mod, so just use the vfs name
            titles[i] = mod_vfs_names[i];
            break;
        }
#else
        titles[i] = mod_vfs_names[i];
#endif
    }
    return all_known;
}

void MissingModsScreenImpl::createGui()
{
    const Localization &loc = knights_app.getLocalization();

    // Build the message: intro, list of mods (one per line), then instructions
    // depending on whether we have Workshop mods, local mods, or both
    bool any_workshop = false, any_local = false;
    std::string msg = loc.get(LocalKey("missing_mods_intro")).asUTF8() + "\n\n";
    for (size_t i = 0; i < mod_vfs_names.size(); ++i) {
        msg += "    " + (titles[i].empty() ? mod_vfs_names[i] : titles[i]) + "\n";
#ifdef ONLINE_PLATFORM
        if (knights_app.getOnlinePlatform().isWorkshopModName(mod_vfs_names[i])) {
            any_workshop = true;
            continue;
        }
#endif
        any_local = true;
    }
    if (any_local) msg += "\n" + loc.get(LocalKey("missing_mods_local")).asUTF8();
    else if (any_workshop) msg += "\n" + loc.get(LocalKey("missing_mods_workshop")).asUTF8();

    // Create widgets
    title_label.reset(new gcn::Label(loc.get(LocalKey("missing_mods_title")).asUTF8()));
    title_label->setForegroundColor(gcn::Color(0, 0, 128));

    message.reset(new GuiTextWrap);
    message->setForegroundColor(gcn::Color(30, 30, 30));

    retry_button.reset(new GuiButton(loc.get(LocalKey("retry")).asUTF8()));
    retry_button->addActionListener(this);
#ifdef ONLINE_PLATFORM
    browse_button.reset(new GuiButton(loc.get(LocalKey("browse_workshop")).asUTF8()));
    browse_button->addActionListener(this);
#endif
    exit_button.reset(new GuiButton(loc.get(LocalKey("exit")).asUTF8()));
    exit_button->addActionListener(this);

    // Lay out the container
    gui.setTop(nullptr);
    container.reset(new gcn::Container);
    container->setOpaque(false);

    const int pad = 10;
    const int width = 500;
    int y = pad;

    container->add(title_label.get(), pad + width/2 - title_label->getWidth()/2, y);
    y += title_label->getHeight() + 2*pad;

    message->setWidth(width);
    message->setText(UTF8String::fromUTF8Safe(msg));
    message->adjustHeight();
    container->add(message.get(), pad, y);
    y += message->getHeight() + 2*pad;

    container->add(retry_button.get(), pad, y);
    if (browse_button && any_workshop) {
        container->add(browse_button.get(), pad + width/2 - browse_button->getWidth()/2, y);
    }
    container->add(exit_button.get(), pad + width - exit_button->getWidth(), y);
    y += exit_button->getHeight() + pad;

    container->setSize(2*pad + width, y);

    panel.reset(new GuiPanel(container.get()));
    centre.reset(new GuiCentre(panel.get()));

    int w, h;
    window->getSize(w, h);
    centre->setSize(w, h);
    gui.setTop(centre.get());
    gui.logic();
    window->invalidateAll();

    gui_created = true;
}

void MissingModsScreenImpl::action(const gcn::ActionEvent &event)
{
    if (event.getSource() == retry_button.get()) {
        knights_app.retryLastJoin();
#ifdef ONLINE_PLATFORM
    } else if (event.getSource() == browse_button.get()) {
        knights_app.getOnlinePlatform().browseWorkshop();
#endif
    } else {
        std::unique_ptr<Screen> title_screen(new TitleScreen);
        knights_app.requestScreenChange(std::move(title_screen));
    }
}

void MissingModsScreenImpl::update()
{
    if (gui_created) return;

    const unsigned int now = knights_app.getTimer().getMsec();
    if (now - last_poll_msec < TITLE_POLL_INTERVAL_MS) return;
    last_poll_msec = now;

    if (pollTitles() || now - start_msec >= TITLE_TIMEOUT_MS) {
        createGui();
    }
}


//
// MissingModsScreen
//

bool MissingModsScreen::start(KnightsApp &knights_app, boost::shared_ptr<Coercri::Window> w, gcn::Gui &gui)
{
    // As for ErrorScreen: reset everything (this leaves the game we were trying to
    // join), and flash the window to get the user's attention.
    knights_app.resetAll();
    pimpl.reset(new MissingModsScreenImpl(knights_app, w, gui, mod_vfs_names));
    knights_app.flashWindow();
    return true;
}

void MissingModsScreen::update()
{
    pimpl->update();
}

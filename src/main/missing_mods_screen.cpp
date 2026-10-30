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

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"
#include "gui_text_wrap.hpp"

#include "guichan.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace {
    // How often to poll the online platform for mod titles and download progress,
    // and how long to wait for the titles before showing the GUI.
    const unsigned int POLL_INTERVAL_MS = 200;
    const unsigned int TITLE_TIMEOUT_MS = 5000;

    // Layout. The mod names are centred, with View buttons just to the right of
    // the widest name.
    const int PAD = 10;
    const int WIDTH = 500;
    const int VIEW_GAP = 15;
    const int ROW_GAP = 5;

#ifdef ONLINE_PLATFORM
    struct DownloadInfo {
        bool workshop = false;
        OnlinePlatform::ModDownloadStatus status = OnlinePlatform::MDS_NOT_SUBSCRIBED;
        int progress = 0;
        LocalMsg error;

        bool operator!=(const DownloadInfo &other) const {
            return status != other.status || progress != other.progress || error != other.error;
        }
    };

    // Returns true for Workshop mods that should have OnlinePlatform::subscribeMod
    // called on them (when the Subscribe button is clicked).
    bool CanSubscribe(const DownloadInfo &d)
    {
        return d.workshop && (d.status == OnlinePlatform::MDS_NOT_SUBSCRIBED
                              || d.status == OnlinePlatform::MDS_FAILED);
    }
#endif

    struct ModRow {
        std::string vfs_name;
        std::string title;  // initially vfs_name; becomes the actual Workshop title when known
        bool title_finalized = false;
        std::unique_ptr<gcn::Label> name_label;
        std::unique_ptr<GuiButton> view_button;   // only for Workshop mods
#ifdef ONLINE_PLATFORM
        DownloadInfo download;
#endif
    };

    // Shorten a string (adding "…") so that it fits within max_width pixels
    std::string TruncateToWidth(const gcn::Font &font, std::string str, int max_width)
    {
        if (font.getWidth(str) <= max_width) return str;
        const std::string ellipsis = "…";
        while (!str.empty() && font.getWidth(str + ellipsis) > max_width) {
            // Remove the last UTF-8 character (continuation bytes, then the lead byte)
            while (!str.empty() && (str.back() & 0xC0) == 0x80) str.pop_back();
            if (!str.empty()) str.pop_back();
        }
        return str + ellipsis;
    }

    std::unique_ptr<GuiTextWrap> MakeTextWrap(bool centred)
    {
        std::unique_ptr<GuiTextWrap> text(new GuiTextWrap);
        text->setForegroundColor(gcn::Color(30, 30, 30));
        text->setWidth(WIDTH);
        text->setCentred(centred);
        return text;
    }
}

class MissingModsScreenImpl : public gcn::ActionListener {
public:
    MissingModsScreenImpl(KnightsApp &app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui,
                          const std::vector<std::string> &mod_vfs_names);
    void action(const gcn::ActionEvent &event) override;
    void update();

private:
    bool pollTitles();
    bool allTitlesFinalized() const;
    UTF8String statusText() const;
    bool canSubscribe() const;
    bool updateStatus();
    void layout();
#ifdef ONLINE_PLATFORM
    bool pollDownloads();
    bool readyForAutoJoin() const;
    bool subscribeFailed() const;
#endif

    KnightsApp &knights_app;
    boost::shared_ptr<Coercri::Window> window;
    gcn::Gui &gui;

    std::vector<ModRow> rows;
    int view_space;           // width of a View button plus gap (or 0 if no View buttons)
#ifdef ONLINE_PLATFORM
    bool subscribe_clicked;   // if true, join the game automatically once all mods are installed
#endif
    bool gui_created;
    bool subscribe_shown;     // whether the Subscribe button is currently shown

    unsigned int start_msec, last_poll_msec;

    std::unique_ptr<GuiCentre> centre;
    std::unique_ptr<GuiPanel> panel;
    std::unique_ptr<gcn::Container> container;
    std::unique_ptr<gcn::Label> title_label;
    std::unique_ptr<GuiTextWrap> intro_text;
    std::unique_ptr<GuiTextWrap> instructions_text;
    std::unique_ptr<GuiTextWrap> status_text;
    std::unique_ptr<gcn::Button> subscribe_button;
    std::unique_ptr<gcn::Button> exit_button;
};

MissingModsScreenImpl::MissingModsScreenImpl(KnightsApp &app, boost::shared_ptr<Coercri::Window> win,
                                             gcn::Gui &g, const std::vector<std::string> &names)
    : knights_app(app), window(win), gui(g), rows(names.size()), view_space(0),
#ifdef ONLINE_PLATFORM
      subscribe_clicked(false),
#endif
      gui_created(false), subscribe_shown(false),
      start_msec(app.getTimer().getMsec()), last_poll_msec(0)
{
    const Localization &loc = knights_app.getLocalization();

    title_label.reset(new gcn::Label(loc.get(LocalKey("missing_mods_title")).asUTF8()));
    title_label->setForegroundColor(gcn::Color(0, 0, 128));

    intro_text = MakeTextWrap(false);
    intro_text->setText(loc.get(LocalKey("missing_mods_intro")));
    intro_text->adjustHeight();

    // Create the per-mod widgets, and find out whether we have Workshop mods,
    // local mods, or both
    bool any_workshop = false, any_local = false;
    for (size_t i = 0; i < names.size(); ++i) {
        ModRow &row = rows[i];
        row.vfs_name = names[i];
        row.title = names[i];  // Initial title, might be overridden later
        row.name_label.reset(new gcn::Label);
        row.name_label->setForegroundColor(gcn::Color(30, 30, 30));
#ifdef ONLINE_PLATFORM
        row.download.workshop = knights_app.getOnlinePlatform().isWorkshopModName(row.vfs_name);
        if (row.download.workshop) {
            row.view_button.reset(new GuiButton(loc.get(LocalKey("view")).asUTF8()));
            row.view_button->addActionListener(this);
            view_space = row.view_button->getWidth() + VIEW_GAP;
            any_workshop = true;
            continue;
        }

        // Apply filtering to the initial title (the vfs name).
        // (The thinking behind the filtering is that malicious hosts might create
        // local mod directories with offensive names, so those names shouldn't be
        // displayed to the player unless they have been through the online platform's
        // game content filter.)
        row.title = knights_app.getOnlinePlatform().filterGameContent(UTF8String::fromUTF8Safe(names[i])).asUTF8();
#endif
        any_local = true;
    }

    instructions_text = MakeTextWrap(false);
    Coercri::UTF8String instructions_string;
    if (any_workshop) {
        instructions_string = loc.get(LocalKey("missing_mods_workshop"));
    }
    if (any_local) {
        if (!instructions_string.empty()) {
            instructions_string += Coercri::UTF8String::fromUTF8("\n\n");
        }
        instructions_string += loc.get(LocalKey("missing_mods_local" PLATFORM_SUFFIX));
    }
    instructions_text->setText(instructions_string);
    instructions_text->adjustHeight();

    status_text = MakeTextWrap(true);

#ifdef ONLINE_PLATFORM
    subscribe_button.reset(new GuiButton(loc.get(LocalKey("subscribe")).asUTF8()));
    subscribe_button->addActionListener(this);
#endif
    exit_button.reset(new GuiButton(loc.get(LocalKey("exit")).asUTF8()));
    exit_button->addActionListener(this);

    // Show a blank screen (guichan requires a top widget) until the titles come in
    container.reset(new gcn::Container);
    container->setOpaque(false);
    gui.setTop(container.get());

#ifdef ONLINE_PLATFORM
    knights_app.getOnlinePlatform().sendModQuery(names);
#endif
}

// Check for title query results. Returns true if any title text was changed.
bool MissingModsScreenImpl::pollTitles()
{
    bool changed = false;
    for (ModRow &row : rows) {
        if (row.title_finalized) continue;
#ifdef ONLINE_PLATFORM
        OnlinePlatform::ModDetails details;
        switch (knights_app.getOnlinePlatform().getModQueryResult(row.vfs_name, details)) {
        case OnlinePlatform::MQR_WAITING:
            // Title isn't known yet
            break;
        case OnlinePlatform::MQR_SUCCESS:
            // We have the actual Workshop title; use it.
            // Note: Workshop titles are not filtered; we assume Steam is already policing
            // Workshop item titles for offensive content.
            row.title = details.title.asUTF8();
            row.title_finalized = true;
            changed = true;
            break;
        case OnlinePlatform::MQR_FAILED:
            // Probably a local (non-Workshop) mod, so continue using the vfs_name,
            // and count the title as finalized now.
            row.title_finalized = true;
            break;
        }
#else
        // No OnlinePlatform available to query, so continue using the vfs_name,
        // and count the title as finalized now.
        row.title_finalized = true;
#endif
    }
    return changed;
}

bool MissingModsScreenImpl::allTitlesFinalized() const
{
    for (const ModRow &row : rows) {
        if (!row.title_finalized) return false;
    }
    return true;
}

#ifdef ONLINE_PLATFORM
// Check download status of the Workshop mods. Returns true if anything changed.
// If the subscription attempt failed, stops polling (by setting subscribe_clicked to false).
bool MissingModsScreenImpl::pollDownloads()
{
    OnlinePlatform &op = knights_app.getOnlinePlatform();
    bool changed = false;

    for (ModRow &row : rows) {
        if (!row.download.workshop) continue;
        DownloadInfo d = row.download;
        d.status = op.getModDownloadStatus(row.vfs_name, d.progress, d.error);
        if (d != row.download) {
            row.download = d;
            changed = true;
        }
    }

    if (subscribeFailed()) subscribe_clicked = false;

    return changed;
}

// True if we may attempt auto-join: all workshop mods are
// MDS_INSTALLED, no local mods exist, and subscribe_clicked is true.
bool MissingModsScreenImpl::readyForAutoJoin() const
{
    if (!subscribe_clicked) return false;  // Never auto-join before subscribe is clicked
    for (const ModRow &row : rows) {
        if (row.download.workshop) {
            if (row.download.status != OnlinePlatform::MDS_INSTALLED) {
                // Non-installed workshop mod exists, so can't auto-join
                return false;
            }
        } else {
            // Local mod exists, so can't auto-join
            return false;
        }
    }
    return true;
}

// True if our subscribe attempt ended in failure, in other words: all
// Workshop mods have reached a terminal state (MDS_INSTALLED or
// MDS_FAILED) and at least one was FAILED.
bool MissingModsScreenImpl::subscribeFailed() const
{
    bool any_failed = false;
    for (const ModRow &row : rows) {
        if (row.download.workshop) {
            if (row.download.status != OnlinePlatform::MDS_INSTALLED
            && row.download.status != OnlinePlatform::MDS_FAILED) {
                // Non-terminal row exists; result is false.
                return false;
            }
            if (row.download.status == OnlinePlatform::MDS_FAILED) {
                any_failed = true;
            }
        }
    }
    return any_failed;
}
#endif

// Status message shown below the list of mods: an error line for each failed download,
// then overall download progress (or a message saying all Workshop mods are installed)
UTF8String MissingModsScreenImpl::statusText() const
{
    std::string msg;
#ifdef ONLINE_PLATFORM
    const Localization &loc = knights_app.getLocalization();
    auto add_line = [&msg](const UTF8String &line) {
        if (!msg.empty()) msg += "\n";
        msg += line.asUTF8();
    };

    int num_workshop = 0, num_installed = 0;
    int progress = -1;   // progress of the current download, or -1 if none
    bool waiting = false;
    for (const ModRow &row : rows) {
        const DownloadInfo &d = row.download;
        if (!d.workshop) continue;
        ++num_workshop;
        switch (d.status) {
        case OnlinePlatform::MDS_NOT_SUBSCRIBED:
            break;
        case OnlinePlatform::MDS_PENDING:
            waiting = true;
            break;
        case OnlinePlatform::MDS_DOWNLOADING:
            if (progress < 0) progress = d.progress;
            break;
        case OnlinePlatform::MDS_INSTALLED:
            ++num_installed;
            break;
        case OnlinePlatform::MDS_FAILED:
            add_line(loc.get(LocalKey("mod_download_failed"),
                             {LocalParam(UTF8String::fromUTF8Safe(row.title)),
                              LocalParam(loc.get(d.error))}));
            break;
        }
    }

    if (progress >= 0) {
        add_line(loc.get(LocalKey("mod_status_downloading"),
                         {LocalParam(std::min(num_installed + 1, num_workshop)),
                          LocalParam(num_workshop),
                          LocalParam(progress)}));
    } else if (waiting) {
        add_line(loc.get(LocalKey("mod_status_waiting")));
    } else if (num_workshop > 0 && num_installed == num_workshop && num_workshop < int(rows.size())) {
        // All Workshop mods installed, but there are local mods, so we can't auto-join
        add_line(loc.get(LocalKey("mod_status_workshop_installed")));
    }
#endif
    return UTF8String::fromUTF8Safe(msg);
}

// Returns true if the Subscribe button should be visible.
// subscribe_clicked must be false (i.e. there must not already be a
// Subscribe in progress), and at least one row must report
// CanSubscribe true (i.e. clicking Subscribe would actually do
// something for that row).
bool MissingModsScreenImpl::canSubscribe() const
{
#ifdef ONLINE_PLATFORM
    if (subscribe_clicked) return false;
    for (const ModRow &row : rows) {
        if (CanSubscribe(row.download)) return true;
    }
#endif
    return false;
}

// Update the status text. Returns true if layout() needs to be called (because the
// height of something changed, or the subscribe button needs to be shown or hidden).
bool MissingModsScreenImpl::updateStatus()
{
    const int old_height = status_text->getHeight();
    status_text->setText(statusText());
    status_text->adjustHeight();
    if (!gui_created || status_text->getHeight() != old_height || canSubscribe() != subscribe_shown) {
        return true;
    } else {
        window->invalidateAll();
        return false;
    }
}

// (Re-)create the panel. Expects all texts to be set already.
// (This shouldn't be called from within action(), because it destroys gui objects.)
void MissingModsScreenImpl::layout()
{
    subscribe_shown = canSubscribe();

    gui.setTop(nullptr);
    container.reset(new gcn::Container);
    container->setOpaque(false);

    int y = PAD;

    container->add(title_label.get(), PAD + WIDTH/2 - title_label->getWidth()/2, y);
    y += title_label->getHeight() + 2*PAD;

    container->add(intro_text.get(), PAD, y);
    y += intro_text->getHeight() + PAD*3/2;

    // Mod names are centred within a column as wide as the widest name, and that
    // column (plus the View buttons) is centred within the panel
    int names_width = 0;
    for (const ModRow &row : rows) names_width = std::max(names_width, row.name_label->getWidth());
    const int names_x = PAD + (WIDTH - names_width - view_space) / 2;

    for (ModRow &row : rows) {
        // If there is a View button, line up the name with the button's caption
        const int text_dy = row.view_button ? row.view_button->getTextY() : 0;
        container->add(row.name_label.get(), names_x + (names_width - row.name_label->getWidth()) / 2, y + text_dy);
        int row_height = text_dy + row.name_label->getHeight();
        if (row.view_button) {
            container->add(row.view_button.get(), names_x + names_width + VIEW_GAP, y);
            row_height = std::max(row_height, row.view_button->getHeight());
        }
        y += row_height + ROW_GAP;
    }
    y += PAD*3/2;

    container->add(instructions_text.get(), PAD, y);
    y += instructions_text->getHeight() + 2*PAD;

    if (!status_text->getText().empty()) {
        container->add(status_text.get(), PAD, y);
        y += status_text->getHeight() + 2*PAD;
    }

    if (subscribe_shown) container->add(subscribe_button.get(), PAD, y);
    container->add(exit_button.get(), PAD + WIDTH - exit_button->getWidth(), y);
    y += exit_button->getHeight() + PAD;

    container->setSize(2*PAD + WIDTH, y);

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
    if (event.getSource() == exit_button.get()) {
        knights_app.exitLastJoin();
        return;
    }

#ifdef ONLINE_PLATFORM
    OnlinePlatform &op = knights_app.getOnlinePlatform();
    if (event.getSource() == subscribe_button.get()) subscribe_clicked = true;
    for (const ModRow &row : rows) {
        if (event.getSource() == row.view_button.get()) {
            op.browseWorkshopMod(row.vfs_name);
        } else if (event.getSource() == subscribe_button.get() && CanSubscribe(row.download)) {
            // The new status will be picked up by the next pollDownloads() call
            op.subscribeMod(row.vfs_name);
        }
    }
#endif
}

void MissingModsScreenImpl::update()
{
    const unsigned int now = knights_app.getTimer().getMsec();
    if (now - last_poll_msec < POLL_INTERVAL_MS) return;
    last_poll_msec = now;

    bool need_row_labels_update = false;
    bool need_status_update = false;
    bool need_relayout = false;

    // If the gui is already created, then run pollTitles anyway, to handle the
    // (rare) case where titles come in after the gui is created. (In the common
    // case where all titles are finalized already, pollTitles will return false,
    // quickly.)
    if (gui_created && pollTitles()) {
        need_row_labels_update = need_status_update = need_relayout = true;
    }

    // If the gui is NOT created, run pollTitles and see if the gui is ready to create.
    if (!gui_created) {
        pollTitles();
        if (allTitlesFinalized() || now - start_msec >= TITLE_TIMEOUT_MS) {
            need_row_labels_update = need_status_update = need_relayout = true;
        }
    }

#ifdef ONLINE_PLATFORM
    // If Subscribe has been clicked, then poll downloads.
    // If pollDownloads returned true, that means something changed, so we have
    // to update the status text.
    if (subscribe_clicked && pollDownloads()) need_status_update = true;

    // Join the game automatically once all downloads are complete (unless there are
    // local mods). subscribe_clicked is reset to make sure we only try once.
    if (readyForAutoJoin()) {
        subscribe_clicked = false;
        knights_app.retryLastJoin();

        // retryLastJoin will close this UI, so there is no point continuing with updates
        return;
    }
#endif

    // Apply required updates.
    if (need_row_labels_update) {
        // Long names are shortened so that the name and View button fit in the panel
        for (ModRow &row : rows) {
            row.name_label->setCaption(TruncateToWidth(*row.name_label->getFont(),
                                                       row.title,
                                                       WIDTH - view_space));
            row.name_label->adjustSize();
        }
    }

    if (need_status_update) {
        if (updateStatus()) {
            need_relayout = true;
        }
    }

    if (need_relayout) {
        layout();
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

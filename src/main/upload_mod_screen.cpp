/*
 * upload_mod_screen.cpp
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

#include "adjust_list_box_size.hpp"
#include "knights_app.hpp"
#include "localization.hpp"
#include "make_scroll_area.hpp"
#include "mods_screen.hpp"
#include "module_manager.hpp"
#include "online_platform.hpp"
#include "tooltip_widget.hpp"
#include "upload_mod_screen.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"
#include "gui_text_wrap.hpp"

#include "guichan.hpp"

#include <exception>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace {
    // How often to poll the online platform for upload progress
    const unsigned int UPLOAD_POLL_INTERVAL_MS = 200;

    const int TOOLTIP_MAX_WIDTH = 250;

    // Built-in modules, which are never offered for upload
    const char * const BASE_MODULE_NAME = "base";
    const char * const TUTORIAL_MODULE_NAME = "tutorial";

    // Which buttons to show at the bottom of the dialog
    enum Buttons {
        NO_BUTTONS,
        CANCEL_ONLY,
        UPLOAD_AND_CANCEL,
        CLOSE_ONLY,
        EXIT_ONLY
    };

    // Which stage of the upload process we are at
    enum State {
        SELECTING,    // Choosing a mod from the list
        CONFIRMING,   // Showing the "this folder will be uploaded" message
        UPLOADING,    // Showing the progress meter
        FINISHED      // Showing the success or failure message
    };

    // Requests from action() or mouseClicked(), to be carried out by update()
    // (because they destroy gui objects).
    enum Request {
        NO_REQUEST,
        SHOW_LIST,
        SHOW_CONFIRM,
        START_UPLOAD
    };

    UTF8String PathToUTF8(const std::filesystem::path &path)
    {
        const std::u8string str = path.u8string();
        return UTF8String::fromUTF8Safe(std::string(str.begin(), str.end()));
    }

    class StringListModel : public gcn::ListModel {
    public:
        explicit StringListModel(const std::vector<std::string> &s) : strings(s) { }

        int getNumberOfElements() override {
            return int(strings.size());
        }

        std::string getElementAt(int i) override {
            if (i >= 0 && i < int(strings.size())) return strings[i];
            return std::string();
        }

    private:
        const std::vector<std::string> &strings;
    };
}

class UploadModScreenImpl : public gcn::ActionListener, public gcn::MouseListener {
public:
    UploadModScreenImpl(KnightsApp &app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui);
    void action(const gcn::ActionEvent &event) override;
    void mouseClicked(gcn::MouseEvent &event) override;
    void mouseEntered(gcn::MouseEvent &event) override;
    void mouseExited(gcn::MouseEvent &event) override;
    void update();

private:
    void rebuild(const UTF8String &msg, Buttons buttons, bool show_list);
    void showList();
    void showConfirm();
    void showProgress(int progress);
    void startUpload();
    void pollUpload();

    KnightsApp &knights_app;
    boost::shared_ptr<Coercri::Window> window;
    gcn::Gui &gui;

    std::vector<std::string> mod_names;  // Uploadable mods (vfs names)
    std::filesystem::path mod_dir;       // Directory of the mod being uploaded
    State state;
    Request request;
    int last_progress;
    unsigned int last_poll_msec;

    std::unique_ptr<GuiCentre> centre;
    std::unique_ptr<GuiPanel> panel;
    std::unique_ptr<gcn::Container> container;
    std::unique_ptr<gcn::Label> title_label;
    std::unique_ptr<GuiTextWrap> message;
    std::unique_ptr<StringListModel> list_model;
    std::unique_ptr<gcn::ListBox> listbox;
    std::unique_ptr<gcn::ScrollArea> scroll_area;
    std::unique_ptr<gcn::Button> upload_button;
    std::unique_ptr<gcn::Button> cancel_button;
    std::unique_ptr<gcn::Button> close_button;
    std::unique_ptr<gcn::Button> exit_button;
    std::unique_ptr<TooltipWidget> close_tooltip;
};

UploadModScreenImpl::UploadModScreenImpl(KnightsApp &app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g)
    : knights_app(app), window(win), gui(g),
      state(SELECTING), request(NO_REQUEST), last_progress(-1), last_poll_msec(0)
{
    const Localization &loc = knights_app.getLocalization();

    title_label.reset(new gcn::Label(loc.get(LocalKey("upload_mod_title")).asUTF8()));
    title_label->setForegroundColor(gcn::Color(0, 0, 128));

    message.reset(new GuiTextWrap);
    message->setForegroundColor(gcn::Color(30, 30, 30));

    upload_button.reset(new GuiButton(loc.get(LocalKey("upload")).asUTF8()));
    upload_button->addActionListener(this);
    cancel_button.reset(new GuiButton(loc.get(LocalKey("cancel")).asUTF8()));
    cancel_button->addActionListener(this);
    close_button.reset(new GuiButton(loc.get(LocalKey("close")).asUTF8()));
    close_button->addActionListener(this);
    close_button->addMouseListener(this);
    close_tooltip.reset(new TooltipWidget(
        loc.get(LocalKey("upload_close_tooltip")), TOOLTIP_MAX_WIDTH, knights_app.getTimer(), *window));
    exit_button.reset(new GuiButton(loc.get(LocalKey("exit")).asUTF8()));
    exit_button->addActionListener(this);

    // Only local mods can be uploaded (excluding the built-in ones)
    const ModuleManager &mm = knights_app.getModuleManager();
    for (const ModulesTxtLine &line : mm.getInstalledModules()) {
        const std::string &name = line.module_name;
        if (mm.isLocalModule(name) && name != BASE_MODULE_NAME && name != TUTORIAL_MODULE_NAME) {
            mod_names.push_back(name);
        }
    }

    list_model.reset(new StringListModel(mod_names));
    listbox.reset(new gcn::ListBox);
    listbox->setListModel(list_model.get());
    listbox->addMouseListener(this);
    if (!mod_names.empty()) listbox->setSelected(0);
    scroll_area = MakeScrollArea(*listbox, 500, 200);

    // If an upload is already running (i.e. the user closed this screen during an
    // upload, then came back) then go straight to the progress display
    int progress = 0;
    UTF8String error;
    if (knights_app.getOnlinePlatform().getUploadStatus(progress, error) == OnlinePlatform::UPLOAD_IN_PROGRESS) {
        showProgress(progress);
    } else {
        showList();
    }
}

// Set the message and buttons, and (re-)create the panel at the appropriate size.
// If show_list is true, the list of mods is shown between the message and the buttons.
// (This shouldn't be called from within action(), because it destroys gui objects.)
void UploadModScreenImpl::rebuild(const UTF8String &msg, Buttons buttons, bool show_list)
{
    gui.setTop(nullptr);
    container.reset();
    panel.reset();
    centre.reset();

    container.reset(new gcn::Container);
    container->setOpaque(false);

    const int pad = 10;
    const int width = 500;
    int y = pad;

    container->add(title_label.get(), pad + width/2 - title_label->getWidth()/2, y);
    y += title_label->getHeight() + 2*pad;

    message->setWidth(width);
    message->setText(msg);
    message->adjustHeight();
    container->add(message.get(), pad, y);
    y += message->getHeight() + 2*pad;

    if (show_list) {
        container->add(scroll_area.get(), pad, y);
        y += scroll_area->getHeight() + 2*pad;
    }

    switch (buttons) {
    case NO_BUTTONS:
        break;
    case CANCEL_ONLY:
        container->add(cancel_button.get(), pad + width/2 - cancel_button->getWidth()/2, y);
        y += cancel_button->getHeight() + pad;
        break;
    case UPLOAD_AND_CANCEL:
        container->add(upload_button.get(), pad, y);
        container->add(cancel_button.get(), pad + width - cancel_button->getWidth(), y);
        y += cancel_button->getHeight() + pad;
        break;
    case CLOSE_ONLY:
        container->add(close_button.get(), pad + width/2 - close_button->getWidth()/2, y);
        // Tooltip appears above the button (added last so it renders on top)
        container->add(close_tooltip.get(), pad + width/2 - close_tooltip->getWidth()/2,
                       y - close_tooltip->getHeight() - 3);
        y += close_button->getHeight() + pad;
        break;
    case EXIT_ONLY:
        container->add(exit_button.get(), pad + width/2 - exit_button->getWidth()/2, y);
        y += exit_button->getHeight() + pad;
        break;
    }

    container->setSize(2*pad + width, y);

    panel.reset(new GuiPanel(container.get()));
    centre.reset(new GuiCentre(panel.get()));

    int w, h;
    window->getSize(w, h);
    centre->setSize(w, h);
    gui.setTop(centre.get());
    if (show_list) AdjustListBoxSize(*listbox, *scroll_area);
    gui.logic();
    window->invalidateAll();
}

void UploadModScreenImpl::showList()
{
    const Localization &loc = knights_app.getLocalization();
    state = SELECTING;
    if (mod_names.empty()) {
        rebuild(loc.get(LocalKey("upload_no_local_mods")), CANCEL_ONLY, false);
    } else {
        rebuild(loc.get(LocalKey("upload_select_mod")), UPLOAD_AND_CANCEL, true);
    }
}

void UploadModScreenImpl::showConfirm()
{
    const int sel = listbox->getSelected();
    if (sel < 0 || sel >= int(mod_names.size())) return;

    const Localization &loc = knights_app.getLocalization();
    state = CONFIRMING;
    mod_dir = knights_app.getModuleManager().getModulePath(mod_names[sel]);
    const bool first_time = knights_app.getOnlinePlatform().isFirstTimeUpload(mod_dir);
    rebuild(loc.get(LocalKey(first_time ? "upload_new_item" : "upload_existing_item"),
                    PathToUTF8(mod_dir)),
            UPLOAD_AND_CANCEL,
            false);
}

void UploadModScreenImpl::action(const gcn::ActionEvent &event)
{
    if (event.getSource() == upload_button.get()) {
        request = (state == SELECTING ? SHOW_CONFIRM : START_UPLOAD);
    } else if (event.getSource() == cancel_button.get() && state == CONFIRMING) {
        request = SHOW_LIST;
    } else {
        // Cancel (from the list), Close or Exit: go back to the mods screen.
        // (Close leaves any upload running in the background.)
        std::unique_ptr<Screen> new_screen(new ModsScreen);
        knights_app.requestScreenChange(std::move(new_screen));
    }
}

void UploadModScreenImpl::mouseClicked(gcn::MouseEvent &event)
{
    // Double clicking a mod in the list is equivalent to clicking Upload
    if (state == SELECTING && event.getButton() == gcn::MouseEvent::LEFT && event.getClickCount() == 2) {
        request = SHOW_CONFIRM;
    }
}

void UploadModScreenImpl::mouseEntered(gcn::MouseEvent &event)
{
    if (event.getSource() == close_button.get()) close_tooltip->scheduleShow();
}

void UploadModScreenImpl::mouseExited(gcn::MouseEvent &event)
{
    if (event.getSource() == close_button.get()) close_tooltip->cancelShow();
}

void UploadModScreenImpl::showProgress(int progress)
{
    const Localization &loc = knights_app.getLocalization();
    state = UPLOADING;
    last_progress = progress;
    rebuild(loc.get(LocalKey("upload_progress"), {LocalParam(progress)}),
            CLOSE_ONLY,
            false);
}

void UploadModScreenImpl::startUpload()
{
    knights_app.getOnlinePlatform().uploadMod(mod_dir);
    showProgress(0);
}

void UploadModScreenImpl::update()
{
    const Request req = request;
    request = NO_REQUEST;
    switch (req) {
    case NO_REQUEST: break;
    case SHOW_LIST: showList(); break;
    case SHOW_CONFIRM: showConfirm(); break;
    case START_UPLOAD: startUpload(); break;
    }

    if (state == UPLOADING) pollUpload();
}

void UploadModScreenImpl::pollUpload()
{
    const unsigned int now = knights_app.getTimer().getMsec();
    if (now - last_poll_msec < UPLOAD_POLL_INTERVAL_MS) return;
    last_poll_msec = now;

    const Localization &loc = knights_app.getLocalization();
    int progress = 0;
    UTF8String error;
    switch (knights_app.getOnlinePlatform().getUploadStatus(progress, error)) {
    case OnlinePlatform::UPLOAD_IN_PROGRESS:
        if (progress != last_progress) {
            last_progress = progress;
            message->setText(loc.get(LocalKey("upload_progress"), {LocalParam(progress)}));
            window->invalidateAll();
        }
        break;

    case OnlinePlatform::UPLOAD_COMPLETE:
        state = FINISHED;
        rebuild(loc.get(LocalKey("upload_complete")), EXIT_ONLY, false);
        break;

    case OnlinePlatform::UPLOAD_ERROR:
        state = FINISHED;
        rebuild(loc.get(LocalKey("upload_failed"), error), EXIT_ONLY, false);
        break;
    }
}


//
// UploadModScreen
//

bool UploadModScreen::start(KnightsApp &knights_app, boost::shared_ptr<Coercri::Window> w, gcn::Gui &gui)
{
    pimpl.reset(new UploadModScreenImpl(knights_app, w, gui));
    return true;
}

void UploadModScreen::update()
{
    pimpl->update();
}

#endif  // ONLINE_PLATFORM

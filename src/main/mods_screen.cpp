/*
 * mods_screen.cpp
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

#include "adjust_list_box_size.hpp"
#include "knights_app.hpp"
#include "localization.hpp"
#include "make_scroll_area.hpp"
#include "mods_screen.hpp"
#include "module_manager.hpp"
#include "title_screen.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"
#include "gui_text_wrap.hpp"
#include "tooltip_widget.hpp"

#include "guichan.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace {

    // The "base" module is always enabled and always loaded first.
    const char * const BASE_MODULE_NAME = "base";

    // The "tutorial" module is filtered out from the list
    const char * const TUTORIAL_MODULE_NAME = "tutorial";

    // How often to poll the online platform for mod titles
    const unsigned int TITLE_POLL_INTERVAL_MS = 200;

    struct ModEntry {
        std::string name;    // "vfs mod name" (identifier; this is what gets saved)
        std::string title;   // Displayed in the list (UTF-8). Same as name until/unless
                             // the online platform gives us a proper title.
        bool title_pending;  // true if still waiting for the online platform
        bool enabled;
        bool locked;   // true if the user cannot toggle or move this entry (i.e. "base")
    };

    bool TitleLess(const ModEntry &a, const ModEntry &b)
    {
        return std::tie(a.title, a.name) < std::tie(b.title, b.name);
    }

    // Colour used for locked entries
    const gcn::Color LOCKED_COLOUR(150, 150, 150);

    // Simple ListModel exposing the titles from a vector<ModEntry>.
    class ModListModel : public gcn::ListModel {
    public:
        explicit ModListModel(const std::vector<ModEntry> &e) : entries(e) { }

        int getNumberOfElements() override {
            return int(entries.size());
        }

        std::string getElementAt(int i) override {
            if (i >= 0 && i < int(entries.size())) return entries[i].title;
            return std::string();
        }

    private:
        const std::vector<ModEntry> &entries;
    };
}

// ModListBox: a ListBox that draws a check box to the left of each row.
// Clicking the check box (or pressing Space on the selected row) toggles
// the "enabled" flag for that module.
class ModListBox : public gcn::ListBox {
public:
    ModListBox(ModsScreenImpl &impl, const std::vector<ModEntry> &entries)
        : impl(impl), entries(entries) { }

    void draw(gcn::Graphics *graphics) override;
    unsigned int getRowHeight() const override;
    void mousePressed(gcn::MouseEvent &mouseEvent) override;
    void keyPressed(gcn::KeyEvent &keyEvent) override;

private:
    // Size of the check box, and the total width of the check box column
    int getBoxSize() const { return getFont()->getHeight() - 2; }
    int getBoxColumnWidth() const { return getBoxSize() + 8; }
    void drawCheckBox(gcn::Graphics *graphics, int x, int y, bool checked, bool locked);

    ModsScreenImpl &impl;
    const std::vector<ModEntry> &entries;
};

class ModsScreenImpl : public gcn::ActionListener, public gcn::MouseListener {
public:
    ModsScreenImpl(KnightsApp &app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui);
    void action(const gcn::ActionEvent &event) override;
    void mouseEntered(gcn::MouseEvent &e) override;
    void mouseExited(gcn::MouseEvent &e) override;

    // Called by ModListBox when the user clicks a check box.
    void toggleEnabled(int index);

    // Called every frame. Polls for any mod titles that we are still waiting for.
    void update();

private:
    void populateFromModuleManager();
    void refreshList();
    void sendTitleQuery(const std::vector<std::string> &names);
    ModEntry makeEntry(const std::string &name, bool enabled);
    void resolveTitle(ModEntry &entry);
    void enforceBaseModule();
    void moveSelected(int delta);
    void listChanged();

    KnightsApp &knights_app;
    boost::shared_ptr<Coercri::Window> window;
    gcn::Gui &gui;

    std::vector<ModEntry> entries;
    std::unique_ptr<ModListModel> list_model;
    bool titles_pending;   // true if any entry might have title_pending set
    unsigned int last_poll_msec;

    std::unique_ptr<GuiCentre> centre;
    std::unique_ptr<GuiPanel> panel;
    std::unique_ptr<gcn::Container> container;
    std::unique_ptr<gcn::Label> title_label;
    std::unique_ptr<GuiTextWrap> help_text;
    std::unique_ptr<ModListBox> listbox;
    std::unique_ptr<gcn::ScrollArea> scroll_area;
    std::unique_ptr<gcn::Button> up_button;
    std::unique_ptr<gcn::Button> down_button;
    std::unique_ptr<gcn::Button> refresh_button;
    std::unique_ptr<gcn::Button> save_button;
    std::unique_ptr<gcn::Button> cancel_button;
#ifdef ONLINE_PLATFORM
    std::unique_ptr<gcn::Button> browse_button;
    std::unique_ptr<gcn::Button> upload_button;
#endif
    std::unique_ptr<TooltipWidget> up_tooltip;
    std::unique_ptr<TooltipWidget> down_tooltip;
    std::unique_ptr<TooltipWidget> refresh_tooltip;
#ifdef ONLINE_PLATFORM
    std::unique_ptr<TooltipWidget> upload_tooltip;
    std::unique_ptr<TooltipWidget> browse_tooltip;
#endif
};


//
// ModListBox
//

unsigned int ModListBox::getRowHeight() const
{
    return getFont()->getHeight() + 4;
}

void ModListBox::drawCheckBox(gcn::Graphics *graphics, int x, int y, bool checked, bool locked)
{
    // Modelled on gcn::CheckBox::drawBox
    const int h = getBoxSize();

    const gcn::Color faceColor = getBaseColor();
    const gcn::Color highlightColor = faceColor + 0x303030;
    const gcn::Color shadowColor = faceColor - 0x303030;

    graphics->setColor(shadowColor);
    graphics->drawLine(x, y, x + h - 1, y);
    graphics->drawLine(x, y, x, y + h - 1);

    graphics->setColor(highlightColor);
    graphics->drawLine(x + h - 1, y, x + h - 1, y + h - 1);
    graphics->drawLine(x, y + h - 1, x + h - 2, y + h - 1);

    // Locked check boxes get a grey interior, like a disabled control
    graphics->setColor(locked ? gcn::Color(225, 225, 225) : gcn::Color(255, 255, 255));
    graphics->fillRectangle(gcn::Rectangle(x + 1, y + 1, h - 2, h - 2));

    if (checked) {
        graphics->setColor(locked ? LOCKED_COLOUR : getForegroundColor());
        // Two-pixel-thick tick mark
        graphics->drawLine(x + 2, y + 4, x + 2, y + h - 3);
        graphics->drawLine(x + 3, y + 4, x + 3, y + h - 3);
        graphics->drawLine(x + 4, y + h - 4, x + h - 3, y + 3);
        graphics->drawLine(x + 4, y + h - 5, x + h - 5, y + 4);
    }
}

void ModListBox::draw(gcn::Graphics *graphics)
{
    graphics->setColor(getBackgroundColor());
    graphics->fillRectangle(gcn::Rectangle(0, 0, getWidth(), getHeight()));

    if (mListModel == NULL) return;

    graphics->setFont(getFont());

    // Only draw the rows that are actually visible (see gcn::ListBox::draw)
    const gcn::ClipRectangle currentClipArea = graphics->getCurrentClipArea();
    const int rowHeight = getRowHeight();

    int numberOfRows = currentClipArea.height / rowHeight + 2;
    if (numberOfRows > mListModel->getNumberOfElements()) {
        numberOfRows = mListModel->getNumberOfElements();
    }

    const int startRow = (getY() < 0) ? (-1 * (getY() / rowHeight)) : 0;

    const int box_size = getBoxSize();
    const int text_x = getBoxColumnWidth();
    const int font_height = getFont()->getHeight();

    int y = rowHeight * startRow;
    for (int i = startRow; i < startRow + numberOfRows; ++i) {
        if (i == mSelected) {
            graphics->setColor(getSelectionColor());
            graphics->fillRectangle(gcn::Rectangle(0, y, getWidth(), rowHeight));
        }

        const bool valid = (i >= 0 && i < int(entries.size()));
        const bool checked = valid && entries[i].enabled;
        const bool locked = valid && entries[i].locked;
        drawCheckBox(graphics, 3, y + (rowHeight - box_size) / 2, checked, locked);

        graphics->setColor(locked ? LOCKED_COLOUR : getForegroundColor());
        graphics->drawText(mListModel->getElementAt(i), text_x, y + (rowHeight - font_height) / 2);

        y += rowHeight;
    }
}

void ModListBox::mousePressed(gcn::MouseEvent &mouseEvent)
{
    if (mouseEvent.getButton() == gcn::MouseEvent::LEFT
    && mouseEvent.getX() < getBoxColumnWidth()) {
        // Click on the check box column: toggle, but don't change selection
        const int row = mouseEvent.getY() / getRowHeight();
        if (row >= 0 && row < int(entries.size())) {
            impl.toggleEnabled(row);
        }
        mouseEvent.consume();
        return;
    }

    gcn::ListBox::mousePressed(mouseEvent);
}

void ModListBox::keyPressed(gcn::KeyEvent &keyEvent)
{
    if (keyEvent.getKey().getValue() == gcn::Key::SPACE) {
        if (mSelected >= 0 && mSelected < int(entries.size())) {
            impl.toggleEnabled(mSelected);
        }
        keyEvent.consume();
        return;
    }

    gcn::ListBox::keyPressed(keyEvent);
}


//
// ModsScreenImpl
//

ModsScreenImpl::ModsScreenImpl(KnightsApp &app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g)
    : knights_app(app), window(win), gui(g), titles_pending(false), last_poll_msec(0)
{
    const Localization &loc = knights_app.getLocalization();

    populateFromModuleManager();
    list_model.reset(new ModListModel(entries));

    container.reset(new gcn::Container);
    container->setOpaque(false);

    const int pad = 10;
    const int list_width = 600;
    const int list_height = 300;
    int y = pad;

    // Title
    title_label.reset(new gcn::Label(loc.get(LocalKey("mods")).asUTF8()));
    title_label->setForegroundColor(gcn::Color(0, 0, 128));

    // Side buttons (to the right of the list): Up / Down at the top,
    // Upload Mod / Refresh List at the bottom.
    up_button.reset(new GuiButton(loc.get(LocalKey("move_up")).asUTF8()));
    up_button->addActionListener(this);
    down_button.reset(new GuiButton(loc.get(LocalKey("move_down")).asUTF8()));
    down_button->addActionListener(this);
#ifdef ONLINE_PLATFORM
    upload_button.reset(new GuiButton(loc.get(LocalKey("upload_mod")).asUTF8()));
    upload_button->addActionListener(this);
#endif
    refresh_button.reset(new GuiButton(loc.get(LocalKey("refresh_list")).asUTF8()));
    refresh_button->addActionListener(this);

    int side_button_width =
        std::max(std::max(up_button->getWidth(), down_button->getWidth()),
                 refresh_button->getWidth());
#ifdef ONLINE_PLATFORM
    side_button_width = std::max(side_button_width, upload_button->getWidth());
#endif

    up_button->setWidth(side_button_width);
    down_button->setWidth(side_button_width);
    refresh_button->setWidth(side_button_width);
#ifdef ONLINE_PLATFORM
    upload_button->setWidth(side_button_width);
#endif

    const int width = list_width + pad + side_button_width;

    container->add(title_label.get(), pad + width/2 - title_label->getWidth()/2, y);
    y += title_label->getHeight() + 2*pad;

    // Help text
    help_text.reset(new GuiTextWrap);
    help_text->setWidth(width);
    help_text->setText(loc.get(LocalKey("mods_help")));
    help_text->adjustHeight();
    help_text->setForegroundColor(gcn::Color(30, 30, 30));
    container->add(help_text.get(), pad, y);
    y += help_text->getHeight() + pad;

    // The module list
    listbox.reset(new ModListBox(*this, entries));
    listbox->setListModel(list_model.get());
    listbox->setWidth(list_width);
    scroll_area = MakeScrollArea(*listbox, list_width, list_height);
    container->add(scroll_area.get(), pad, y);

    const int side_x = pad + list_width + pad;
    const int list_bottom = y + scroll_area->getHeight();

    // Up / Down: aligned with the top of the list
    container->add(up_button.get(), side_x, y);
    container->add(down_button.get(), side_x, y + up_button->getHeight() + pad);

    // Refresh List: aligned with the bottom of the list; Upload Mod just above it
    const int refresh_y = list_bottom - refresh_button->getHeight();
    container->add(refresh_button.get(), side_x, refresh_y);
#ifdef ONLINE_PLATFORM
    container->add(upload_button.get(), side_x, refresh_y - pad - upload_button->getHeight());
#endif

    y = list_bottom + 2*pad;

    // Save, Browse Workshop and Cancel buttons
    save_button.reset(new GuiButton(loc.get(LocalKey("save_changes")).asUTF8()));
    save_button->addActionListener(this);
#ifdef ONLINE_PLATFORM
    browse_button.reset(new GuiButton(loc.get(LocalKey("browse_workshop")).asUTF8()));
    browse_button->addActionListener(this);
#endif
    cancel_button.reset(new GuiButton(loc.get(LocalKey("cancel")).asUTF8()));
    cancel_button->addActionListener(this);
    container->add(save_button.get(), pad, y);
#ifdef ONLINE_PLATFORM
    container->add(browse_button.get(), pad + width/2 - browse_button->getWidth()/2, y);
#endif
    container->add(cancel_button.get(), pad + width - cancel_button->getWidth(), y);
    y += save_button->getHeight() + pad;

    // Tooltips -- added last so they render on top of all other widgets
    const int TOOLTIP_MAX_WIDTH = list_width / 2;
    Coercri::Timer &tmr = knights_app.getTimer();

    up_tooltip.reset(new TooltipWidget(
        loc.get(LocalKey("move_up_tooltip")), TOOLTIP_MAX_WIDTH, tmr, *window));
    down_tooltip.reset(new TooltipWidget(
        loc.get(LocalKey("move_down_tooltip")), TOOLTIP_MAX_WIDTH, tmr, *window));
    refresh_tooltip.reset(new TooltipWidget(
        loc.get(LocalKey("refresh_list_tooltip")), TOOLTIP_MAX_WIDTH, tmr, *window));
#ifdef ONLINE_PLATFORM
    upload_tooltip.reset(new TooltipWidget(
        loc.get(LocalKey("upload_mod_tooltip")), TOOLTIP_MAX_WIDTH, tmr, *window));
    browse_tooltip.reset(new TooltipWidget(
        loc.get(LocalKey("browse_workshop_tooltip")), TOOLTIP_MAX_WIDTH, tmr, *window));
#endif

    // Side button tooltips appear to the left of the button (over the list)
    auto place_side_tooltip = [&](TooltipWidget *tt, gcn::Widget *btn) {
        container->add(tt, btn->getX() - tt->getWidth() - 3, btn->getY());
    };
    place_side_tooltip(up_tooltip.get(),      up_button.get());
    place_side_tooltip(down_tooltip.get(),    down_button.get());
    place_side_tooltip(refresh_tooltip.get(), refresh_button.get());
#ifdef ONLINE_PLATFORM
    place_side_tooltip(upload_tooltip.get(),  upload_button.get());

    // Browse Workshop tooltip appears above the button
    container->add(browse_tooltip.get(),
                   browse_button->getX() + 15,
                   browse_button->getY() - browse_tooltip->getHeight() - 3);
#endif

    up_button->addMouseListener(this);
    down_button->addMouseListener(this);
    refresh_button->addMouseListener(this);
#ifdef ONLINE_PLATFORM
    upload_button->addMouseListener(this);
    browse_button->addMouseListener(this);
#endif

    container->setSize(2*pad + width, y);

    panel.reset(new GuiPanel(container.get()));
    centre.reset(new GuiCentre(panel.get()));
    gui.setTop(centre.get());

    AdjustListBoxSize(*listbox, *scroll_area);
}

// Build the initial list: enabled modules first (in load order), then
// all other installed modules (alphabetically). Updates the
// ModuleManager first.
void ModsScreenImpl::populateFromModuleManager()
{
    ModuleManager &mm = knights_app.getModuleManager();
    mm.update();

    const std::vector<std::string> enabled = mm.getEnabledModules();
    const std::vector<std::string> installed = mm.getInstalledModules();

    std::vector<std::string> all_names = enabled;
    all_names.insert(all_names.end(), installed.begin(), installed.end());
    sendTitleQuery(all_names);

    entries.clear();

    std::unordered_set<std::string> seen;
    for (const std::string &name : enabled) {
        if (seen.insert(name).second && name != TUTORIAL_MODULE_NAME) {
            entries.push_back(makeEntry(name, true));
        }
    }

    const size_t num_enabled = entries.size();
    for (const std::string &name : installed) {
        if (seen.insert(name).second && name != TUTORIAL_MODULE_NAME) {
            entries.push_back(makeEntry(name, false));
        }
    }
    std::sort(entries.begin() + num_enabled, entries.end(), TitleLess);

    enforceBaseModule();
}

// Ask the online platform (if any) for the titles of the given mods.
// Results are picked up by resolveTitle.
void ModsScreenImpl::sendTitleQuery(const std::vector<std::string> &names)
{
#ifdef ONLINE_PLATFORM
    knights_app.getOnlinePlatform().sendModQuery(names);
#endif
    titles_pending = true;   // update() will clear this once all titles are resolved
}

// Make a new ModEntry. The title is looked up immediately if it is available,
// otherwise it is set to the vfs name for now (and update() will fix it later).
ModEntry ModsScreenImpl::makeEntry(const std::string &name, bool enabled)
{
    ModEntry entry{name, name, true, enabled, false};
    resolveTitle(entry);
    return entry;
}

// Check whether the title query result is available yet. If so, set
// entry.title and clear entry.title_pending.
void ModsScreenImpl::resolveTitle(ModEntry &entry)
{
#ifdef ONLINE_PLATFORM
    OnlinePlatform::ModDetails details;
    switch (knights_app.getOnlinePlatform().getModQueryResult(entry.name, details)) {
    case OnlinePlatform::MQR_WAITING:
        return;
    case OnlinePlatform::MQR_SUCCESS:
        entry.title = details.title.asUTF8();
        break;
    case OnlinePlatform::MQR_FAILED:
        // Assume this is a local (non-workshop) mod, and just use the vfs name
        entry.title = entry.name;
        break;
    }
#endif
    entry.title_pending = false;
}

void ModsScreenImpl::update()
{
    if (!titles_pending) return;

    const unsigned int now = knights_app.getTimer().getMsec();
    if (now - last_poll_msec < TITLE_POLL_INTERVAL_MS) return;
    last_poll_msec = now;

    // Note: Titles are replaced in-place. We don't re-sort the list at this point.
    bool changed = false;
    titles_pending = false;
    for (ModEntry &entry : entries) {
        if (entry.title_pending) {
            resolveTitle(entry);
            if (entry.title_pending) {
                titles_pending = true;
            } else {
                changed = true;
            }
        }
    }
    if (changed) listChanged();
}

// Update the ModuleManager (which re-scans installed modules), then
// drop any uninstalled modules from the list, and add newly-installed
// modules (disabled) to the end of the list (alphabetically). Does
// not re-read modules.txt or change the enabled-state of any module
// in the list.
void ModsScreenImpl::refreshList()
{
    ModuleManager &mm = knights_app.getModuleManager();
    mm.update();

    const std::vector<std::string> installed = mm.getInstalledModules();
    const std::unordered_set<std::string> installed_set(installed.begin(), installed.end());

    sendTitleQuery(installed);

    // Remember the selected module (by name) so we can restore it afterwards
    std::string selected_name;
    const int sel = listbox->getSelected();
    if (sel >= 0 && sel < int(entries.size())) selected_name = entries[sel].name;

    // Remove modules that are no longer installed
    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
            [&installed_set](const ModEntry &e) { return installed_set.count(e.name) == 0; }),
        entries.end());

    // Re-check the titles of the existing entries (update() will do this). The
    // current title stays on display in the meantime.
    std::unordered_set<std::string> present;
    for (ModEntry &e : entries) {
        e.title_pending = true;
        present.insert(e.name);
    }

    // Add newly discovered modules at the end (alphabetically), disabled.
    // Exception: TUTORIAL_MODULE_NAME is ignored.
    const size_t num_existing = entries.size();
    for (const std::string &name : installed) {
        if (present.count(name) == 0 && name != TUTORIAL_MODULE_NAME) {
            entries.push_back(makeEntry(name, false));
        }
    }
    std::sort(entries.begin() + num_existing, entries.end(), TitleLess);

    enforceBaseModule();

    // Restore selection
    int new_sel = -1;
    for (int i = 0; i < int(entries.size()); ++i) {
        if (entries[i].name == selected_name) {
            new_sel = i;
            break;
        }
    }
    listbox->setSelected(new_sel);

    listChanged();
}

// Ensure "base" (if installed) is enabled, locked, and at the top of the list.
void ModsScreenImpl::enforceBaseModule()
{
    auto it = std::find_if(entries.begin(), entries.end(),
        [](const ModEntry &e) { return e.name == BASE_MODULE_NAME; });
    if (it == entries.end()) return;

    it->enabled = true;
    it->locked = true;
    std::rotate(entries.begin(), it, it + 1);
}

void ModsScreenImpl::toggleEnabled(int index)
{
    if (index < 0 || index >= int(entries.size())) return;
    if (entries[index].locked) return;

    entries[index].enabled = !entries[index].enabled;
    listChanged();
}

// Move the selected entry up (delta = -1) or down (delta = +1).
void ModsScreenImpl::moveSelected(int delta)
{
    const int i = listbox->getSelected();
    const int j = i + delta;
    if (i < 0 || i >= int(entries.size())) return;
    if (j < 0 || j >= int(entries.size())) return;

    // Locked entries ("base") can neither be moved nor swapped with.
    if (entries[i].locked || entries[j].locked) return;

    std::swap(entries[i], entries[j]);
    listbox->setSelected(j);
    listChanged();
}

void ModsScreenImpl::listChanged()
{
    AdjustListBoxSize(*listbox, *scroll_area);
    gui.logic();
    window->invalidateAll();
}

void ModsScreenImpl::action(const gcn::ActionEvent &event)
{
    if (event.getSource() == save_button.get()) {
        // Save the settings
        std::vector<std::string> mods;
        mods.reserve(entries.size());
        for (const auto &entry : entries) {
            if (entry.enabled) {
                mods.push_back(entry.name);
            }
        }
        knights_app.getModuleManager().setAndSaveLoadOrder(mods);

        // Go back to title screen
        std::unique_ptr<Screen> new_screen(new TitleScreen);
        knights_app.requestScreenChange(std::move(new_screen));

    } else if (event.getSource() == cancel_button.get()) {
        // Just go back to title screen without saving
        std::unique_ptr<Screen> new_screen(new TitleScreen);
        knights_app.requestScreenChange(std::move(new_screen));

    } else if (event.getSource() == refresh_button.get()) {
        refreshList();

    } else if (event.getSource() == up_button.get()) {
        moveSelected(-1);

    } else if (event.getSource() == down_button.get()) {
        moveSelected(+1);

#ifdef ONLINE_PLATFORM
    } else if (event.getSource() == upload_button.get()) {
        // TODO: go to mod uploading dialog

    } else if (event.getSource() == browse_button.get()) {
        knights_app.getOnlinePlatform().browseWorkshop();
#endif

    }
}

void ModsScreenImpl::mouseEntered(gcn::MouseEvent &e)
{
    if (e.getSource() == up_button.get()) {
        up_tooltip->scheduleShow();
    } else if (e.getSource() == down_button.get()) {
        down_tooltip->scheduleShow();
    } else if (e.getSource() == refresh_button.get()) {
        refresh_tooltip->scheduleShow();
#ifdef ONLINE_PLATFORM
    } else if (e.getSource() == upload_button.get()) {
        upload_tooltip->scheduleShow();
    } else if (e.getSource() == browse_button.get()) {
        browse_tooltip->scheduleShow();
#endif
    }
}

void ModsScreenImpl::mouseExited(gcn::MouseEvent &e)
{
    if (e.getSource() == up_button.get()) {
        up_tooltip->cancelShow();
    } else if (e.getSource() == down_button.get()) {
        down_tooltip->cancelShow();
    } else if (e.getSource() == refresh_button.get()) {
        refresh_tooltip->cancelShow();
#ifdef ONLINE_PLATFORM
    } else if (e.getSource() == upload_button.get()) {
        upload_tooltip->cancelShow();
    } else if (e.getSource() == browse_button.get()) {
        browse_tooltip->cancelShow();
#endif
    }
}


//
// ModsScreen
//

bool ModsScreen::start(KnightsApp &knights_app, boost::shared_ptr<Coercri::Window> w, gcn::Gui &gui)
{
    pimpl.reset(new ModsScreenImpl(knights_app, w, gui));
    return true;
}

void ModsScreen::update()
{
    pimpl->update();
}

/*
 * menu_screen.cpp
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
#include "config_map.hpp"
#include "game_manager.hpp"
#include "knights_app.hpp"
#include "knights_client.hpp"
#include "make_scroll_area.hpp"
#include "menu.hpp"
#include "menu_screen.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"
#include "gui_text_wrap.hpp"
#include "utf8_text_field.hpp"

#include "house_colour_font.hpp"

#include <memory>
#include <sstream>

namespace {
    class HouseColourListModel : public gcn::ListModel {
    public:
        explicit HouseColourListModel(const GameManager &gm) : game_manager(gm) { }
        std::string getElementAt(int i) {
            if (i < 0 || i >= game_manager.getNumAvailHouseColours()) return "";
            else return ColToText(game_manager.getAvailHouseColour(i)).asUTF8();
        }
        int getNumberOfElements() { return game_manager.getNumAvailHouseColours(); }
    private:
        const GameManager &game_manager;
    };
}

class MenuScreenImpl : public gcn::ActionListener, public gcn::SelectionListener, public gcn::MouseListener, public gcn::KeyListener {
public:
    MenuScreenImpl(boost::shared_ptr<KnightsClient> kc,
                   bool extended, bool can_invite,
                   KnightsApp &app,
                   boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui,
                   const UTF8String &saved_chat);
    ~MenuScreenImpl();
    void action(const gcn::ActionEvent &event);
    void valueChanged(const gcn::SelectionEvent &event);
    void mouseEntered(gcn::MouseEvent &event) override;
    void keyPressed(gcn::KeyEvent &event) override;
    void updateGui();
    void applyHelpVisibility();

    boost::shared_ptr<KnightsClient> knights_client;
    KnightsApp &knights_app;
    gcn::Gui &gui;
    boost::shared_ptr<Coercri::Window> window;

    std::unique_ptr<GuiCentre> centre;
    std::unique_ptr<GuiPanel> panel;
    std::unique_ptr<gcn::Container> container;
    std::unique_ptr<gcn::CheckBox> ready_checkbox;
    std::unique_ptr<GuiButton> start_button;
    std::unique_ptr<GuiButton> observe_button;
    std::unique_ptr<GuiButton> exit_button;
    std::unique_ptr<GuiButton> random_quest_button;
    std::unique_ptr<GuiButton> toggle_help_button;
    std::unique_ptr<gcn::Label> label, title, observer_label;
    std::unique_ptr<GuiTextWrap> quest_description_box;
    std::unique_ptr<gcn::ScrollArea> quest_description_scroll_area;
    std::unique_ptr<gcn::Label> setting_help_title;
    std::unique_ptr<GuiTextWrap> setting_help_box;
    std::unique_ptr<gcn::ScrollArea> setting_help_scroll_area;

    std::unique_ptr<gcn::Label> house_colour_label, team_label;
    std::unique_ptr<gcn::DropDown> house_colour_dropdown;
    std::unique_ptr<HouseColourListModel> house_colour_list_model;
    std::unique_ptr<GuiButton> join_button;
#ifdef ONLINE_PLATFORM
    std::unique_ptr<GuiButton> invite_button;
#endif
    std::unique_ptr<gcn::ListBox> players_listbox;
    std::unique_ptr<gcn::ScrollArea> players_scrollarea;
    std::unique_ptr<gcn::Label> players_title;
    std::unique_ptr<gcn::ListBox> chat_listbox;
    std::unique_ptr<gcn::ScrollArea> chat_scrollarea;
    std::unique_ptr<gcn::Label> chat_field_title;
    std::unique_ptr<UTF8TextField> chat_field;

    std::unique_ptr<HouseColourFont> house_colour_font;

    bool extended;
    bool can_invite;
    bool chat_reformatted;
    bool help_visible;
    int container_width_no_help;
    int container_width_with_help;
    int toggle_button_right_x;  // right edge of toggle button (fixed, does not change on toggle)
};

MenuScreenImpl::MenuScreenImpl(boost::shared_ptr<KnightsClient> kc,
                               bool extended, bool can_invite,
                               KnightsApp &app,
                               boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui_,
                               const UTF8String &saved_chat)
    : knights_client(kc), knights_app(app), gui(gui_), window(win),
      extended(extended), can_invite(can_invite),
      chat_reformatted(false), help_visible(false)
{
    const Localization &loc = app.getLocalization();

    container.reset(new gcn::Container);
    container->setOpaque(false);

    const int pad = 6, vpad_posttitle = 15, vpad_bot = 4;
    const int vpad_after_quest_box = 10;
    const int vpad_rhs = 5, width_rhs = extended ? 450 : 0;
    const int gutter = 20;
    const int help_window_width = 450;
    const int label_gap = 2;
    const int vpad_pretitle = vpad_bot;

    // Create title (don't add it yet)
    Coercri::UTF8String menu_title = app.getLocalization().get(app.getGameManager().getMenuTitle());
    title.reset(new gcn::Label(menu_title.asUTF8()));

    // Add menu widgets (col1)
    const int menu_y = vpad_pretitle + title->getHeight() + vpad_posttitle;
    int menu_width, y_after_menu;
    app.getGameManager().createMenuWidgets(this, this, this, pad, menu_y, *container.get(),
                                           menu_width, y_after_menu);

    // Random Quest button
    y_after_menu += 5;
    random_quest_button.reset(new GuiButton(loc.get(LocalKey("random_quest")).asUTF8()));
    random_quest_button->addActionListener(this);
    const int rqb_y = y_after_menu;  // y of random quest button row
    container->add(random_quest_button.get(), pad, rqb_y);
    y_after_menu += random_quest_button->getHeight();

    // Ensure there is at least enough width to show the Random Quest
    // and Show/Hide Help buttons.
    {
        // Measure the random quest button
        int w = random_quest_button->getWidth();

        // Measure the show and hide help buttons -- since
        // toggle_help_button isn't created yet, we can instead make
        // temporary GuiButton objects for the two possible states
        // (Show and Hide) and find their widths
        GuiButton b1(loc.get(LocalKey("show_help")).asUTF8());
        GuiButton b2(loc.get(LocalKey("hide_help")).asUTF8());
        w += std::max(b1.getWidth(), b2.getWidth());

        // Apply 'w' as a minimum width
        menu_width = std::max(menu_width, w);
    }

    // Add title (centered over col1)
    container->add(title.get(), pad + menu_width/2 - title->getWidth()/2, vpad_pretitle);

    // Toggle help button: right of Random Quest, right-aligned to the quest description box.
    toggle_help_button.reset(new GuiButton(loc.get(LocalKey("show_help")).asUTF8()));
    toggle_help_button->addActionListener(this);
    toggle_button_right_x = pad + menu_width;
    container->add(toggle_help_button.get(),
                   toggle_button_right_x - toggle_help_button->getWidth(),
                   rqb_y);

    // Quest description box (below random quest button)
    const int y_quest_scroll = y_after_menu + 5;
    const int font_h = title->getFont()->getHeight();
    const int quest_desc_h = 7 * font_h + 4;
    quest_description_box.reset(new GuiTextWrap);
    quest_description_box->setForegroundColor(gcn::Color(0,0,0));
    quest_description_box->setWidth(menu_width - DEFAULT_SCROLLBAR_WIDTH - 4);
    quest_description_box->adjustHeight();
    quest_description_scroll_area = MakeScrollArea(*quest_description_box, menu_width, quest_desc_h);
    container->add(quest_description_scroll_area.get(), pad, y_quest_scroll);
    const int y_after_col1 = y_quest_scroll + quest_desc_h;

    // Column x positions.
    // rhs_x is the left edge of second column. This is the help column
    //   when help is shown, or the players/chat column in extended mode
    //   when help is hidden. (It's not used at all in non-extended
    //   mode with help hidden.)
    // In extended mode, the players/chat column shifts right to col3_x
    //   when help is shown (the container expands accordingly).
    const int rhs_x  = pad + menu_width + gutter;
    const int col3_x  = rhs_x + help_window_width + gutter;

    // Lay out the players/chat column.
    // We place it at rhs_x; applyHelpVisibility() will move it
    // to the right if necessary.
    int rhs_y = 0;

    if (extended) {
        // create "house colour font"
        const int HOUSE_COL_WIDTH = 40, HOUSE_COL_HEIGHT = 20;
        house_colour_font.reset(new HouseColourFont(*title->getFont(), HOUSE_COL_WIDTH, HOUSE_COL_HEIGHT));

        // Add players area
        players_title.reset(new gcn::Label(loc.get(LocalKey("players")).asUTF8()));
        players_listbox.reset(new gcn::ListBox);
        players_listbox->setFont(house_colour_font.get());
        players_listbox->setListModel(&app.getGameManager().getGamePlayersList());
        players_scrollarea = MakeScrollArea(*players_listbox, width_rhs, 6 * players_listbox->getFont()->getHeight());
        AdjustListBoxSize(*players_listbox, *players_scrollarea);
        container->add(players_title.get(), rhs_x, menu_y - players_title->getHeight() - 2);
        container->add(players_scrollarea.get(), rhs_x, menu_y);
        rhs_y = menu_y + players_scrollarea->getHeight() + vpad_rhs*2;

        // Add house colour dropdown
        house_colour_label.reset(new gcn::Label(loc.get(LocalKey("change_house_colour")).asUTF8() + " "));
        container->add(house_colour_label.get(), rhs_x, rhs_y + 1);
        house_colour_list_model.reset(new HouseColourListModel(app.getGameManager()));
        house_colour_dropdown.reset(new gcn::DropDown);
        house_colour_dropdown->setFont(house_colour_font.get());
        house_colour_dropdown->setListModel(house_colour_list_model.get());
        house_colour_dropdown->setWidth(HOUSE_COL_WIDTH + HOUSE_COL_HEIGHT + 5);
        house_colour_dropdown->addActionListener(this);
        house_colour_dropdown->setSelectionColor(gcn::Color(255,255,255));
        container->add(house_colour_dropdown.get(), house_colour_label->getX() + house_colour_label->getWidth(), rhs_y);
        rhs_y += house_colour_dropdown->getHeight() + 1;

        // Add a small label
        team_label.reset(new gcn::Label(loc.get(LocalKey("knights_of_same_colour")).asUTF8()));
        container->add(team_label.get(), rhs_x, rhs_y);
        rhs_y += team_label->getHeight() + vpad_rhs;

        // Add chat field (at bottom, aligned with bottom of quest overview box)
        int ybelow = y_after_col1;

        chat_field.reset(new UTF8TextField);
        chat_field->adjustSize();
        chat_field->setWidth(width_rhs);
        chat_field->addActionListener(this);
        container->add(chat_field.get(), rhs_x, ybelow - chat_field->getHeight());
        if (!saved_chat.empty()) chat_field->setText(saved_chat.asUTF8());
        ybelow -= chat_field->getHeight();

        chat_field_title.reset(new gcn::Label(loc.get(LocalKey("type_here_to_chat")).asUTF8()));
        container->add(chat_field_title.get(), rhs_x, ybelow - chat_field_title->getHeight());
        ybelow -= chat_field_title->getHeight();
        ybelow -= (pad + vpad_rhs);

        // Add chat area (to take up remaining space)
        rhs_y += 12;
        chat_listbox.reset(new gcn::ListBox);
        app.getGameManager().getChatList().setGuiParams(chat_listbox->getFont(), width_rhs);
        chat_listbox->setListModel(&app.getGameManager().getChatList());
        chat_listbox->setWidth(width_rhs);
        chat_scrollarea = MakeScrollArea(*chat_listbox, width_rhs, std::max(10, ybelow - rhs_y));
        chat_scrollarea->setHorizontalScrollPolicy(gcn::ScrollArea::SHOW_NEVER);
        chat_scrollarea->setVerticalScrollPolicy(gcn::ScrollArea::SHOW_NEVER);
        container->add(chat_scrollarea.get(), rhs_x, rhs_y);
    }

    // Help area (label and scroll area).
    // Place at rhs_x, but initially hidden.
    setting_help_title.reset(new gcn::Label(loc.get(LocalKey("help")).asUTF8()));
    container->add(setting_help_title.get(), rhs_x, menu_y - setting_help_title->getHeight() - 2);
    setting_help_title->setVisible(false);

    const int help_area_height = std::max(10, y_after_col1 - menu_y);
    setting_help_box.reset(new GuiTextWrap);
    setting_help_box->setForegroundColor(gcn::Color(0,0,0));
    setting_help_box->setText(loc.get(LocalKey("hover_setting_help")));
    setting_help_box->setWidth(help_window_width - DEFAULT_SCROLLBAR_WIDTH - 4);
    setting_help_box->adjustHeight();
    setting_help_scroll_area = MakeScrollArea(*setting_help_box, help_window_width, help_area_height);
    container->add(setting_help_scroll_area.get(), rhs_x, menu_y);
    setting_help_scroll_area->setVisible(false);

    // Precompute container widths for the two help states.
    // Help hidden: Settings | Chat (extended), or Settings alone (non-extended).
    // Help shown:  Settings | Help | Chat (extended), or Settings | Help (non-extended).
    if (extended) {
        container_width_no_help   = rhs_x + pad + width_rhs;
        container_width_with_help = col3_x + pad + width_rhs;
    } else {
        container_width_no_help   = pad + menu_width + pad;
        container_width_with_help = rhs_x + pad + help_window_width;
    }

    // buttons_right: right edge of the chat column in the help-hidden state
    const int buttons_right = extended ? (rhs_x + width_rhs) : (pad + menu_width);

    // Add the start and exit buttons
    const int button_yofs = -3;
    int y = y_after_col1 + vpad_after_quest_box;
    exit_button.reset(new GuiButton(loc.get(LocalKey("exit")).asUTF8()));
    exit_button->addActionListener(this);

    // In extended view, set a min width for both the Observe and Exit
    // buttons (because they are next to each other and it looks odd
    // if one of them is really small)
    int min_exit_observe_width = extended ? exit_button->getFont()->getWidth("nnnnnn") : 0;
    if (exit_button->getWidth() < min_exit_observe_width) {
        exit_button->setWidth(min_exit_observe_width);
    }

    container->add(exit_button.get(), buttons_right - exit_button->getWidth(), y + button_yofs);
    if (extended) {
        const int exit_button_gap = 30;

        ready_checkbox.reset(new gcn::CheckBox(loc.get(LocalKey("ready_to_start")).asUTF8()));
        ready_checkbox->addActionListener(this);
        container->add(ready_checkbox.get(), pad, y + 2);

        int x = exit_button->getX() - exit_button_gap;

#ifdef ONLINE_PLATFORM
        if (can_invite) {
            invite_button.reset(new GuiButton(loc.get(LocalKey("invite_friend")).asUTF8()));
            invite_button->addActionListener(this);
            container->add(invite_button.get(), x - invite_button->getWidth(), y + button_yofs);
            x -= invite_button->getWidth() + exit_button_gap;
        }
#endif

        join_button.reset(new GuiButton(loc.get(LocalKey("join_game")).asUTF8()));
        join_button->addActionListener(this);
        container->add(join_button.get(), x - join_button->getWidth(), y + button_yofs);
        join_button->setVisible(false);

        observe_button.reset(new GuiButton(loc.get(LocalKey("observe")).asUTF8()));
        observe_button->addActionListener(this);
        if (observe_button->getWidth() < min_exit_observe_width) {
            observe_button->setWidth(min_exit_observe_width);
        }
        container->add(observe_button.get(), x - observe_button->getWidth(), y + button_yofs);

        observer_label.reset(new gcn::Label(loc.get(LocalKey("you_are_observing")).asUTF8()));
        container->add(observer_label.get(), pad, y + 2);
    } else {
        start_button.reset(new GuiButton(loc.get(LocalKey("start")).asUTF8()));
        start_button->addActionListener(this);
        container->add(start_button.get(), pad, y + button_yofs);
    }
    y += exit_button->getHeight();
    y += vpad_bot;

    // resize the container
    container->setSize(container_width_no_help, y);

    panel.reset(new GuiPanel(container.get()));
    centre.reset(new GuiCentre(panel.get()));
    gui.setTop(centre.get());
    gui.addGlobalKeyListener(this);

    // make sure everything is up to date
    updateGui();

    if (chat_field && !saved_chat.empty()) {
        chat_field->requestFocus();
        chat_field->setCaretPosition(saved_chat.asUTF8().length());
    }
}

MenuScreenImpl::~MenuScreenImpl()
{
    // get rid of the menu widgets at this point.
    container.reset();
    knights_app.getGameManager().destroyMenuWidgets();
}

void MenuScreenImpl::keyPressed(gcn::KeyEvent &event)
{
    if (event.getKey().getValue() == gcn::Key::F1) {
        help_visible = !help_visible;
        applyHelpVisibility();
        event.consume();
    }
}

void MenuScreenImpl::applyHelpVisibility()
{
    const Localization &loc = knights_app.getLocalization();
    setting_help_scroll_area->setVisible(help_visible);
    setting_help_title->setVisible(help_visible);

    toggle_help_button->setCaption(
        loc.get(LocalKey(help_visible ? "hide_help" : "show_help")).asUTF8());
    toggle_help_button->adjustSize();

    // Toggle button stays right-aligned to toggle_button_right_x (fixed position).
    toggle_help_button->setX(toggle_button_right_x - toggle_help_button->getWidth());

    // Reset help text to the default prompt whenever the panel is opened.
    if (help_visible) {
        setting_help_box->setText(loc.get(LocalKey("hover_setting_help")));
        setting_help_box->adjustHeight();
    }

    const int new_width = help_visible ? container_width_with_help : container_width_no_help;

    // dx: amount all right-side elements need to shift.
    const int dx = new_width - container->getWidth();

    // Shift right-anchored buttons so they remain at the right edge of the panel.
    if (exit_button)    exit_button->setX(exit_button->getX() + dx);
    if (observe_button) observe_button->setX(observe_button->getX() + dx);
    if (join_button)    join_button->setX(join_button->getX() + dx);
#ifdef ONLINE_PLATFORM
    if (invite_button)  invite_button->setX(invite_button->getX() + dx);
#endif

    // In extended mode, shift the chat column widgets (they live at rhs_x when
    // help is hidden, and shift right to col3_x when help is shown).
    if (extended) {
        if (players_title)         players_title->setX(players_title->getX() + dx);
        if (players_scrollarea)    players_scrollarea->setX(players_scrollarea->getX() + dx);
        if (house_colour_label)    house_colour_label->setX(house_colour_label->getX() + dx);
        if (house_colour_dropdown) house_colour_dropdown->setX(house_colour_dropdown->getX() + dx);
        if (team_label)            team_label->setX(team_label->getX() + dx);
        if (chat_scrollarea)       chat_scrollarea->setX(chat_scrollarea->getX() + dx);
        if (chat_field_title)      chat_field_title->setX(chat_field_title->getX() + dx);
        if (chat_field)            chat_field->setX(chat_field->getX() + dx);
    }

    container->setSize(new_width, container->getHeight());
    // GuiPanel does not auto-resize when its child changes size; update it manually.
    // panel border = 2 * DEFAULT_BORDER = 2 * 3 = 6 (see gui_panel.cpp)
    panel->setSize(new_width + 6, container->getHeight() + 6);
    // Re-centre: mirror what cg_listener does on a real window resize — set GuiCentre
    // to the full window size, which fires widgetResized and re-computes the centering.
    int win_w, win_h;
    window->getSize(win_w, win_h);
    centre->setSize(win_w, win_h);
    window->invalidateAll();
}

void MenuScreenImpl::mouseEntered(gcn::MouseEvent &event)
{
    const auto help_text = knights_app.getGameManager().getMenuItemHelpText(event.getSource());
    if (help_text.has_value()) {
        const Localization &loc = knights_app.getLocalization();
        const UTF8String &text = help_text->empty()
            ? loc.get(LocalKey("hover_setting_help"))
            : *help_text;
        setting_help_box->setText(text);
        setting_help_box->adjustHeight();
        window->invalidateAll();
    }
}

void MenuScreenImpl::action(const gcn::ActionEvent &event)
{
    if (event.getSource() == exit_button.get()) {
        knights_client->leaveGame();
        return;
    }

    if (event.getSource() == ready_checkbox.get()) {
        knights_client->setReady(ready_checkbox->isSelected());
        return;
    }

    if (event.getSource() == start_button.get()) {
        knights_client->setReady(true);
        return;
    }

    if (event.getSource() == random_quest_button.get()) {
        knights_client->randomQuest();
        return;
    }

    if (event.getSource() == toggle_help_button.get()) {
        help_visible = !help_visible;
        applyHelpVisibility();
        return;
    }

    if (event.getSource() == chat_field.get() && !chat_field->getText().empty()) {
        knights_client->sendChatMessage(Coercri::UTF8String::fromUTF8Safe(chat_field->getText()));
        chat_field->setText("");
        gui.logic();
        window->invalidateAll();
        return;
    }

    if (event.getSource() == house_colour_dropdown.get()) {
        const int sel = house_colour_dropdown->getSelected();
        knights_client->setHouseColour(sel);
        return;
    }

    if (event.getSource() == join_button.get()) {
        knights_client->setObsFlag(false);
        return;
    }

#ifdef ONLINE_PLATFORM
    if (event.getSource() == invite_button.get()) {
        knights_app.inviteFriendToLobby();
        return;
    }
#endif

    if (event.getSource() == observe_button.get()) {
        knights_client->setObsFlag(true);
        return;
    }

    // Must have been one of the menu widgets
    int item_num;
    int choice_num;
    const bool found = knights_app.getGameManager().getMenuWidgetInfo(event.getSource(), item_num, choice_num);
    if (found) {
        knights_client->setMenuSelection(item_num, choice_num);
    }
}

void MenuScreenImpl::valueChanged(const gcn::SelectionEvent &event)
{
    if (knights_app.getGameManager().doingMenuWidgetUpdate()) return;  // prevent infinite loops -- Trac #72.

    int item_num;
    int choice_num;
    const bool found = knights_app.getGameManager().getMenuWidgetInfo(event.getSource(), item_num, choice_num);
    if (found) {
        knights_client->setMenuSelection(item_num, choice_num);
    }
}

void MenuScreenImpl::updateGui()
{
    GameManager &game_manager = knights_app.getGameManager();

    const bool i_am_observer = game_manager.getMyObsFlag();

    if (chat_listbox) {

        chat_listbox->adjustSize();

        if (!chat_reformatted && chat_listbox->getHeight() > chat_scrollarea->getHeight()) {
            chat_listbox->setWidth(chat_listbox->getWidth() - DEFAULT_SCROLLBAR_WIDTH);
            chat_scrollarea->setVerticalScrollPolicy(gcn::ScrollArea::SHOW_ALWAYS);
            game_manager.getChatList().setGuiParams(chat_listbox->getFont(), chat_listbox->getWidth());
            chat_reformatted = true;
        }
    }

    quest_description_box->setText(game_manager.getQuestDescription());
    quest_description_box->adjustHeight();

    if (players_listbox) AdjustListBoxSize(*players_listbox, *players_scrollarea);

    game_manager.setMenuWidgetsEnabled(!i_am_observer);
    if (ready_checkbox) ready_checkbox->setVisible(!i_am_observer);
    if (house_colour_dropdown) house_colour_dropdown->setSelected(game_manager.getMyHouseColour());
    if (join_button) join_button->setVisible(i_am_observer);
    if (observe_button) observe_button->setVisible(!i_am_observer);
    if (observer_label) observer_label->setVisible(i_am_observer);
    if (random_quest_button) random_quest_button->setEnabled(!i_am_observer);

    // if I am observer then make sure 'ready to start' is always unchecked
    // otherwise, set it from the game manager.
    if (ready_checkbox) {
        if (i_am_observer) ready_checkbox->setSelected(false);
        else ready_checkbox->setSelected(game_manager.getMyReadyFlag());
    }

    // work around guichan bug when buttons are made visible/invisible...
    knights_app.repeatLastMouseInput();

    gui.logic();
    window->invalidateAll();
}

MenuScreen::MenuScreen(boost::shared_ptr<KnightsClient> kc,
                       bool extended, bool can_invite,
                       UTF8String saved_chat)
    : knights_client(kc), extended(extended), can_invite(can_invite), saved_chat(saved_chat)
{ }

bool MenuScreen::start(KnightsApp &app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui)
{
    pimpl.reset(new MenuScreenImpl(knights_client, extended, can_invite, app, win, gui, saved_chat));
    saved_chat = UTF8String();
    return true;
}

void MenuScreen::update()
{
    if (pimpl->knights_app.getGameManager().isGuiInvalid()) {
        pimpl->updateGui();
    }

    if (extended && pimpl->knights_app.getGameManager().getChatList().isUpdated()) {
        // auto scroll chat window to bottom
        gcn::Rectangle rect(0, pimpl->chat_listbox->getHeight() - pimpl->chat_listbox->getFont()->getHeight(),
                            1, pimpl->chat_listbox->getFont()->getHeight());
        pimpl->chat_scrollarea->showWidgetPart(pimpl->chat_listbox.get(), rect);
    }

    // Save our chat field contents into GameManager (so that it can be preserved across
    // host migrations if applicable)
    if (extended && !pimpl->knights_app.screenChangePending()) {
        pimpl->knights_app.getGameManager().setSavedChat(Coercri::UTF8String::fromUTF8Safe(pimpl->chat_field->getText()));
    }
}

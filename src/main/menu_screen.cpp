/*
 * menu_screen.cpp
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

#include "house_colour_font.hpp"

#include "boost/scoped_ptr.hpp"
#include <sstream>

namespace {
    class HouseColourListModel : public gcn::ListModel {
    public:
        explicit HouseColourListModel(const GameManager &gm) : game_manager(gm) { }
        std::string getElementAt(int i) {
            if (i < 0 || i >= game_manager.getNumAvailHouseColours()) return "";
            else return ColToText(game_manager.getAvailHouseColour(i));
        }
        int getNumberOfElements() { return game_manager.getNumAvailHouseColours(); }
    private:
        const GameManager &game_manager;
    };
}

class MenuScreenImpl : public gcn::ActionListener, public gcn::SelectionListener {
public:
    MenuScreenImpl(boost::shared_ptr<KnightsClient> kc, bool extended_, KnightsApp &app, 
                   boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui,
                   const std::string &saved_chat);
    ~MenuScreenImpl();
    void action(const gcn::ActionEvent &event);
    void valueChanged(const gcn::SelectionEvent &event);
    void updateGui();
    
    boost::shared_ptr<KnightsClient> knights_client;
    KnightsApp &knights_app;
    gcn::Gui &gui;
    boost::shared_ptr<Coercri::Window> window;

    boost::scoped_ptr<GuiCentre> centre;
    boost::scoped_ptr<GuiPanel> panel;
    boost::scoped_ptr<gcn::Container> container;
    boost::scoped_ptr<gcn::CheckBox> ready_checkbox;
    boost::scoped_ptr<GuiButton> start_button;
    boost::scoped_ptr<GuiButton> observe_button;
    boost::scoped_ptr<GuiButton> exit_button;
    boost::scoped_ptr<GuiButton> random_quest_button;
    boost::scoped_ptr<gcn::Label> label, title, observer_label;
    boost::scoped_ptr<GuiTextWrap> quest_description_box;
    std::auto_ptr<gcn::ScrollArea> quest_description_scroll_area;
    
    boost::scoped_ptr<gcn::Label> house_colour_label, team_label;
    boost::scoped_ptr<gcn::DropDown> house_colour_dropdown;
    boost::scoped_ptr<HouseColourListModel> house_colour_list_model;
    boost::scoped_ptr<GuiButton> join_button;
    boost::scoped_ptr<gcn::ListBox> players_listbox;
    std::auto_ptr<gcn::ScrollArea> players_scrollarea;
    boost::scoped_ptr<gcn::Label> players_title;
    boost::scoped_ptr<gcn::ListBox> chat_listbox;
    std::auto_ptr<gcn::ScrollArea> chat_scrollarea;
    boost::scoped_ptr<gcn::Label> chat_field_title;
    boost::scoped_ptr<gcn::TextField> chat_field;

    boost::scoped_ptr<HouseColourFont> house_colour_font;
    
    bool extended;
    bool chat_reformatted;
};

MenuScreenImpl::MenuScreenImpl(boost::shared_ptr<KnightsClient> kc, bool extended_, KnightsApp &app, 
                               boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui_,
                               const std::string &saved_chat)
    : knights_client(kc), knights_app(app), gui(gui_), window(win), extended(extended_),
      chat_reformatted(false)
{
    // create container
    container.reset(new gcn::Container);
    container->setOpaque(false);
    
    const int pad = 6, vpad_pretitle = 6, vpad_posttitle = 15, vpad_mid = 15, vpad_bot = 4;
    const int vpad_before_quest_box = 18, vpad_after_quest_box = 10;
    const int vpad_rhs = 5, width_rhs = extended ? 450 : 0;
    const int gutter = 15;
    
    // Create title (don't add it yet)
    title.reset(new gcn::Label(app.getGameManager().getMenuTitle()));

    // Add menu widgets
    const int menu_y = vpad_pretitle + title->getHeight() + vpad_posttitle;
    int menu_width, y_after_menu;
    app.getGameManager().createMenuWidgets(this, this, pad, menu_y, *container.get(),
                                           menu_width, y_after_menu);

    // random quest button
    y_after_menu += 5;
    random_quest_button.reset(new GuiButton("Random Quest"));
    random_quest_button->addActionListener(this);
    container->add(random_quest_button.get(), pad, y_after_menu);
    y_after_menu += random_quest_button->getHeight();

    // ensure there is at least enough width to show the Random Quest button.
    menu_width = std::max(menu_width, random_quest_button->getWidth());
    
    // quest description box
    quest_description_box.reset(new GuiTextWrap);
    quest_description_box->setForegroundColor(gcn::Color(0,0,0));
    quest_description_box->setWidth(menu_width - DEFAULT_SCROLLBAR_WIDTH - 4);
    quest_description_box->adjustHeight();
    quest_description_scroll_area = MakeScrollArea(*quest_description_box, menu_width, title->getFont()->getHeight() * 7);
    y_after_menu += vpad_before_quest_box;
    container->add(quest_description_scroll_area.get(), pad, y_after_menu);
    y_after_menu += quest_description_scroll_area->getHeight();

    // Add title
    container->add(title.get(), pad + menu_width/2 - title->getWidth()/2, vpad_pretitle);

    const int rhs_x = pad + menu_width + (extended ? gutter : 0);
    int rhs_y = vpad_pretitle;

    if (extended) {
        // create "house colour font"
        const int HOUSE_COL_WIDTH = 40, HOUSE_COL_HEIGHT = 20;
        house_colour_font.reset(new HouseColourFont(*title->getFont(), HOUSE_COL_WIDTH, HOUSE_COL_HEIGHT));
        
        // Add players area
        players_title.reset(new gcn::Label("Players"));
        container->add(players_title.get(), rhs_x, rhs_y);
        rhs_y += players_title->getHeight() + 4;
        players_listbox.reset(new gcn::ListBox);
        players_listbox->setFont(house_colour_font.get());
        players_listbox->setListModel(&app.getGameManager().getGamePlayersList());
        players_scrollarea = MakeScrollArea(*players_listbox, width_rhs, 6 * players_listbox->getFont()->getHeight());
        AdjustListBoxSize(*players_listbox, *players_scrollarea);
        container->add(players_scrollarea.get(), rhs_x, rhs_y);
        rhs_y += players_scrollarea->getHeight() + vpad_rhs*2;

        // Add house colour dropdown
        house_colour_label.reset(new gcn::Label("Change Knight House Colour: "));
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
        team_label.reset(new gcn::Label("(Knights of the same House Colour play on the same team.)"));
        container->add(team_label.get(), rhs_x, rhs_y);
        rhs_y += team_label->getHeight() + vpad_rhs;
        
        // Add chat field (at bottom)
        int ybelow = y_after_menu;

        chat_field.reset(new gcn::TextField);
        chat_field->adjustSize();
        chat_field->setWidth(width_rhs);
        chat_field->addActionListener(this);
        container->add(chat_field.get(), rhs_x, ybelow - chat_field->getHeight());
        if (!saved_chat.empty()) chat_field->setText(saved_chat);
        ybelow -= chat_field->getHeight();

        chat_field_title.reset(new gcn::Label("Type here to chat"));
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

    const int container_width = rhs_x + width_rhs + pad;
    
    // Add the start and exit buttons
    const int button_yofs = -3;
    int y = y_after_menu + vpad_after_quest_box;
    if (extended) {
        // (this looks a little better with some extra spacing, as its next to the "Observe" button which is fairly big...)
        exit_button.reset(new GuiButton("  Exit  "));
    } else {
        exit_button.reset(new GuiButton("Exit"));
    }
    exit_button->addActionListener(this);
    container->add(exit_button.get(), container_width - pad - exit_button->getWidth(), y + button_yofs);
    if (extended) {
        const int exit_button_gap = 30;
        
        ready_checkbox.reset(new gcn::CheckBox("Ready to Start"));
        ready_checkbox->addActionListener(this);
        container->add(ready_checkbox.get(), pad, y + 2);
        join_button.reset(new GuiButton("Join Game"));
        join_button->addActionListener(this);
        container->add(join_button.get(), exit_button->getX() - join_button->getWidth() - exit_button_gap, y + button_yofs);
        join_button->setVisible(false);
        observe_button.reset(new GuiButton("Observe"));
        observe_button->addActionListener(this);
        container->add(observe_button.get(), exit_button->getX() - observe_button->getWidth() - exit_button_gap, y + button_yofs);

        observer_label.reset(new gcn::Label("You are observing this game. Click \"Join Game\" if you wish to play."));
        container->add(observer_label.get(), pad, y + 2);
    } else {
        start_button.reset(new GuiButton("Start"));
        start_button->addActionListener(this);
        container->add(start_button.get(), pad, y + button_yofs);
    }
    y += exit_button->getHeight();
    y += vpad_bot;
    
    // resize the container
    container->setSize(container_width, y);

    panel.reset(new GuiPanel(container.get()));
    centre.reset(new GuiCentre(panel.get()));
    gui.setTop(centre.get());

    // make sure everything is up to date
    updateGui();

    if (chat_field && !saved_chat.empty()) {
        chat_field->requestFocus();
        chat_field->setCaretPosition(saved_chat.length());
    }
}

MenuScreenImpl::~MenuScreenImpl()
{
    // get rid of the menu widgets at this point.
    container.reset();
    knights_app.getGameManager().destroyMenuWidgets();
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

    if (event.getSource() == chat_field.get() && !chat_field->getText().empty()) {
        knights_client->sendChatMessage(chat_field->getText());
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

MenuScreen::MenuScreen(boost::shared_ptr<KnightsClient> kc, bool extended_, std::string saved_chat_)
    : knights_client(kc), extended(extended_), saved_chat(saved_chat_)
{ }

bool MenuScreen::start(KnightsApp &app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui)
{
    pimpl.reset(new MenuScreenImpl(knights_client, extended, app, win, gui, saved_chat));
    saved_chat.clear();
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
}

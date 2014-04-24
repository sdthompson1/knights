/*
 * lobby_screen.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2013.
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
#include "client_callbacks.hpp"
#include "error_screen.hpp"
#include "game_info.hpp"
#include "game_manager.hpp"
#include "knights_app.hpp"
#include "knights_client.hpp"
#include "lobby_screen.hpp"
#include "make_scroll_area.hpp"
#include "tab_font.hpp"
#include "title_screen.hpp"
#include "utf8string.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"
#include "title_block.hpp"

#include "gcn/cg_font.hpp" // coercri

#include "boost/scoped_ptr.hpp"

#include <sstream>
#include <cstring>
#include <numeric>

namespace {

    //
    // GameInfoToString
    //
    
    std::string GameInfoToString(const GameInfo &gi)
    {
        std::ostringstream str;

        str << gi.game_name << "\t"
            << gi.num_players << "\t"
            << gi.num_observers << "\t";

        switch (gi.status) {
        case GS_RUNNING:
            str << "Running";
            break;
        case GS_SELECTING_QUEST:
            str << "Selecting Quest";
            break;
        case GS_WAITING_FOR_PLAYERS:
            if (gi.num_players == 0) {
                str << "Empty";
            } else {
                str << "Waiting for 1 player";
            }
            break;
        }

        return str.str();
    }
    

    //
    // GameList
    //

    class GameList : public gcn::ListModel {
    public:
        virtual int getNumberOfElements()
        {
            return int(game_infos.size());
        }
        
        virtual std::string getElementAt(int i)
        {
            if (i < 0 || i >= int(game_infos.size())) {
                return std::string();
            } else {
                return GameInfoToString(game_infos[i]);
            }
        }

        std::vector<GameInfo> game_infos;
    };
}


class LobbyScreenImpl : public gcn::ActionListener {
public:
    explicit LobbyScreenImpl(boost::shared_ptr<KnightsClient> cli, const std::string &svr_name)
        : knights_app(0), gui(0), client(cli), game_list_init_yet(false), server_name(svr_name) { }
    void gotoJoiningDialog(const std::string &msg);
    void start(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g);
    void action(const gcn::ActionEvent &event);

    KnightsApp *knights_app;
    boost::shared_ptr<Coercri::Window> window;
    gcn::Gui *gui;

    boost::shared_ptr<KnightsClient> client;
    GameList game_list;
    bool game_list_init_yet;
    
    boost::scoped_ptr<GuiCentre> centre;
    boost::scoped_ptr<GuiPanel> panel;
    boost::scoped_ptr<gcn::Container> container;
    boost::scoped_ptr<gcn::Label> games_label, players_label, chat_label;
    boost::scoped_ptr<gcn::Button> disconnect_button;
    boost::scoped_ptr<gcn::ListBox> games_listbox, players_listbox, chat_listbox;
    boost::scoped_ptr<TitleBlock> games_titleblock;
    std::auto_ptr<gcn::ScrollArea> games_scrollarea, players_scrollarea, chat_scrollarea;
    boost::scoped_ptr<gcn::TextField> chat_field;

    boost::scoped_ptr<GuiCentre> join_centre;
    boost::scoped_ptr<GuiPanel> join_panel;
    boost::scoped_ptr<gcn::Container> join_container;
    boost::scoped_ptr<gcn::Label> join_label;
    boost::scoped_ptr<gcn::Button> join_disconn_button;

    boost::shared_ptr<gcn::Font> cg_font;
    boost::scoped_ptr<gcn::Font> tab_font;
    boost::scoped_ptr<gcn::Label> title_label;
    std::string server_name;
};


namespace {
    //
    // MyListBox
    // Reimplements ListBox::mouseClicked to check for double clicks, and
    // sends out a join game request if one is received.
    //

    class MyListBox : public gcn::ListBox {
    public:
        MyListBox(KnightsApp &ka, const GameList &games, LobbyScreenImpl &lsi_)
            : knights_app(ka), game_list(games), lsi(lsi_) { }
        
        void mouseClicked(gcn::MouseEvent &mouse_event)
        {
            gcn::ListBox::mouseClicked(mouse_event);  // make sure guichan's mouseClicked gets called
            
            if (mouse_event.getButton() == gcn::MouseEvent::LEFT && mouse_event.getClickCount() == 2) {
                // Find the game name
                const int sel = getSelected();
                if (sel >= 0 && sel < game_list.game_infos.size()) {
                    // Send out the join request
                    knights_app.getGameManager().tryJoinGame(game_list.game_infos[sel].game_name);

                    // Also make the gui go into "joining..." mode
                    lsi.gotoJoiningDialog("Joining " + game_list.game_infos[sel].game_name + "...");
                }
            }
        }
        
    private:
        KnightsApp &knights_app;
        const GameList &game_list;
        LobbyScreenImpl &lsi;
    };
}




void LobbyScreenImpl::start(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g)
{
    knights_app = &ka;
    window = win;
    gui = &g;

    
    // set up the gui

    const char * plyr_string = "Players";
    const char * obs_string = "Observers";
    const int plyr_width = knights_app->getFont()->getTextWidth(UTF8String::fromUTF8(plyr_string));
    const int obs_width = knights_app->getFont()->getTextWidth(UTF8String::fromUTF8(obs_string));
    const int status_width = std::max(knights_app->getFont()->getTextWidth(UTF8String::fromUTF8("Waiting for 2 players")),
                                      knights_app->getFont()->getTextWidth(UTF8String::fromUTF8("Selecting Quest")));
    
    std::vector<int> widths;
    widths.push_back(150);
    widths.push_back(plyr_width + 12);
    widths.push_back(obs_width + 12);
    widths.push_back(status_width + 6);
    
    container.reset(new gcn::Container);
    container->setOpaque(false);

    const int pad = 10;
    const int gutter = 20;
    int y = pad;

    cg_font.reset(new Coercri::CGFont(knights_app->getFont()));
    tab_font.reset(new TabFont(cg_font, widths));
    
    title_label.reset(new gcn::Label("Connected to: " + server_name));
    title_label->setForegroundColor(gcn::Color(0,0,128));
        
    games_label.reset(new gcn::Label(std::string("Games (double click to join)")));
    players_label.reset(new gcn::Label(std::string("Players in Lobby")));
    
    const int col_1_width = std::accumulate(widths.begin(), widths.end(), 0);
    const int col_2_width = 250;
    const int rhs_x = pad + col_1_width + gutter;
    const int width = col_1_width + col_2_width + gutter;

    const int games_height = 160;  // includes "titleblock"
    const int chat_height = 320;

    container->add(title_label.get(), width/2 - title_label->getWidth()/2, y);
    y += title_label->getHeight() + pad;
                   
    container->add(games_label.get(), pad, y);
    container->add(players_label.get(), rhs_x, y);
    y += games_label->getHeight() + pad;

    players_listbox.reset(new gcn::ListBox);
    players_listbox->setListModel(&ka.getGameManager().getLobbyPlayersList());
    players_scrollarea = MakeScrollArea(*players_listbox, col_2_width, games_height);
    AdjustListBoxSize(*players_listbox, *players_scrollarea);
    container->add(players_scrollarea.get(), rhs_x, y);
    
    std::vector<std::string> titles;
    titles.push_back("Game");
    titles.push_back(plyr_string);
    titles.push_back(obs_string);
    titles.push_back("Status");
    games_titleblock.reset(new TitleBlock(titles, widths));
    games_titleblock->setBaseColor(gcn::Color(180, 180, 180));
    container->add(games_titleblock.get(), pad, y);
    y += games_titleblock->getHeight();
    
    games_listbox.reset(new MyListBox(*knights_app, game_list, *this));
    games_listbox->setListModel(&game_list);
    games_listbox->addActionListener(this);
    games_listbox->setFont(tab_font.get());
    games_scrollarea = MakeScrollArea(*games_listbox, col_1_width, games_height - games_titleblock->getHeight());
    games_scrollarea->setHorizontalScrollPolicy(gcn::ScrollArea::SHOW_NEVER);
    AdjustListBoxSize(*games_listbox, *games_scrollarea);
    container->add(games_scrollarea.get(), pad, y);
    
    y += games_scrollarea->getHeight() + pad*2;

    chat_listbox.reset(new gcn::ListBox);
    ka.getGameManager().getChatList().setGuiParams(chat_listbox->getFont(), width);
    chat_listbox->setListModel(&ka.getGameManager().getChatList());
    chat_listbox->setWidth(width);
    chat_scrollarea = MakeScrollArea(*chat_listbox, width, chat_height);
    chat_scrollarea->setHorizontalScrollPolicy(gcn::ScrollArea::SHOW_NEVER);
    container->add(chat_scrollarea.get(), pad, y);
    y += chat_scrollarea->getHeight() + pad*2;
    
    chat_label.reset(new gcn::Label("Type here to chat"));
    container->add(chat_label.get(), pad, y);
    y += chat_label->getHeight() + pad;

    chat_field.reset(new gcn::TextField);
    chat_field->adjustSize();
    chat_field->setWidth(width);
    chat_field->addActionListener(this);
    container->add(chat_field.get(), pad, y);
    y += chat_field->getHeight() + pad*2;
    
    disconnect_button.reset(new GuiButton("Disconnect"));
    disconnect_button->addActionListener(this);
    container->add(disconnect_button.get(), width - disconnect_button->getWidth() + pad, y);

    container->setSize(2*pad + width, y + disconnect_button->getHeight() + pad);

    panel.reset(new GuiPanel(container.get()));
    centre.reset(new GuiCentre(panel.get()));
    gui->setTop(centre.get());
}

void LobbyScreenImpl::gotoJoiningDialog(const std::string &msg)
{
    join_container.reset(new gcn::Container);
    join_container->setOpaque(false);
    const int pad = 10;
    int y = pad;

    join_label.reset(new gcn::Label(msg));
    join_container->add(join_label.get(), pad, y);
    y += join_label->getHeight() + pad;

    join_disconn_button.reset(new GuiButton("Disconnect"));
    join_disconn_button->addActionListener(this);
    join_container->add(join_disconn_button.get(), pad + join_label->getWidth()/2 - join_disconn_button->getWidth()/2, y);
    y += join_disconn_button->getHeight() + pad;

    join_container->setSize(2*pad + join_label->getWidth(), y);
    join_panel.reset(new GuiPanel(join_container.get()));
    join_centre.reset(new GuiCentre(join_panel.get()));

    int w,h;
    window->getSize(w,h);
    join_centre->setSize(w,h);
    gui->setTop(join_centre.get());
    gui->logic();
    window->invalidateAll();
}

void LobbyScreenImpl::action(const gcn::ActionEvent &event)
{
    if (event.getSource() == disconnect_button.get() || event.getSource() == join_disconn_button.get()) {
        // Go back to title screen
        std::auto_ptr<Screen> title_screen(new TitleScreen);
        knights_app->requestScreenChange(title_screen);
    } else if (event.getSource() == chat_field.get()) {
        client->sendChatMessage(chat_field->getText());
        chat_field->setText("");
        gui->logic();
        window->invalidateAll();
    }
}

LobbyScreen::LobbyScreen(boost::shared_ptr<KnightsClient> client, const std::string &svr_name)
    : pimpl(new LobbyScreenImpl(client, svr_name))
{ }

LobbyScreen::~LobbyScreen()
{
    if (pimpl && pimpl->knights_app) {
        pimpl->knights_app->getGameManager().getChatList().setGuiParams((gcn::Font*)0, 999);
    }
}

bool LobbyScreen::start(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui)
{
    pimpl->start(ka, win, gui);
    return true;
}

void LobbyScreen::update()
{
    const int ask_interval = 2000;
    const int reply_timeout = 15000;

    if (!pimpl->knights_app) return;

    if (pimpl->knights_app->getGameManager().isGameListUpdated() || !pimpl->game_list_init_yet) {
        pimpl->game_list.game_infos = pimpl->knights_app->getGameManager().getGameInfos();
        pimpl->game_list_init_yet = true;
        pimpl->gui->logic();
        pimpl->window->invalidateAll();
    }

    if (pimpl->knights_app->getGameManager().isGuiInvalid()) {

        // if join-game failed then go back to main gui screen.
        if (pimpl->join_centre && pimpl->knights_app->getGameManager().getCurrentGameName().empty()) {
            int w,h;
            pimpl->window->getSize(w,h);
            pimpl->centre->setSize(w,h);
            pimpl->gui->setTop(pimpl->centre.get());
            pimpl->gui->logic();
            pimpl->join_centre.reset();
            pimpl->join_panel.reset();
            pimpl->join_container.reset();
            pimpl->join_label.reset();
            pimpl->join_disconn_button.reset();
        }
        
        // resize the game and player listboxes as needed
        AdjustListBoxSize(*pimpl->games_listbox, *pimpl->games_scrollarea);
        AdjustListBoxSize(*pimpl->players_listbox, *pimpl->players_scrollarea);
        
        pimpl->gui->logic();

        if (pimpl->knights_app->getGameManager().getChatList().isUpdated()) {
            // auto scroll chat window to bottom.
            gcn::Rectangle rect(0, pimpl->chat_listbox->getHeight() - pimpl->chat_listbox->getFont()->getHeight(),
                                1, pimpl->chat_listbox->getFont()->getHeight());
            pimpl->chat_scrollarea->showWidgetPart(pimpl->chat_listbox.get(), rect);
        }
        
        pimpl->window->invalidateAll();
    }
}

unsigned int LobbyScreen::getUpdateInterval()
{
    return 100;
}

/*
 * online_multiplayer_screen.cpp
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

#ifdef ONLINE_PLATFORM

#include "misc.hpp"

#include "connecting_screen.hpp"
#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"
#include "knights_app.hpp"
#include "loading_screen.hpp"
#include "make_scroll_area.hpp"
#include "online_multiplayer_screen.hpp"
#include "online_platform.hpp"
#include "start_game_screen.hpp"
#include "tab_font.hpp"
#include "title_block.hpp"

#include "boost/scoped_ptr.hpp"
#include "gcn/cg_font.hpp"
#include <vector>
#include <string>
#include <numeric>

namespace {
    struct GameInfo {
        GameInfo(const std::string &lobby_id, const Coercri::UTF8String &leader_name, int players, int status)
            : lobby_id(lobby_id), leader_name(leader_name), num_players(players), status_code(status) { }
        std::string lobby_id;
        Coercri::UTF8String leader_name;
        int num_players;
        int status_code;
    };

    class GameList : public gcn::ListModel {
    public:
        explicit GameList(OnlinePlatform &platform, KnightsApp &app);
        virtual int getNumberOfElements();
        virtual std::string getElementAt(int i);
        const GameInfo * getGameAt(int i) const;

    private:
        std::vector<GameInfo> games;
        KnightsApp &knights_app;
    };

    GameList::GameList(OnlinePlatform &platform, KnightsApp &app)
        : knights_app(app)
    {
        // This constructor queries the OnlinePlatform for the latest lobby
        // list, and creates a vector of GameInfos accordingly.
        std::vector<std::string> lobbies = platform.getLobbyList();
        for (const auto & lobby_id : lobbies) {
            OnlinePlatform::LobbyInfo info = platform.getLobbyInfo(lobby_id);
            Coercri::UTF8String leader_name = platform.lookupUserName(info.leader_id);
            games.push_back(GameInfo(lobby_id, leader_name, info.num_players, info.status_code));
        }
    }

    int GameList::getNumberOfElements()
    {
        return int(games.size());
    }

    std::string GameList::getElementAt(int i)
    {
        if (i >= 0 && i < int(games.size())) {
            const GameInfo &game = games[i];
            std::string result = game.leader_name.asLatin1();
            result += "\t";
            result += std::to_string(game.num_players);
            result += "\t";

            Coercri::UTF8String status_msg;
            if (game.status_code == 0) {
                // Localization String 1 = "Selecting Quest"
                status_msg = knights_app.getLocalizationString(1);
            } else {
                // Localization String 2 = "Playing (%1)"
                status_msg = knights_app.getLocalizationString(2, knights_app.getLocalizationString(game.status_code));
            }
            result += status_msg.asLatin1();

            return result;
        }
        return std::string();
    }

    const GameInfo * GameList::getGameAt(int i) const
    {
        if (i >= 0 && i < int(games.size())) {
            return &games[i];
        }
        return nullptr;
    }

    class MyGameListBox : public gcn::ListBox {
    public:
        MyGameListBox(KnightsApp &ka, const GameList &gl, OnlineMultiplayerScreenImpl &impl)
            : knights_app(ka), game_list(gl), screen_impl(impl) { }
        virtual void mouseClicked(gcn::MouseEvent &mouse_event);
    private:
        KnightsApp &knights_app;
        const GameList &game_list;
        OnlineMultiplayerScreenImpl &screen_impl;
    };
}

class OnlineMultiplayerScreenImpl : public gcn::ActionListener {
public:
    OnlineMultiplayerScreenImpl(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g);
    void action(const gcn::ActionEvent &event);
    void joinGame(const std::string &lobby_id);
    void createGame();
    void update();
    void refreshGameList();

private:
    KnightsApp &knights_app;
    boost::shared_ptr<Coercri::Window> window;
    gcn::Gui &gui;
    
    unsigned int last_refresh_time;

    boost::scoped_ptr<GameList> game_list;

    boost::scoped_ptr<gcn::Label> title_label;
    boost::scoped_ptr<GuiCentre> centre;
    boost::scoped_ptr<GuiPanel> panel;
    boost::scoped_ptr<gcn::Container> container;
    boost::scoped_ptr<gcn::Label> games_label;
    boost::scoped_ptr<TitleBlock> games_titleblock;
    std::unique_ptr<gcn::ScrollArea> scroll_area;
    boost::scoped_ptr<gcn::ListBox> listbox;
    boost::scoped_ptr<gcn::Button> back_button;
    boost::scoped_ptr<gcn::Button> create_game_button;
    boost::shared_ptr<gcn::Font> cg_font;
    std::unique_ptr<gcn::Font> tab_font;
};

namespace
{
    void MyGameListBox::mouseClicked(gcn::MouseEvent &mouse_event)
    {
        gcn::ListBox::mouseClicked(mouse_event);

        if (mouse_event.getButton() == gcn::MouseEvent::LEFT && mouse_event.getClickCount() == 2) {
            const GameInfo *gi = game_list.getGameAt(getSelected());
            if (gi) {
                screen_impl.joinGame(gi->lobby_id);
            }
        }
    }
}

OnlineMultiplayerScreenImpl::OnlineMultiplayerScreenImpl(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g)
    : knights_app(ka), window(win), gui(g), last_refresh_time(ka.getTimer().getMsec())
{
    game_list.reset(new GameList(ka.getOnlinePlatform(), ka));

    // Set up TabFont for formatting columns: Leader | Players | Status
    std::vector<int> widths;
    widths.push_back(200);  // Leader name column
    widths.push_back(80);   // Player count column  
    widths.push_back(320);  // Status column
    
    cg_font.reset(new Coercri::CGFont(ka.getFont()));
    tab_font.reset(new TabFont(cg_font, widths));

    container.reset(new gcn::Container);
    container->setOpaque(false);

    const int pad = 10;
    int y = 5;
    const int width = std::accumulate(widths.begin(), widths.end(), 0);

    title_label.reset(new gcn::Label("Online Multiplayer Games"));
    title_label->setForegroundColor(gcn::Color(0,0,128));
    container->add(title_label.get(), pad + width/2 - title_label->getWidth()/2, y);
    y += title_label->getHeight() + 2*pad;

    games_label.reset(new gcn::Label("Available Games (double-click to join):"));
    container->add(games_label.get(), pad, y);
    y += games_label->getHeight() + pad;

    std::vector<std::string> titles;
    titles.push_back("Host");
    titles.push_back("Players");
    titles.push_back("Status");
    games_titleblock.reset(new TitleBlock(titles, widths));
    games_titleblock->setBaseColor(gcn::Color(200, 200, 200));
    container->add(games_titleblock.get(), pad, y);
    y += games_titleblock->getHeight();

    listbox.reset(new MyGameListBox(knights_app, *game_list, *this));
    listbox->setListModel(game_list.get());
    listbox->setFont(tab_font.get());
    listbox->setWidth(width);
    scroll_area = MakeScrollArea(*listbox, width, 300 - games_titleblock->getHeight());
    container->add(scroll_area.get(), pad, y);
    y += scroll_area->getHeight() + pad*3/2;

    create_game_button.reset(new GuiButton("Create New Game"));
    create_game_button->addActionListener(this);
    back_button.reset(new GuiButton("Back"));
    back_button->addActionListener(this);
    container->add(create_game_button.get(), pad, y);
    container->add(back_button.get(), pad + width - back_button->getWidth(), y);

    container->setSize(2*pad + width, y + back_button->getHeight() + pad);

    panel.reset(new GuiPanel(container.get()));
    centre.reset(new GuiCentre(panel.get()));
    gui.setTop(centre.get());
}

void OnlineMultiplayerScreenImpl::action(const gcn::ActionEvent &event)
{
    if (event.getSource() == back_button.get()) {
        // Return to Start Game screen
        std::unique_ptr<Screen> start_screen(new StartGameScreen);
        knights_app.requestScreenChange(std::move(start_screen));

    } else if (event.getSource() == create_game_button.get()) {
        createGame();
    }
}

void OnlineMultiplayerScreenImpl::joinGame(const std::string &lobby_id)
{
    // Go to ConnectingScreen
    OnlinePlatform &op = knights_app.getOnlinePlatform();
    Coercri::UTF8String my_player_name = op.lookupUserName(op.getCurrentUserId());

    std::unique_ptr<Screen> connecting_screen(new ConnectingScreen(lobby_id,
                                                                   0,
                                                                   false, // lan game
                                                                   true,  // online platform game
                                                                   my_player_name));
    knights_app.requestScreenChange(std::move(connecting_screen));
}

void OnlineMultiplayerScreenImpl::createGame()
{
    // For now, go directly to LoadingScreen
    // TODO: Instead we should probably go to a create game screen where the user can set options
    OnlinePlatform &op = knights_app.getOnlinePlatform();
    Coercri::UTF8String my_player_name = op.lookupUserName(op.getCurrentUserId());

    std::unique_ptr<Screen> loading_screen(new LoadingScreen(0, my_player_name,
                                                             false,    // single player
                                                             false,    // menu strict
                                                             false,    // tutorial
                                                             false));  // autostart
    knights_app.requestScreenChange(std::move(loading_screen));
}

void OnlineMultiplayerScreenImpl::refreshGameList()
{
    // Remember the currently selected lobby ID before refreshing
    std::string selected_lobby_id;
    int current_selection = listbox->getSelected();
    if (current_selection >= 0 && game_list) {
        const GameInfo *selected_game = game_list->getGameAt(current_selection);
        if (selected_game) {
            selected_lobby_id = selected_game->lobby_id;
        }
    }
    
    // Create new game list
    game_list.reset(new GameList(knights_app.getOnlinePlatform(), knights_app));
    listbox->setListModel(game_list.get());
    
    // Try to restore the selection if the lobby still exists
    if (!selected_lobby_id.empty()) {
        for (int i = 0; i < game_list->getNumberOfElements(); ++i) {
            const GameInfo *game = game_list->getGameAt(i);
            if (game && game->lobby_id == selected_lobby_id) {
                listbox->setSelected(i);
                break;
            }
        }
    }
    
    // Invalidate the window to ensure the UI updates
    window->invalidateAll();
}

void OnlineMultiplayerScreenImpl::update()
{
    const unsigned int current_time = knights_app.getTimer().getMsec();
    const unsigned int refresh_interval = 5000; // 5 seconds in milliseconds
    
    if (current_time - last_refresh_time >= refresh_interval) {
        refreshGameList();
        last_refresh_time = current_time;
    }
}

bool OnlineMultiplayerScreen::start(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui)
{
    pimpl.reset(new OnlineMultiplayerScreenImpl(ka, win, gui));
    return true;
}

void OnlineMultiplayerScreen::update()
{
    pimpl->update();
}

#endif   // ONLINE_PLATFORM

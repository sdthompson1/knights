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

#include "adjust_list_box_size.hpp"
#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"
#include "knights_app.hpp"
#include "loading_screen.hpp"
#include "make_scroll_area.hpp"
#include "online_multiplayer_screen.hpp"
#include "online_platform.hpp"
#include "rstream.hpp"
#include "start_game_screen.hpp"
#include "tab_font.hpp"
#include "title_block.hpp"
#include "vm_loading_screen.hpp"
#include "xxhash.hpp"

#include "boost/scoped_ptr.hpp"
#include "boost/thread.hpp"
#include "gcn/cg_font.hpp"
#include <vector>
#include <string>
#include <numeric>

namespace {
    struct GameInfo {
        GameInfo(const std::string &lobby_id, const Coercri::UTF8String &leader_name, int players,
                 const LocalKey &game_status_key,
                 const std::vector<LocalParam> &game_status_params,
                 uint64_t chksum)
            : lobby_id(lobby_id), leader_name(leader_name), num_players(players),
              game_status_key(game_status_key),
              game_status_params(game_status_params),
              checksum(chksum)
        { }
        std::string lobby_id;
        Coercri::UTF8String leader_name;
        int num_players;
        LocalKey game_status_key;
        std::vector<LocalParam> game_status_params;
        uint64_t checksum;
    };

    class GameList : public gcn::ListModel {
    public:
        explicit GameList(KnightsApp &app);
        void refresh();
        virtual int getNumberOfElements();
        virtual std::string getElementAt(int i);
        const GameInfo * getGameAt(int i) const;

    private:
        std::vector<GameInfo> games;
        KnightsApp &knights_app;
    };

    GameList::GameList(KnightsApp &app)
        : knights_app(app)
    {
        refresh();
    }

    void GameList::refresh()
    {
        OnlinePlatform &platform = knights_app.getOnlinePlatform();
        std::vector<std::string> lobbies = platform.getLobbyList();
        games.clear();
        for (const auto & lobby_id : lobbies) {
            OnlinePlatform::LobbyInfo info = platform.getLobbyInfo(lobby_id);
            Coercri::UTF8String leader_name = platform.lookupUserName(info.leader_id);
            games.push_back(GameInfo(lobby_id, leader_name, info.num_players,
                                     info.game_status_key, info.game_status_params, info.checksum));
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
            std::string result = game.leader_name.asUTF8();
            result += "\t";
            result += std::to_string(game.num_players);
            result += "\t";

            const Localization &loc = knights_app.getLocalization();
            Coercri::UTF8String status_msg = loc.get(game.game_status_key, game.game_status_params);
            result += status_msg.asUTF8();

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
        virtual void draw(gcn::Graphics* graphics);
    private:
        KnightsApp &knights_app;
        const GameList &game_list;
        OnlineMultiplayerScreenImpl &screen_impl;
    };

    class ChecksumThread {
    public:
        explicit ChecksumThread(boost::mutex &mutex,
                                uint64_t &checksum,
                                bool &checksum_done,
                                std::string &&build_id)
            : mutex(mutex), checksum(checksum), checksum_done(checksum_done) {}
        void operator()();
    private:
        boost::mutex &mutex;
        uint64_t &checksum;
        bool &checksum_done;
        std::string build_id;
    };

    void ChecksumThread::operator()()
    {
        // Construct a hash of knights_data/server file contents,
        // together with build ID from the online platform.
        XXHash hasher(0);
        hasher.updateHashPartial(reinterpret_cast<uint8_t*>(build_id.data()), build_id.length());
        RStream::HashDirectory("server", hasher);
        uint64_t value = hasher.finalHash();

        // Write result back to the main thread
        boost::unique_lock lock(mutex);
        checksum = value;
        checksum_done = true;
    }

    bool g_show_incompatible = false;
}

class OnlineMultiplayerScreenImpl : public gcn::ActionListener {
public:
    OnlineMultiplayerScreenImpl(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g);
    ~OnlineMultiplayerScreenImpl();
    void createFullGui();
    void action(const gcn::ActionEvent &event);
    void setLobbyFilters(bool show_incompat);
    void joinGame(const std::string &lobby_id);
    void createGame();
    void update();
    void refreshGameList();

    uint64_t myChecksum() const { return my_checksum; }

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
    boost::scoped_ptr<gcn::CheckBox> show_incompatible_checkbox;
    boost::scoped_ptr<gcn::Button> refresh_button;
    boost::scoped_ptr<gcn::Button> cancel_button;
    boost::scoped_ptr<gcn::Button> create_game_button;
    boost::shared_ptr<gcn::Font> cg_font;
    std::unique_ptr<gcn::Font> tab_font;

    boost::mutex mutex;  // Protects my_checksum and checksum_done
    uint64_t my_checksum;
    bool checksum_done = false;
    boost::thread checksum_thread;
};

namespace
{
    void MyGameListBox::mouseClicked(gcn::MouseEvent &mouse_event)
    {
        gcn::ListBox::mouseClicked(mouse_event);

        if (mouse_event.getButton() == gcn::MouseEvent::LEFT && mouse_event.getClickCount() == 2) {
            const GameInfo *gi = game_list.getGameAt(getSelected());
            if (gi) {
                bool is_incompatible = (gi->checksum != screen_impl.myChecksum());
                if (is_incompatible) {
                    throw ExceptionBase(LocalKey("incompatible_game"));
                } else {
                    screen_impl.joinGame(gi->lobby_id);
                }
            }
        }
    }

    void MyGameListBox::draw(gcn::Graphics* graphics)
    {
        graphics->setColor(getBackgroundColor());
        graphics->fillRectangle(gcn::Rectangle(0, 0, getWidth(), getHeight()));

        if (mListModel == NULL)
        {
            return;
        }

        graphics->setFont(getFont());

        // Check the current clip area so we don't draw unnecessary items
        // that are not visible.
        const gcn::ClipRectangle currentClipArea = graphics->getCurrentClipArea();
        int rowHeight = getRowHeight();

        // Calculate the number of rows to draw by checking the clip area.
        // The addition of two makes covers a partial visible row at the top
        // and a partial visible row at the bottom.
        int numberOfRows = currentClipArea.height / rowHeight + 2;

        if (numberOfRows > mListModel->getNumberOfElements())
        {
            numberOfRows = mListModel->getNumberOfElements();
        }

        // Calculate which row to start drawing. If the list box
        // has a negative y coordinate value we should check if
        // we should drop rows in the begining of the list as
        // they might not be visible. A negative y value is very
        // common if the list box for instance resides in a scroll
        // area and the user has scrolled the list box downwards.
        int startRow;
        if (getY() < 0)
        {
            startRow = -1 * (getY() / rowHeight);
        }
        else
        {
            startRow = 0;
        }

        int i;
        // The y coordinate where we start to draw the text is
        // simply the y coordinate multiplied with the font height.
        int y = rowHeight * startRow;

        for (i = startRow; i < startRow + numberOfRows; ++i)
        {
            // Check if this game has an incompatible checksum
            const GameInfo *game_info = game_list.getGameAt(i);
            bool is_incompatible = (game_info && game_info->checksum != screen_impl.myChecksum());

            if (i == mSelected)
            {
                graphics->setColor(getSelectionColor());
                graphics->fillRectangle(gcn::Rectangle(0, y, getWidth(), rowHeight));
            }

            // Set text color: red for incompatible, normal foreground color otherwise
            if (is_incompatible)
            {
                graphics->setColor(gcn::Color(255, 0, 0));  // Red
            }
            else
            {
                graphics->setColor(getForegroundColor());
            }

            // If the row height is greater than the font height we
            // draw the text with a center vertical alignment.
            if (rowHeight > getFont()->getHeight())
            {
                graphics->drawText(mListModel->getElementAt(i), 1, y + rowHeight / 2 - getFont()->getHeight() / 2);
            }
            else
            {
                graphics->drawText(mListModel->getElementAt(i), 1, y);
            }

            y += rowHeight;
        }
    }
}

OnlineMultiplayerScreenImpl::OnlineMultiplayerScreenImpl(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g)
    : knights_app(ka), window(win), gui(g), last_refresh_time(ka.getTimer().getMsec() - 10000)
{
    // Set up skeleton GUI (rest will be created once our checksum is known)
    container.reset(new gcn::Container);
    container->setOpaque(false);
    centre.reset(new GuiCentre(container.get()));
    gui.setTop(centre.get());

    // Start a background thread to checksum all knights_data files
    ChecksumThread thr(mutex, my_checksum, checksum_done, knights_app.getOnlinePlatform().getBuildId());
    checksum_thread = std::move(boost::thread(thr));
}

OnlineMultiplayerScreenImpl::~OnlineMultiplayerScreenImpl()
{
    if (checksum_thread.joinable()) {
        checksum_thread.join();
    }
}

void OnlineMultiplayerScreenImpl::createFullGui()
{
    const Localization &loc = knights_app.getLocalization();

    // Remove the skeleton GUI as it is no longer needed
    gui.setTop(nullptr);
    centre.reset();
    container.reset();

    // Initialize lobby filters based on initial setting for
    // incompatible games checkbox
    setLobbyFilters(g_show_incompatible);

    // Create game list (using the initial filters)
    game_list.reset(new GameList(knights_app));

    // Set up TabFont for formatting columns: Leader | Players | Status
    std::vector<int> widths;
    widths.push_back(200);  // Leader name column
    widths.push_back(80);   // Player count column  
    widths.push_back(320);  // Status column
    
    cg_font.reset(new Coercri::CGFont(knights_app.getFont()));
    tab_font.reset(new TabFont(cg_font, widths));

    container.reset(new gcn::Container);
    container->setOpaque(false);

    const int pad = 10;
    int y = 5;
    const int width = std::accumulate(widths.begin(), widths.end(), 0);

    title_label.reset(new gcn::Label(loc.get(LocalKey("online_multiplayer_games")).asUTF8()));
    title_label->setForegroundColor(gcn::Color(0,0,128));
    container->add(title_label.get(), pad + width/2 - title_label->getWidth()/2, y);
    y += title_label->getHeight() + 2*pad;

    games_label.reset(new gcn::Label(loc.get(LocalKey("avail_games_click_to_join")).asUTF8()));
    container->add(games_label.get(), pad, y);

    show_incompatible_checkbox.reset(new gcn::CheckBox(loc.get(LocalKey("show_incompat_games")).asUTF8()));
    show_incompatible_checkbox->addActionListener(this);
    show_incompatible_checkbox->setSelected(g_show_incompatible);
    container->add(show_incompatible_checkbox.get(), pad + width - show_incompatible_checkbox->getWidth(), y + games_label->getHeight()/2 - show_incompatible_checkbox->getHeight()/2);

    y += games_label->getHeight() + pad;

    std::vector<std::string> titles;
    titles.push_back(loc.get(LocalKey("host")).asUTF8());
    titles.push_back(loc.get(LocalKey("players")).asUTF8());
    titles.push_back(loc.get(LocalKey("status")).asUTF8());
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
    y += scroll_area->getHeight() + pad;

    AdjustListBoxSize(*listbox, *scroll_area);

    y += pad/2;

    create_game_button.reset(new GuiButton(loc.get(LocalKey("create_new_game")).asUTF8()));
    create_game_button->addActionListener(this);
    refresh_button.reset(new GuiButton(loc.get(LocalKey("refresh_list")).asUTF8()));
    refresh_button->addActionListener(this);
    cancel_button.reset(new GuiButton(loc.get(LocalKey("cancel")).asUTF8()));
    cancel_button->addActionListener(this);
    container->add(create_game_button.get(), pad, y);
    container->add(refresh_button.get(), 25 + create_game_button->getWidth() + pad, y);
    container->add(cancel_button.get(), pad + width - cancel_button->getWidth(), y);

    container->setSize(2*pad + width, y + cancel_button->getHeight() + pad);

    panel.reset(new GuiPanel(container.get()));
    centre.reset(new GuiCentre(panel.get()));

    // This sequence is needed to get the GUI to display:
    int w, h;
    window->getSize(w, h);
    centre->setSize(w, h);
    gui.setTop(centre.get());
    gui.logic();
    window->invalidateAll();
}

void OnlineMultiplayerScreenImpl::action(const gcn::ActionEvent &event)
{
    if (event.getSource() == cancel_button.get()) {
        // Return to Start Game screen
        std::unique_ptr<Screen> start_screen(new StartGameScreen);
        knights_app.requestScreenChange(std::move(start_screen));

    } else if (event.getSource() == create_game_button.get()) {
        createGame();

    } else if (event.getSource() == show_incompatible_checkbox.get()) {
        setLobbyFilters(show_incompatible_checkbox->isSelected());

        // Remember setting for next time this UI is opened
        g_show_incompatible = show_incompatible_checkbox->isSelected();

    } else if (event.getSource() == refresh_button.get()) {
        knights_app.getOnlinePlatform().refreshLobbyList();
        refreshGameList();
    }
}

void OnlineMultiplayerScreenImpl::setLobbyFilters(bool show_incompat)
{
    OnlinePlatform &platform = knights_app.getOnlinePlatform();
    platform.clearLobbyFilters();
    if (!show_incompat) {
        platform.addChecksumFilter(myChecksum());
    }
}

void OnlineMultiplayerScreenImpl::joinGame(const std::string &lobby_id)
{
    // Go to VMLoadingScreen
    OnlinePlatform::Visibility vis = OnlinePlatform::Visibility::PRIVATE; // Dummy value
    std::unique_ptr<Screen> loading_screen(new VMLoadingScreen(lobby_id, vis, my_checksum));
    knights_app.requestScreenChange(std::move(loading_screen));
}

void OnlineMultiplayerScreenImpl::createGame()
{
    // For now, go directly to VMLoadingScreen
    // TODO: Instead we should probably go to a create game screen where the user can set options
    OnlinePlatform::Visibility vis = OnlinePlatform::Visibility::PUBLIC; // Temporary value
    std::unique_ptr<Screen> loading_screen(new VMLoadingScreen("", vis, my_checksum));
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
    game_list->refresh();
    AdjustListBoxSize(*listbox, *scroll_area);

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
    // Hold off until the checksum is ready.
    if (checksum_thread.joinable()) {
        boost::unique_lock lock(mutex);
        if (!checksum_done) return;
        checksum_thread.join();

        // Once checksum is available, activate the GUI
        createFullGui();
    }

    // Refresh the GameList if required.
    // Note: Online platform caches the getLobbyList results so refreshing
    // the UI relatively frequently (every 200ms) should be OK
    const unsigned int refresh_interval = 200;
    const unsigned int current_time = knights_app.getTimer().getMsec();
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

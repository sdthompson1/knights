/*
 * game_manager.hpp
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

/*
 * Implements ClientCallbacks interface.
 *
 */

#ifndef GAME_MANAGER_HPP
#define GAME_MANAGER_HPP

#include "client_callbacks.hpp"
#include "utf8string.hpp"

// coercri includes
#include "gfx/font.hpp"
#include "timer/timer.hpp"

#include "guichan.hpp"

#include "boost/shared_ptr.hpp"
#include <deque>
#include <functional>
#include <vector>

class GameManagerImpl;
class KnightsApp;
class KnightsClient;
class OnlinePlatform;


// internal structs, used in ChatList
struct FormattedLine {
    bool firstline;
    std::string text_latin1;
};
class FontWrapper;

class ChatList : public gcn::ListModel {
public:
    explicit ChatList(int mm, bool do_fmt, bool do_tstmp)
        : max_msgs(mm), font(0), width(999999), is_updated(false), do_format(do_fmt), do_timestamps(do_tstmp) { }
    
    void add(const std::string &msg_latin1);
    void setGuiParams(const gcn::Font *new_font, int new_width);
    void clear();

    // from gcn::ListModel
    virtual int getNumberOfElements() override { return formatted_lines.size(); }
    virtual std::string getElementAt(int i) override { if (i>=0 && i<formatted_lines.size()) return formatted_lines[i].text_latin1; else return ""; }

    bool isUpdated();  // clears is_updated flag afterwards. used for auto-scrolling to bottom.
    
private:
    void doSetWidth(int);
    void addFormattedLine(const std::string &msg_latin1);
    void rmFormattedLine();
    
private:
    int max_msgs;
    const gcn::Font *font;
    int width;
    std::deque<std::string> lines;    // latin1 encoding
    std::deque<FormattedLine> formatted_lines;
    bool is_updated;
    bool do_format;
    bool do_timestamps;
};

// list of unique player IDs, and corresponding names
class NameList : public gcn::ListModel {
public:
    NameList(const std::vector<Coercri::Color> &hc, KnightsApp &app);
    void add(const PlayerID &id, bool observer, bool ready, int house_col);
    void alter(const PlayerID &id, const bool *observer, const bool *ready, const int *house_col);  // ptr = null means don't change.
    void clearReady();
    void clear();
    void remove(const PlayerID &id);

    // overridden from gcn::ListModel
    virtual int getNumberOfElements() override;
    virtual std::string getElementAt(int i) override;   // latin1 encoding (with special sequences for "house colour blocks")

private:
    void sortNames();
    Coercri::UTF8String nameLookup(const PlayerID &id) const;

    struct Name {
        PlayerID id;
        bool observer;
        bool ready;
        int house_col;
    };
    std::vector<Name> names;
    const std::vector<Coercri::Color> &house_cols;
#ifdef ONLINE_PLATFORM
    OnlinePlatform &online_platform;
#endif
};


class GameManager : public ClientCallbacks {
public:
    GameManager(KnightsApp &ka,
                boost::shared_ptr<KnightsClient> client,
                boost::shared_ptr<Coercri::Timer> timer,
                bool single_player,
                bool tutorial,
                bool autostart,
                bool allow_lobby_screen,
                bool can_invite,
                const PlayerID &my_player_id);

    void tryJoinGame(const std::string &game_name);
    void tryJoinGameSplitScreen(const std::string &game_name);


    //
    // gui management
    //

    // lobby
    void setServerName(const std::string &server_name);
    const std::string & getServerName() const;
    const std::vector<GameInfo> & getGameInfos() const;
    bool isGameListUpdated();  // clears game_list_updated flag afterwards
    
    // this is set true if any of the following are invalid:
    // lobby players list, this game players list (incl. ready flags and house colours), chat list, quest description
    // after the call, the gui_invalid flag is cleared.
    bool isGuiInvalid();
    
    const std::string & getCurrentGameName() const;

    
    // quest selection menu
    const LocalKey & getMenuTitle() const;
    void createMenuWidgets(gcn::ActionListener *listener,
                           gcn::SelectionListener *listener2,
                           int initial_x,
                           int initial_y,
                           gcn::Container &container,
                           int &menu_width,
                           int &y_after_menu);
    void destroyMenuWidgets();
    void setMenuWidgetsEnabled(bool enabled);
    void getMenuStrings(std::vector<std::pair<std::string, std::string> > &menu_strings) const;
    bool getMenuWidgetInfo(gcn::Widget *source, int &item_num, int &choice_num) const;
    LocalKey getQuestMessageCode() const;  // return a localization key for name of current quest
    const UTF8String &getQuestDescription() const;
    
    // player lists etc.
    NameList & getLobbyPlayersList() const;   // names of players in lobby
    NameList & getGamePlayersList() const;    // names of players in the current game
    ChatList & getChatList() const;
    ChatList & getIngamePlayerList() const;
    ChatList & getQuestRequirementsList() const;
    Coercri::Color getAvailHouseColour(int) const;  // translate house-colour-code into RGB colour.
    int getNumAvailHouseColours() const;
    std::function<UTF8String(const PlayerID&)> getPlayerNameLookup() const;
    bool getMyObsFlag() const;
    bool getMyReadyFlag() const;
    int getMyHouseColour() const;
    int getTimeRemaining() const;  // in ms, or -1 if no time limit

    // Updates saved chat string, also returns true if the string has changed since last time.
    // See Trac #11 and #12.
    bool setSavedChat(const UTF8String &);

    bool doingMenuWidgetUpdate() const; // Trac #72


    //
    // callback implementations
    //
    
    virtual void connectionLost() override;     // goes to ErrorScreen
    virtual void connectionFailed() override;   // goes to ErrorScreen
    virtual void serverError(const LocalKey &error) override;       // goes to ErrorScreen
    virtual void luaError(const std::string &error) override;       // goes to ErrorScreen
    virtual void connectionAccepted(int server_version) override;   // goes to LobbyScreen
    
    virtual void joinGameAccepted(boost::shared_ptr<const ClientConfig> conf,
                                  int my_house_colour,
                                  const std::vector<PlayerID> &player_ids,
                                  const std::vector<bool> &ready_flags,
                                  const std::vector<int> &house_cols,
                                  const std::vector<PlayerID> &observers,
                                  bool already_started) override;
    virtual void joinGameDenied(const LocalKey &reason) override;     // goes to ErrorScreen

    virtual void loadGraphic(const Graphic &g, const std::string &contents) override;
    virtual void loadSound(const Sound &s, const std::string &contents) override;

    virtual void passwordRequested(bool first_attempt) override;
    virtual void playerConnected(const PlayerID &id) override;
    virtual void playerDisconnected(const PlayerID &id) override;

    virtual void updateGame(const std::string &game_name, int num_players, int num_observers, GameStatus status) override;
    virtual void dropGame(const std::string &game_name) override;
    virtual void updatePlayer(const PlayerID &player, const std::string &game, bool obs_flag) override;
    virtual void playerList(const std::vector<ClientPlayerInfo> &player_list) override;
    virtual void setTimeRemaining(int milliseconds) override;
    virtual void playerIsReadyToEnd(const PlayerID &player) override;
    
    virtual void leaveGame();     // goes to lobby
    virtual void setMenuSelection(int item_num, int choice_num, const std::vector<int> &allowed_vals) override;
    virtual void setQuestDescription(const UTF8String &quest_descr) override;
    virtual void startGame(int ndisplays, bool deathmatch_mode,
                           const std::vector<PlayerID> &player_ids, bool already_started) override;  // goes to InGameScreen
    virtual void gotoMenu() override;     // goes to MenuScreen

    virtual void playerJoinedThisGame(const PlayerID &id, bool obs_flag, int house_col) override;
    virtual void playerLeftThisGame(const PlayerID &id, bool obs_flag) override;
    virtual void setPlayerHouseColour(const PlayerID &id, int house_col) override;
    virtual void setAvailableHouseColours(const std::vector<Coercri::Color> &cols) override;
    virtual void setReady(const PlayerID &id, bool ready) override;
    virtual void deactivateReadyFlags() override;

    virtual void setObsFlag(const PlayerID &id, bool new_obs_flag) override;
    
    virtual void chat(const PlayerID &id, bool observer, bool team, const Coercri::UTF8String &msg) override;
    virtual void announcementLoc(const LocalKey &key, const std::vector<LocalParam> &params, bool err) override;


private:
    void updateMenuWidget(int item_num);
    void updateAllMenuWidgets();
    void gotoMenuIfAllDownloaded();
    
private:
    boost::shared_ptr<GameManagerImpl> pimpl;
};

#endif

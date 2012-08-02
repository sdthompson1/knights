/*
 * game_manager.hpp
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

/*
 * Implements ClientCallbacks interface.
 *
 */

#ifndef GAME_MANAGER_HPP
#define GAME_MANAGER_HPP

#include "client_callbacks.hpp"

// coercri includes
#include "gfx/font.hpp"
#include "timer/timer.hpp"

#include "guichan.hpp"

#include "boost/shared_ptr.hpp"
#include <deque>
#include <vector>

class GameManagerImpl;
class KnightsApp;
class KnightsClient;


// internal structs, used in ChatList
struct FormattedLine {
    bool firstline;
    std::string text;
};
class FontWrapper;

class ChatList : public gcn::ListModel {
public:
    explicit ChatList(int mm, bool do_fmt, bool do_tstmp)
        : max_msgs(mm), width(999999), is_updated(false), do_format(do_fmt), do_timestamps(do_tstmp) { }
    
    void add(const std::string &msg);
    void setGuiParams(const Coercri::Font *new_font, int new_width);
    void setGuiParams(const gcn::Font *new_font, int new_width);
    void clear();

    int getNumberOfElements() { return formatted_lines.size(); }
    std::string getElementAt(int i) { if (i>=0 && i<formatted_lines.size()) return formatted_lines[i].text; else return ""; }

    bool isUpdated();  // clears is_updated flag afterwards. used for auto-scrolling to bottom.
    
private:
    void doSetWidth(int);
    void addFormattedLine(const std::string &msg);
    void rmFormattedLine();
    
private:
    int max_msgs;
    boost::shared_ptr<FontWrapper> font;
    int width;
    std::deque<std::string> lines;
    std::deque<FormattedLine> formatted_lines;
    bool is_updated;
    bool do_format;
    bool do_timestamps;
};

// list of unique player names
class NameList : public gcn::ListModel {
public:
    explicit NameList(const std::vector<Coercri::Color> &hc) : house_cols(hc) { }
    void add(const std::string &x, bool observer, bool ready, int house_col);
    void alter(const std::string &x, const bool *observer, const bool *ready, const int *house_col);  // ptr = null means don't change.
    void clearReady();
    void clear();
    void remove(const std::string &x);
    int getNumberOfElements();
    std::string getElementAt(int i);
private:
    struct Name {
        std::string name;
        bool observer;
        bool ready;
        int house_col;
        bool operator<(const Name &other) const {
            return observer < other.observer || 
                (observer == other.observer && ToUpper(name) < ToUpper(other.name));
        }
    private:
        static std::string ToUpper(const std::string &input);
    };
    std::vector<Name> names;
    const std::vector<Coercri::Color> &house_cols;
};


class GameManager : public ClientCallbacks {
public:
    GameManager(KnightsApp &ka, boost::shared_ptr<KnightsClient> client, boost::shared_ptr<Coercri::Timer> timer,
                bool single_player_, bool tutorial, bool autostart, const std::string &my_player_name);

    void setLanGame(bool);
    
    // join game
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
    const std::string & getMenuTitle() const;
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
    const std::string &getQuestDescription() const;
    
    // player lists etc.
    NameList & getLobbyPlayersList() const;   // names of players in lobby
    NameList & getGamePlayersList() const;    // names of players in the current game
    ChatList & getChatList() const;
    ChatList & getIngamePlayerList() const;
    ChatList & getQuestRequirementsList() const;
    Coercri::Color getAvailHouseColour(int) const;  // translate house-colour-code into RGB colour.
    int getNumAvailHouseColours() const;
    bool getMyObsFlag() const;
    bool getMyReadyFlag() const;
    int getMyHouseColour() const;
    int getTimeRemaining() const;  // in ms, or -1 if no time limit

    // Updates saved chat string, also returns true if the string has changed since last time.
    // See Trac #11 and #12.
    bool setSavedChat(const std::string &);

    bool doingMenuWidgetUpdate() const; // Trac #72
    

    //
    // callback implementations
    //
    
    virtual void connectionLost();     // goes to ErrorScreen
    virtual void connectionFailed();   // goes to ErrorScreen
    virtual void serverError(const std::string &error);    // goes to ErrorScreen
    virtual void connectionAccepted(int server_version);   // goes to LobbyScreen
    
    virtual void joinGameAccepted(boost::shared_ptr<const ClientConfig> conf,
                                  int my_house_colour,
                                  const std::vector<std::string> &player_names,
                                  const std::vector<bool> &ready_flags,
                                  const std::vector<int> &house_cols,
                                  const std::vector<std::string> &observers);
    virtual void joinGameDenied(const std::string &reason);     // goes to ErrorScreen

    virtual void loadGraphic(const Graphic &g, const std::string &contents);
    virtual void loadSound(const Sound &s, const std::string &contents);

    virtual void passwordRequested(bool first_attempt);
    virtual void playerConnected(const std::string &name);
    virtual void playerDisconnected(const std::string &name);

    virtual void updateGame(const std::string &game_name, int num_players, int num_observers, GameStatus status);
    virtual void dropGame(const std::string &game_name);
    virtual void updatePlayer(const std::string &player, const std::string &game, bool obs_flag);
    virtual void playerList(const std::vector<ClientPlayerInfo> &player_list);
    virtual void setTimeRemaining(int milliseconds);
    virtual void playerIsReadyToEnd(const std::string &player);
    
    virtual void leaveGame();     // goes to lobby
    virtual void setMenuSelection(int item_num, int choice_num, const std::vector<int> &allowed_vals);
    virtual void setQuestDescription(const std::string &quest_descr);
    virtual void startGame(int ndisplays, bool deathmatch_mode,
                           const std::vector<std::string> &player_names, bool already_started);  // goes to InGameScreen
    virtual void gotoMenu();     // goes to MenuScreen

    virtual void playerJoinedThisGame(const std::string &name, bool obs_flag, int house_col);
    virtual void playerLeftThisGame(const std::string &name, bool obs_flag);
    virtual void setPlayerHouseColour(const std::string &name, int house_col);
    virtual void setAvailableHouseColours(const std::vector<Coercri::Color> &cols);
    virtual void setReady(const std::string &name, bool ready);
    virtual void deactivateReadyFlags();

    virtual void setObsFlag(const std::string &name, bool new_obs_flag);
    
    virtual void chat(const std::string &whofrom, bool observer, bool team, const std::string &msg);
    virtual void announcement(const std::string &msg, bool err);


private:
    void updateMenuWidget(int item_num);
    void updateAllMenuWidgets();
    void gotoMenuIfAllDownloaded();
    
private:
    boost::shared_ptr<GameManagerImpl> pimpl;
};

#endif

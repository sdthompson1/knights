/*
 * knights_game.hpp
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
 * Class representing an actual game of Knights. (Each KnightsServer
 * can have multiple KnightsGames.)
 *
 * This class is responsible for creating/destroying the sub-thread
 * used to run the game. 
 * 
 */

#ifndef KNIGHTS_GAME_HPP
#define KNIGHTS_GAME_HPP

class KnightsConfig;
class KnightsGameImpl;
class GameConnection;

#include "game_info.hpp"  // for GameStatus

#include "gfx/color.hpp" // coercri
#include "timer/timer.hpp"  // coercri

#include "boost/shared_ptr.hpp"
#include <deque>
#include <memory>
#include <string>
#include <vector>

class KnightsLog;

class KnightsGame {
public:
    explicit KnightsGame(boost::shared_ptr<KnightsConfig> config, boost::shared_ptr<Coercri::Timer> tmr,
                         bool allow_split_screen, bool tutorial_mode, KnightsLog *knights_log, 
                         const std::string &game_name,
                         std::auto_ptr<std::deque<int> > update_counts,
                         std::auto_ptr<std::deque<int> > time_deltas,
                         std::auto_ptr<std::deque<unsigned int> > random_seeds);
    ~KnightsGame();

    // get information
    int getNumPlayers() const;
    int getNumObservers() const;
    GameStatus getStatus() const;
    bool isSplitScreenAllowed() const;
    bool getObsFlag(GameConnection &) const;
    
    // add players/observers.
    // will throw an exception if the same player is added twice.
    // (if client_name_2 is non-empty will create a split-screen 2-player connection, if allowed.)
    GameConnection & newClientConnection(const std::string &client_name, const std::string &client_name_2, 
                                         int client_version, bool approach_based_controls, bool action_bar_controls);

    // remove a player/observer.
    // the GameConnection is invalid after this call.
    void clientLeftGame(GameConnection &conn);


    // incoming msgs should be decoded by KnightsServer and either
    // handled there, or forwarded to one of the following methods:
    void sendChatMessage(GameConnection &, const std::string &msg);
    void setReady(GameConnection &, bool ready);
    void setHouseColour(GameConnection &, int hse_col);
    void finishedLoading(GameConnection &);
    void readyToEnd(GameConnection &);
    bool requestQuit(GameConnection &);
    void setPauseMode(bool p); 
    void setMenuSelection(GameConnection &, int item_num, int new_choice_num);
    void setMenuSelectionWork(GameConnection *, int item_num, int new_choice_num);  // not usu. called directly (replays are the exception).
    void randomQuest(GameConnection &);
    void sendControl(GameConnection &, int plyr, unsigned char control_num);  // plyr is usually 0; can be 1 in split screen mode
    void requestSpeechBubble(GameConnection &, bool show);
    void setObsFlag(GameConnection &, bool flag);

    // This should be called after any calls to sendControl() or
    // requestSpeechBubble(), it makes sure that these messages get
    // through to the game engine. NOTE: wait until you have sent a
    // batch of sendControl()s and/or requestSpeechBubble()s, THEN
    // call endOfMessagePacket().
    void endOfMessagePacket();
    
    // Get any outgoing msgs that need to be sent to the client.
    // Any existing contents of "data" are replaced.
    void getOutputData(GameConnection &conn, std::vector<unsigned char> &data);

    void setPingTime(GameConnection &conn, int ping);

    // This is used for replays
    void setMsgCountUpdateFlag(bool on);
    
private:
    boost::shared_ptr<KnightsGameImpl> pimpl;
};

#endif

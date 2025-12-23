/*
 * knights_client.hpp
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
 * Class representing the connection from a client to a Knights
 * server.
 *
 * Messages can be sent to the server by calling the methods of this
 * class. This will put data into the output buffer, this should then
 * be read with getOutputData() and sent to the server.
 *
 * When data is received from the server it should be processed by
 * calling processInputData(). This will convert any server commands
 * received into calls to the callback interfaces.
 *
 */

#ifndef KNIGHTS_CLIENT_HPP
#define KNIGHTS_CLIENT_HPP

#include "utf8string.hpp"

#include "boost/noncopyable.hpp"

#include <memory>
#include <vector>

class ClientCallbacks;
class KnightsCallbacks;
class KnightsClientImpl;
class PlayerID;
class UserControl;

class KnightsClient : boost::noncopyable {
public:
    typedef unsigned char ubyte;

    explicit KnightsClient(bool allow_untrusted_strings);
    ~KnightsClient();

    bool allowUntrustedStrings() const;


    //
    // Methods to handle incoming/outgoing data
    //

    // process input data
    // this will generate calls to the callback interfaces, if set (see below).
    void receiveInputData(const std::vector<ubyte> &data);

    // get the output data (and then clear the output buffer).
    // caller is then responsible for sending this data to the server.
    // any existing contents of 'data' are deleted.
    void getOutputData(std::vector<ubyte> &data);

    // this should be called if the network link is closed or lost.
    void connectionClosed();

    // this is like connectionClosed but is called when the attempt to establish the outgoing connection fails.
    void connectionFailed();


    //
    // Get/Set Callbacks
    //

    ClientCallbacks* getClientCallbacks() const;
    void setClientCallbacks(ClientCallbacks *client_callbacks);
    KnightsCallbacks* getKnightsCallbacks() const;
    void setKnightsCallbacks(KnightsCallbacks *knights_callbacks);
    

    //
    // Methods to send commands to the server (will write data to the
    // output buffer)
    // 

    void setPlayerIdAndControls(const PlayerID &id, bool action_bar_ctrls);   // should be 1st cmd sent
    void joinGame(const std::string &game_name);   // attempt to join a game.
    void joinGameSplitScreen(const std::string &game_name);  // used for split screen mode.
    void leaveGame();   // attempt to leave a game (go back to "unjoined" state).

    void sendChatMessage(const UTF8String &msg);

    void setReady(bool ready);  // change my ready status (used in menu screen)
    void setHouseColour(int hse_col);
    void setObsFlag(bool obs_flag);   // attempt to become player (false param) or observer (true param)
    void setMenuSelection(int item, int choice);  // set a menu option (used in menu screen)
    void randomQuest();   // ask the server to generate a random quest
    
    void finishedLoading();  // tell server that we have finished loading and are ready to start game

    // Send a control to my knight
    // Note: continuous controls will act until cancelled (by sending a
    // `nullptr`), while discontinuous controls will act once only.
    void sendControl(int plyr, const UserControl *ctrl);

    // tell server whether i am 'typing' or not
    void requestSpeechBubble(bool show);
    
    void readyToEnd();   // tell server that we are ready to exit winner/loser screen

    void setPauseMode(bool);  // only works for split screen games currently.

private:
    std::unique_ptr<KnightsClientImpl> pimpl;
};

#endif

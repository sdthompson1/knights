/*
 * client_callbacks.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
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
 * This must be implemented by the client to handle responses from the
 * server. Each possible server response corresponds to a method call
 * on this interface.
 *
 */

#ifndef CLIENT_CALLBACKS_HPP
#define CLIENT_CALLBACKS_HPP

#include "game_info.hpp"

#include "gfx/color.hpp" // coercri

#include "boost/shared_ptr.hpp"
#include <string>
#include <vector>

class ClientConfig;

struct ClientPlayerInfo {
    std::string name;
    Coercri::Color house_colour;
    int kills;
    int deaths;
    int ping;
};

class ClientCallbacks {
public:
    virtual ~ClientCallbacks() { }

    //
    // Callbacks that can be received at any time
    //

    // connection is being closed (or was never successfully opened)
    virtual void connectionLost() = 0;
    virtual void connectionFailed() = 0;

    // special
    virtual void serverError(const std::string &error) = 0;
    virtual void connectionAccepted(int server_version) = 0;
    
    
    //
    // "Pre-join-game" callbacks
    //
    
    // join game success/failure
    // NOTE: The returned ClientConfig will remain valid for as long as we are connected to the game
    virtual void joinGameAccepted(boost::shared_ptr<const ClientConfig> conf,
                                  int my_house_colour,
                                  const std::vector<std::string> &player_names,
                                  const std::vector<bool> &ready_flags,
                                  const std::vector<int> &house_cols,
                                  const std::vector<std::string> &observers) = 0;
    virtual void joinGameDenied(const std::string &reason) = 0;

    // called if the server wants us to enter a password before proceeding.
    virtual void passwordRequested(bool first_attempt) = 0;
    
    // called when other players connect to or disconnect from the server.
    virtual void playerConnected(const std::string &name) = 0;
    virtual void playerDisconnected(const std::string &name) = 0;


    //
    // Global game/player list updates
    //
    
    virtual void updateGame(const std::string &game_name, int num_players, int num_observers, GameStatus status) = 0;
    virtual void dropGame(const std::string &game_name) = 0;
    virtual void updatePlayer(const std::string &player, const std::string &game, bool obs_flag) = 0;
    virtual void playerList(const std::vector<ClientPlayerInfo> &player_list) = 0;
    virtual void setTimeRemaining(int milliseconds) = 0;
    virtual void playerIsReadyToEnd(const std::string &player) = 0;
    
    //
    // "Post-join-game" callbacks
    //

    // called when I leave the game.
    virtual void leaveGame() = 0;
    
    // menu
    virtual void setMenuSelection(const std::string &key, int val, const std::vector<int> &allowed_values) = 0;
    virtual void setQuestDescription(const std::string &quest_descr) = 0;

    // switching between menu and in-game states
    virtual void startGame(int ndisplays, const std::vector<std::string> &player_names, bool already_started) = 0;
    virtual void gotoMenu() = 0;
    
    // called when players join/leave my current game, or change state.
    // player_num is 0 or 1 for active players, or -1 for observers.
    virtual void playerJoinedThisGame(const std::string &name, bool obs_flag, int house_col) = 0;
    virtual void playerLeftThisGame(const std::string &name, bool obs_flag) = 0;
    virtual void setPlayerHouseColour(const std::string &name, int house_col) = 0;
    virtual void setAvailableHouseColours(const std::vector<Coercri::Color> &cols) = 0;
    virtual void setReady(const std::string &name, bool ready) = 0;

    // called when a player, in the current game, changes from observer to player or vice versa
    virtual void setObsFlag(const std::string &name, bool new_obs_flag) = 0;
    
    // chat, and "announcements".
    virtual void chat(const std::string &whofrom, bool observer, const std::string &msg) = 0;
    virtual void announcement(const std::string &msg) = 0;
};

#endif

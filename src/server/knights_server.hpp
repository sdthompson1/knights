/*
 * knights_server.hpp
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
 * Represents a Knights server. Can run multiple games (and will start
 * multiple threads to do so).
 *
 * Note that access to the KnightsServer object (through its public
 * interface) should still be single threaded.
 *
 */

#ifndef KNIGHTS_SERVER_HPP
#define KNIGHTS_SERVER_HPP

#include "game_info.hpp"

#include "timer/timer.hpp"  // coercri

#include "boost/shared_ptr.hpp"
#include <deque>
#include <memory>
#include <vector>

class KnightsConfig;
class KnightsLog;
class KnightsServerImpl;
class ServerConnection;

class KnightsServer {
public:
    typedef unsigned char ubyte;

    KnightsServer(boost::shared_ptr<Coercri::Timer> timer,
                  bool allow_split_screen,
                  bool tutorial_mode,
                  const std::string &motd_filename,
                  const std::string &old_motd_filename,
                  const std::string &password);
    ~KnightsServer();
    
    //
    // Methods for dealing with client connections
    // 
    
    // Call this when a new client connects
    // NOTE: ServerConnections are owned by KnightsServer and will be deleted by ~KnightsServer.
    // ip_addr is optional; if set it will be displayed in the log.
    ServerConnection & newClientConnection(std::string ip_addr = std::string());

    // Call when data is received from a client
    void receiveInputData(ServerConnection &conn, const std::vector<ubyte> &data);

    // Get data waiting to be sent to a client
    // (this copies the internal output buffer to the given vector, then clears the buffer)
    // Caller is responsible for making sure this data is sent to the client.
    void getOutputData(ServerConnection &conn, std::vector<ubyte> &data);

    // Call this when a client disconnects
    void connectionClosed(ServerConnection &conn);

    // The server regularly sends ping time reports to clients.
    // The following method should be called -- ideally before each call to getOutputData --
    // to let the server know what each player's ping time is.
    void setPingTime(ServerConnection &conn, int ping);

    
    //
    // Methods for creating / destroying games.
    //
    
    // Start a new game
    void startNewGame(boost::shared_ptr<KnightsConfig> config,
                      const std::string &game_name,
                      std::auto_ptr<std::deque<int> > update_counts = std::auto_ptr<std::deque<int> >(),
                      std::auto_ptr<std::deque<int> > time_deltas = std::auto_ptr<std::deque<int> >(),
                      std::auto_ptr<std::deque<unsigned int> > random_seeds = std::auto_ptr<std::deque<unsigned int> >() );
    
    // Destroy an empty game. This will fail if anyone is currently connected to the game.
    // Returns true if successful.
    bool closeGame(const std::string &game_name);

    // Get the list of running games.
    std::vector<GameInfo> getRunningGames() const;

    // Set a menu selection
    // Used for replays.
    void setMenuSelection(const std::string &game_name, const std::string &key, int value);

    
    //
    // Query server status.
    //

    int getNumberOfPlayers() const;


    //
    // Logging
    //

    // Caller is responsible for creating & destroying the KnightsLog object
    // NOTE: This should be called before any games are created, as changes are
    // not (currently) propagated to existing KnightsGames when this is called.
    void setKnightsLog(KnightsLog *);


    void setMsgCountUpdateFlag(bool on);
    
private:
    std::auto_ptr<KnightsServerImpl> pimpl;
};

#endif

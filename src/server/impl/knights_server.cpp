/*
 * knights_server.cpp
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

#include "knights_game.hpp"
#include "knights_log.hpp"
#include "knights_server.hpp"
#include "protocol.hpp"
#include "sh_ptr_eq.hpp"
#include "version.hpp"

#include "network/byte_buf.hpp"  // coercri

#include <fstream>
#include <sstream>
#include <map>

class ServerConnection {
public:
    explicit ServerConnection(const std::string &ip)
        : wait_until(0), game_conn(0), client_version(0), 
          version_string_received(false), connection_accepted(false),
          failed_password_attempts(0), ip_addr(ip), error_sent(false),
          approach_based_controls(true), action_bar_controls(false)
    { }
    
    // output buffer
    std::vector<unsigned char> output_data;
    unsigned int wait_until;   // don't send output before this time. used for password checking. 0 = disabled.

    // player name
    std::string player_name;
    
    // connection to game
    // INVARIANT: either game and game_conn are both null, or they are both non-null.
    boost::shared_ptr<KnightsGame> game;
    GameConnection * game_conn;
    std::string game_name;

    // have we got version string yet?
    int client_version;
    bool version_string_received;
    
    // have we accepted their player name (and password if applicable) yet?
    bool connection_accepted;

    int failed_password_attempts;
    std::string ip_addr;
    bool error_sent;

    // this is used for logging/game recording.
    int unique_id;

    // what type of controls they are using
    bool approach_based_controls;
    bool action_bar_controls;
};

const int MAX_PASSWORD_ATTEMPTS = 5;

typedef std::vector<boost::shared_ptr<ServerConnection> > connection_vector;
typedef std::map<std::string, boost::shared_ptr<KnightsGame> > game_map;

class KnightsServerImpl {
public:
    boost::shared_ptr<Coercri::Timer> timer;
    bool allow_split_screen;
    
    game_map games;
    connection_vector connections;

    std::string motd_file;
    std::string old_motd_file;
    std::string password;

    KnightsLog *knights_log;

    int conn_counter;
};

namespace {
    void ReadDataFromKnightsGame(ServerConnection &conn)
    {
        if (conn.game) {
            if (conn.output_data.empty()) {
                // no existing data so can just overwrite
                conn.game->getOutputData(*conn.game_conn, conn.output_data);
            } else {
                // append it to the existing data
                std::vector<unsigned char> buf;
                conn.game->getOutputData(*conn.game_conn, buf);
                conn.output_data.insert(conn.output_data.end(), buf.begin(), buf.end());
            }
        }
    }

    void SendJoinGameDenied(ServerConnection &conn, const std::string &reason)
    {
        ReadDataFromKnightsGame(conn);
        Coercri::OutputByteBuf buf(conn.output_data);
        buf.writeUbyte(SERVER_JOIN_GAME_DENIED);
        buf.writeString(reason);
    }

    void SendError(ServerConnection &conn, const std::string &error, KnightsServerImpl &impl)
    {
        Coercri::OutputByteBuf buf(conn.output_data);
        buf.writeUbyte(SERVER_ERROR);
        buf.writeString(error);

        if (impl.knights_log) {
            impl.knights_log->logMessage(conn.game_name + "\terror\tplayer=" + conn.player_name + ", error=" + error);
        }

        conn.error_sent = true;
    }

    bool IsNameAvailable(const connection_vector &connections, const std::string &name)
    {
        for (connection_vector::const_iterator it = connections.begin(); it != connections.end(); ++it) {
            if ((*it)->player_name == name) return false;
        }
        return true;
    }

    void SendStartupMessages(Coercri::OutputByteBuf &out, ServerConnection &conn, connection_vector &connections, game_map &games)
    {
        // send the list of connected players to that player
        for (connection_vector::iterator it = connections.begin(); it != connections.end(); ++it) {
            if ((*it)->connection_accepted || it->get() == &conn) {
                out.writeUbyte(SERVER_UPDATE_PLAYER);
                out.writeString((*it)->player_name);
                out.writeString((*it)->game_name);
                bool is_obs = false;
                if ((*it)->game) {
                    is_obs = (*it)->game->getObsFlag(*(*it)->game_conn);
                }
                out.writeUbyte(is_obs ? 1 : 0);
            }
        }

        // send the list of games to that player
        for (game_map::iterator it = games.begin(); it != games.end(); ++it) {
            out.writeUbyte(SERVER_UPDATE_GAME);
            out.writeString(it->first);
            out.writeVarInt(it->second->getNumPlayers());
            out.writeVarInt(it->second->getNumObservers());
            out.writeUbyte(it->second->getStatus());
        }

        // send 'connection accepted' msg to that player
        out.writeUbyte(SERVER_CONNECTION_ACCEPTED);
        out.writeVarInt(KNIGHTS_VERSION_NUM);

        // send a new player notification to all other players.
        for (connection_vector::iterator it = connections.begin(); it != connections.end(); ++it) {
            if (it->get() != &conn) {
                Coercri::OutputByteBuf out_other((*it)->output_data);
                out_other.writeUbyte(SERVER_PLAYER_CONNECTED);
                out_other.writeString(conn.player_name);
            }
        }

        // the connection is now accepted.
        conn.connection_accepted = true;
    }

    void SendGameUpdate(connection_vector &connections, const std::string &game_name, 
        int num_players, int num_observers, GameStatus status)
    {
        for (connection_vector::iterator it = connections.begin(); it != connections.end(); ++it) {
            Coercri::OutputByteBuf out((*it)->output_data);
            out.writeUbyte(SERVER_UPDATE_GAME);
            out.writeString(game_name);
            out.writeVarInt(num_players);
            out.writeVarInt(num_observers);
            out.writeUbyte(status);
        }
    }

    void LeaveGame(connection_vector &connections, ServerConnection &conn)
    {
        const boost::shared_ptr<KnightsGame> the_game = conn.game;
        const std::string game_name = conn.game_name;
                
        // read any pending data from the game before we get rid of his game connection...
        ReadDataFromKnightsGame(conn);

        // remove him from the game
        conn.game->clientLeftGame(*conn.game_conn);
        conn.game.reset();
        conn.game_conn = 0;
        conn.game_name = "";

        // tell him that he has been booted out of the game
        conn.output_data.push_back(SERVER_LEAVE_GAME);

        // send SERVER_UPDATE_PLAYER messages to all connections
        for (connection_vector::iterator it = connections.begin(); it != connections.end(); ++it) {
            Coercri::OutputByteBuf out((*it)->output_data);
            out.writeUbyte(SERVER_UPDATE_PLAYER);
            out.writeString(conn.player_name);
            out.writeString("");
            out.writeUbyte(0);
        }

        // Send SERVER_UPDATE_GAME messages (as the number of players, and maybe status, will have changed).
        const GameStatus new_status = the_game->getStatus();
        SendGameUpdate(connections, game_name, 
                       the_game->getNumPlayers(), the_game->getNumObservers(), new_status);
        
    }
}

KnightsServer::KnightsServer(boost::shared_ptr<Coercri::Timer> timer,
                             bool allow_split_screen, const std::string &motd_file, 
                             const std::string &old_motd_file, const std::string &password)
    : pimpl(new KnightsServerImpl)
{
    pimpl->timer = timer;
    pimpl->allow_split_screen = allow_split_screen;
    pimpl->motd_file = motd_file;
    pimpl->old_motd_file = old_motd_file;
    pimpl->password = password;
    pimpl->knights_log = 0;
    pimpl->conn_counter = 1;
}

KnightsServer::~KnightsServer()
{
}

ServerConnection & KnightsServer::newClientConnection(std::string ip)
{
    boost::shared_ptr<ServerConnection> new_conn(new ServerConnection(ip));
    new_conn->unique_id = pimpl->conn_counter++;
    pimpl->connections.push_back(new_conn);

    // log that we got a new connection (can't show player name yet).
    if (pimpl->knights_log) {
        pimpl->knights_log->logMessage("\tincoming connection\taddr=" + ip);

        // also save to binary log
        pimpl->knights_log->logBinary("NCC", new_conn->unique_id, 0, 0);
    }

    return *new_conn;
}

void KnightsServer::receiveInputData(ServerConnection &conn,
                                     const std::vector<ubyte> &data)
{
    // This is where we decode incoming messages from the client

    // First save the msg to the binary log
    if (pimpl->knights_log) {
        pimpl->knights_log->logBinary("RCV", conn.unique_id, data.size(), reinterpret_cast<const char*>(&data[0]));
    }
    
    std::string error_msg;
    
    try {
    
        Coercri::InputByteBuf buf(data);
    
        while (!buf.eof()) {

            if (!conn.version_string_received) {
                // The first thing the client sends us is a version string
                // e.g. "Knights/009".
                const std::string client_version_string = buf.readString();
                const std::string expected = "Knights/";
                if (client_version_string.substr(0, expected.length()) != expected) {
                    throw ProtocolError("Invalid connection string");
                }
                
                // Parse the version number
                std::istringstream str(client_version_string);
                str.seekg(expected.length());
                int ver = 0;
                str >> ver;

                // Server version check. For now we just insist that the client is running the same version as the server.
                // Update Aug 2011: be slightly less strict, allow any client version between
                // COMPATIBLE_VERSION_NUM and the current server version (inclusive)
                if (ver < COMPATIBLE_VERSION_NUM) {
                    throw ProtocolError("You are running an old version of Knights. Please download the latest version from " KNIGHTS_WEBSITE);
                } else if (ver > KNIGHTS_VERSION_NUM) {
                    throw ProtocolError("Cannot connect because this server is running an older version of Knights.");
                }

                // Send him the MOTD
                // Note: old_motd_file is deprecated. It will not get used, thanks to the above version check.
                const std::string & motd_file = (ver < KNIGHTS_VERSION_NUM) ? pimpl->old_motd_file : pimpl->motd_file;
                if (!motd_file.empty()) {
                    std::ifstream str(motd_file.c_str());
                    std::string motd;
                    while (1) {
                        std::string line;
                        std::getline(str, line);
                        if (!str) break;
                        motd += line;
                        motd += '\n';
                    }
                    Coercri::OutputByteBuf buf(conn.output_data);
                    buf.writeUbyte(SERVER_ANNOUNCEMENT);
                    buf.writeString(motd);
                }
                
                conn.client_version = ver;
                conn.version_string_received = true;
                continue;
            }
        
            const ubyte msg = buf.readUbyte();

            // If the player name and password have not yet been accepted then the client can only send us two
            // messages: CLIENT_SET_PLAYER_NAME and CLIENT_SEND_PASSWORD.
            if (!conn.connection_accepted && msg != CLIENT_SET_PLAYER_NAME && msg != CLIENT_SEND_PASSWORD) {
                // note: if an error has already been sent then do not send another 'access denied' error.
                // (have had problems with useful error messages being overwritten by not-very-useful
                // 'access denied' messages...)
                if (conn.error_sent) {
                    return;
                } else {
                    throw ProtocolError("Access denied");
                }
            }

            switch (msg) {

            case CLIENT_SET_PLAYER_NAME:
                {
                    const std::string new_name = buf.readString();
                    Coercri::OutputByteBuf out(conn.output_data);
                    if (!conn.player_name.empty()) {
                        SendError(conn, "Player name already set", *pimpl);
                    } else if (new_name.empty()) {
                        SendError(conn, "Bad player name", *pimpl);
                    } else if (!IsNameAvailable(pimpl->connections, new_name)) {
                        SendError(conn, "A player with the name \"" + new_name + "\" is already connected.", *pimpl);
                    } else {
                        // set the player name
                        conn.player_name = new_name;

                        // write a log message
                        if (pimpl->knights_log) {
                            pimpl->knights_log->logMessage("\tplayer connected\taddr=" + conn.ip_addr + ", player=" + new_name);
                        }

                        // If the server has a password then request the password. Otherwise proceed as if the
                        // password has just been accepted.
                        if (!pimpl->password.empty()) {
                            out.writeUbyte(SERVER_REQUEST_PASSWORD);
                            out.writeUbyte(1);
                        } else {
                            SendStartupMessages(out, conn, pimpl->connections, pimpl->games);
                        }
                    }
                }
                break;

            case CLIENT_SEND_PASSWORD:
                {
                    const std::string their_password = buf.readString();
                    Coercri::OutputByteBuf out(conn.output_data);
                    if (conn.player_name.empty()) {
                        throw ProtocolError("Must set player name before sending password");
                    }
                    if (conn.failed_password_attempts == MAX_PASSWORD_ATTEMPTS) {
                        throw ProtocolError("Too many failed password attempts");                    
                    } else if (their_password == pimpl->password) {
                        Coercri::OutputByteBuf(conn.output_data);
                        SendStartupMessages(out, conn, pimpl->connections, pimpl->games);
                        if (pimpl->knights_log) {
                            pimpl->knights_log->logMessage("\tpassword accepted\tplayer=" + conn.player_name);
                        }
                    } else {
                        ++conn.failed_password_attempts;
                        conn.wait_until = pimpl->timer->getMsec() + 2000;  // make them wait a couple of seconds between password attempts
                        out.writeUbyte(SERVER_REQUEST_PASSWORD);
                        out.writeUbyte(0);
                        if (pimpl->knights_log) {
                            pimpl->knights_log->logMessage("\tpassword rejected\tplayer=" + conn.player_name);
                        }
                    }
                }
                break;
            
            case CLIENT_JOIN_GAME:
            case CLIENT_JOIN_GAME_SPLIT_SCREEN:
                {
                    const bool split_screen = (msg == CLIENT_JOIN_GAME_SPLIT_SCREEN);
                    const std::string game_name = buf.readString();

                    game_map::iterator it = pimpl->games.find(game_name);
                    if (conn.game) {
                        SendJoinGameDenied(conn, "You are already connected to a game");
                    } else if (it == pimpl->games.end()) {
                        SendJoinGameDenied(conn, "The game \"" + game_name + "\" does not exist on this server.");
                    } else if (split_screen && !it->second->isSplitScreenAllowed()) {
                        SendJoinGameDenied(conn, "Split screen game not allowed");
                    } else if (split_screen && (it->second->getNumPlayers() > 0 || it->second->getNumObservers() > 0)) {
                        SendJoinGameDenied(conn, "Cannot join split-screen if other players are already connected");
                    } else {

                        std::string client_name, client_name_2;
                    
                        if (split_screen) {
                            // dummy player names for the split screen mode
                            client_name = "Player 1";
                            client_name_2 = "Player 2";
                        } else {
                            // name 1 comes from the connection object. name 2 is unset.
                            client_name = conn.player_name;
                        }
                        
                        // the following will send the SERVER_JOIN_GAME_ACCEPTED message and
                        // any necessary SERVER_PLAYER_JOINED_THIS_GAME messages.
                        // NOTE: This should not throw since we have checked all possible error conditions above.
                        conn.game_conn = &it->second->newClientConnection(client_name, client_name_2,
                                                                          conn.client_version,
                                                                          conn.approach_based_controls,
                                                                          conn.action_bar_controls);
                        conn.game = it->second;
                        conn.game_name = game_name;

                        // Now send SERVER_UPDATE_PLAYER messages to all connections.
                        for (connection_vector::iterator it2 = pimpl->connections.begin(); it2 != pimpl->connections.end(); ++it2) {
                            Coercri::OutputByteBuf out((*it2)->output_data);
                            out.writeUbyte(SERVER_UPDATE_PLAYER);
                            out.writeString(client_name);
                            out.writeString(game_name);
                            out.writeUbyte(conn.game->getObsFlag(*conn.game_conn));
                            // NOTE: don't bother with supporting the split screen mode here, so no msg for client_name_2.
                        }

                        // Also send SERVER_UPDATE_GAME messages.
                        const GameStatus new_status = it->second->getStatus();
                        SendGameUpdate(pimpl->connections, it->first, it->second->getNumPlayers(), 
                            it->second->getNumObservers(), new_status);
                    }
                }
                break;
            
            case CLIENT_LEAVE_GAME:
                if (conn.game) {
                    // Kick him out of the game.
                    LeaveGame(pimpl->connections, conn);
                }
                break;
            
            case CLIENT_CHAT:
                {
                    const std::string msg = buf.readString();

                    if (conn.game) {
                        conn.game->sendChatMessage(*conn.game_conn, msg);
                    } else {
                        // send the message to everybody who is not in a game (including the originator)
                        for (connection_vector::const_iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
                            if (!(*it)->game) {
                                Coercri::OutputByteBuf out_other((*it)->output_data);
                                out_other.writeUbyte(SERVER_CHAT);
                                out_other.writeString(conn.player_name);
                                out_other.writeUbyte(0);
                                out_other.writeString(msg);
                            }
                        }
                    }

                    // log it
                    if (pimpl->knights_log) {
                        std::string log_msg = conn.game_name + "\tchat\t";
                        log_msg += conn.player_name + ": " + msg;
                        pimpl->knights_log->logMessage(log_msg);
                    }
                }
                break;
            
            case CLIENT_SET_READY:
                {
                    const bool ready = buf.readUbyte() != 0;
                    if (conn.game) {
                        const GameStatus old_status = conn.game->getStatus();
                        
                        conn.game->setReady(*conn.game_conn, ready);

                        // Send SERVER_UPDATE_GAME messages if needed (game status might have changed after a SET_READY)
                        const GameStatus new_status = conn.game->getStatus();
                        if (new_status != old_status) SendGameUpdate(pimpl->connections, conn.game_name, 
                            conn.game->getNumPlayers(), conn.game->getNumObservers(), new_status);
                    }
                }
                break;

            case CLIENT_SET_HOUSE_COLOUR:
                {
                    const int x = buf.readUbyte();
                    if (conn.game) {
                        conn.game->setHouseColour(*conn.game_conn, x);
                    }
                }
                break;

            case CLIENT_SET_MENU_SELECTION:
                {
                    const int item_num = buf.readVarInt();
                    const int choice_num = buf.readVarInt();
                    if (conn.game) {
                        conn.game->setMenuSelection(*conn.game_conn, item_num, choice_num);
                    }
                }
                break;
            
            case CLIENT_FINISHED_LOADING:
                if (conn.game) {
                    conn.game->finishedLoading(*conn.game_conn);
                }
                break;
            
            case CLIENT_SEND_CONTROL:
                {
                    int control_num = buf.readUbyte();
                    int plyr = 0;
                    if (control_num >= 128) {
                        plyr = 1;
                        control_num -= 128;
                    }
                    if (conn.game) {
                        conn.game->sendControl(*conn.game_conn, plyr, control_num);
                    }
                }
                break;

            case CLIENT_READY_TO_END:
                if (conn.game) {
                    const GameStatus old_status = conn.game->getStatus();
                    conn.game->readyToEnd(*conn.game_conn);  // might change status to SELECTING_QUEST
                    const GameStatus new_status = conn.game->getStatus();
                    if (new_status != old_status) {
                        SendGameUpdate(pimpl->connections, conn.game_name, conn.game->getNumPlayers(),
                                       conn.game->getNumObservers(), new_status);
                    }
                }
                break;

            case CLIENT_QUIT:
                if (conn.game) {
                    const GameStatus old_status = conn.game->getStatus();
                    const bool kick = conn.game->requestQuit(*conn.game_conn);  // might change status to SELECTING_QUEST

                    // If >2 players then ESC-Q will kick you out of the game rather than stopping the game.
                    // We handle that here.
                    if (kick) {
                        LeaveGame(pimpl->connections, conn);
                    } else {
                        // Not kicked out but the game status might still have changed
                        const GameStatus new_status = conn.game->getStatus();
                        if (old_status != new_status) {
                            SendGameUpdate(pimpl->connections, conn.game_name, conn.game->getNumPlayers(),
                                           conn.game->getNumObservers(), new_status);
                        }
                    }
                }
                break;

            case CLIENT_SET_PAUSE_MODE:
                {
                    const bool p = buf.readUbyte() != 0;
                    if (conn.game) {
                        conn.game->setPauseMode(p);
                    }
                }
                break;

            case CLIENT_SET_OBS_FLAG:
                {
                    const bool requested_flag = buf.readUbyte() != 0;
                    if (conn.game) {
                        const bool old_flag = conn.game->getObsFlag(*conn.game_conn);
                        if (requested_flag != old_flag) {
                            conn.game->setObsFlag(*conn.game_conn, requested_flag);
                            const bool new_flag = conn.game->getObsFlag(*conn.game_conn);
                            if (new_flag == requested_flag) { // request was successful
                                // notify all players of the new obs status
                                for (connection_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
                                    Coercri::OutputByteBuf out(conn.output_data);
                                    out.writeUbyte(SERVER_UPDATE_PLAYER);
                                    out.writeString(conn.player_name);
                                    out.writeString(conn.game_name);
                                    out.writeUbyte(new_flag ? 1 : 0);
                                }
                                // notify all players of the new game state (number of observers will have changed)
                                SendGameUpdate(pimpl->connections, conn.game_name, conn.game->getNumPlayers(),
                                               conn.game->getNumObservers(), conn.game->getStatus());
                            }
                        }
                    }
                }
                break;

            case CLIENT_REQUEST_SPEECH_BUBBLE:
                {
                    const bool show = buf.readUbyte() != 0;
                    if (conn.game) {
                        conn.game->requestSpeechBubble(*conn.game_conn, show);
                    }
                }
                break;

            case CLIENT_SET_APPROACH_BASED_CONTROLS:
                {
                    const bool app_based = buf.readUbyte() != 0;
                    conn.approach_based_controls = app_based;
                }
                break;

            case CLIENT_SET_ACTION_BAR_CONTROLS:
                {
                    const bool action_bar = buf.readUbyte() != 0;
                    conn.action_bar_controls = action_bar;
                }
                break;

            case CLIENT_RANDOM_QUEST:
                if (conn.game) {
                    conn.game->randomQuest(*conn.game_conn);
                }
                break;

            case CLIENT_REQUEST_GRAPHICS:
                {
                    const int num_ids = buf.readVarInt();
                    std::vector<int> ids;
                    ids.reserve(num_ids);
                    for (int i = 0; i < num_ids; ++i) {
                        ids.push_back(buf.readVarInt());
                    }
                    Coercri::OutputByteBuf buf(conn.output_data);
                    if (conn.game) conn.game->requestGraphics(buf, ids);
                }
                break;

            case CLIENT_REQUEST_SOUNDS:
                {
                    const int num_ids = buf.readVarInt();
                    std::vector<int> ids;
                    ids.reserve(num_ids);
                    for (int i = 0; i < num_ids; ++i) {
                        ids.push_back(buf.readVarInt());
                    }
                    Coercri::OutputByteBuf buf(conn.output_data);
                    if (conn.game) conn.game->requestSounds(buf, ids);
                }                
                break;
                
            default:
                throw ProtocolError("Unknown message code from client");
            }
        }

        if (conn.game) conn.game->endOfMessagePacket();

        return;  // everything below here is error handling code.
        
    } catch (std::exception &e) {
        error_msg = e.what();
    } catch (...) {
        error_msg = "Unknown Error";
    }

    // Exception thrown. This is (probably) because the client sent us
    // some bad data. Send him an error message (the client will
    // usually then respond by closing the connection). Note that we
    // clear out any existing data already in his buffer, this
    // prevents any half-written messages from being sent out.

    conn.output_data.clear();
    SendError(conn, error_msg, *pimpl);
}

void KnightsServer::getOutputData(ServerConnection &conn,
                                  std::vector<ubyte> &data)
{
    if (conn.wait_until != 0 && (int(pimpl->timer->getMsec() - conn.wait_until)) > 0) {
        // don't release the output yet -- still waiting
        data.clear();

    } else {    
        conn.wait_until = 0;
        
        ReadDataFromKnightsGame(conn);
        
        data.swap(conn.output_data);
        conn.output_data.clear();
    }
}

void KnightsServer::connectionClosed(ServerConnection &conn)
{
    // save the player's name & ip.
    const std::string name = conn.player_name;
    const std::string ip = conn.ip_addr;
    const std::string game_name = conn.game_name;
    boost::shared_ptr<KnightsGame> game = conn.game;
    const int unique_id = conn.unique_id;
    const bool accepted = conn.connection_accepted;
    
    // inform the KnightsGame (if we were connected to a game)
    if (conn.game) {
        conn.game->clientLeftGame(*conn.game_conn);
        conn.game.reset();
        conn.game_conn = 0;
        conn.game_name = "";
    }
    
    // erase it from 'connections'
    connection_vector::iterator it = std::find_if(pimpl->connections.begin(), 
                                                  pimpl->connections.end(),
                                                  ShPtrEq<ServerConnection>(&conn));
    if (it != pimpl->connections.end()) {
        pimpl->connections.erase(it);
    }

    // tell other players
    if (accepted) {
        for (connection_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
            Coercri::OutputByteBuf buf((*it)->output_data);
            buf.writeUbyte(SERVER_PLAYER_DISCONNECTED);
            buf.writeString(name);
        }
        if (game) {
            SendGameUpdate(pimpl->connections, game_name, game->getNumPlayers(), game->getNumObservers(), game->getStatus());
        }
    }

    // log a message
    if (pimpl->knights_log) {
        pimpl->knights_log->logMessage(game_name + "\tplayer disconnected\taddr=" + ip + ", player=" + name);

        // binary log
        pimpl->knights_log->logBinary("CCC", unique_id, 0, 0);
    }
}

void KnightsServer::setPingTime(ServerConnection &conn, int ping)
{
    if (conn.game) {
        conn.game->setPingTime(*conn.game_conn, ping);
    }
}

void KnightsServer::startNewGame(boost::shared_ptr<KnightsConfig> config,
                                 const std::string &game_name,
                                 std::auto_ptr<std::deque<int> > update_counts,
                                 std::auto_ptr<std::deque<int> > time_deltas,
                                 std::auto_ptr<std::deque<unsigned int> > random_seeds)
{
    if (game_name.empty()) throw UnexpectedError("Game name not set");
    
    game_map::const_iterator it = pimpl->games.find(game_name);
    if (it != pimpl->games.end()) {
        throw UnexpectedError("A game with this name already exists");
    } else {
        // The new game starts "empty" ie with no players attached to it.
        boost::shared_ptr<KnightsGame> game(new KnightsGame(config, pimpl->timer,
                                                            pimpl->allow_split_screen,
                                                            pimpl->knights_log,
                                                            game_name,
                                                            update_counts, time_deltas, random_seeds));
        pimpl->games.insert(std::make_pair(game_name, game));

        // Notify players about the new game.
        for (connection_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
            Coercri::OutputByteBuf buf((*it)->output_data);
            buf.writeUbyte(SERVER_UPDATE_GAME);
            buf.writeString(game_name);
            buf.writeVarInt(game->getNumPlayers());
            buf.writeVarInt(game->getNumObservers());
            buf.writeUbyte(game->getStatus());
        }

        // Log it
        if (pimpl->knights_log) {
            pimpl->knights_log->logBinary("NGM", 0, game_name.size(), game_name.c_str());
        }
    }
}

bool KnightsServer::closeGame(const std::string &game_name)
{
    game_map::iterator it = pimpl->games.find(game_name);
    if (it != pimpl->games.end()
    && it->second->getNumPlayers() == 0
    && it->second->getNumObservers() == 0) {
        pimpl->games.erase(it);

        // Notify players that the game is being deleted
        for (connection_vector::iterator it2 = pimpl->connections.begin(); it2 != pimpl->connections.end(); ++it2) {
            Coercri::OutputByteBuf buf((*it2)->output_data);
            buf.writeUbyte(SERVER_DROP_GAME);
            buf.writeString(game_name);
        }

        // Log it
        if (pimpl->knights_log) {
            pimpl->knights_log->logBinary("CGM", 0, game_name.size(), game_name.c_str());
        }
        
        return true;
    } else {
        return false;
    }
}

std::vector<GameInfo> KnightsServer::getRunningGames() const
{
    std::vector<GameInfo> result;
    result.reserve(pimpl->games.size());
    for (game_map::const_iterator it = pimpl->games.begin(); it != pimpl->games.end(); ++it) {
        GameInfo gi;
        gi.game_name = it->first;
        gi.num_players = it->second->getNumPlayers();
        gi.num_observers = it->second->getNumObservers();
        gi.status = it->second->getStatus();
        result.push_back(gi);
    }
    return result;
}

void KnightsServer::setMenuSelection(const std::string &game_name, int item, int choice)
{
    game_map::iterator it = pimpl->games.find(game_name);
    if (it != pimpl->games.end()) {
        it->second->setMenuSelectionWork(0, item, choice);
    }
}

int KnightsServer::getNumberOfPlayers() const
{
    return int(pimpl->connections.size());
}

void KnightsServer::setKnightsLog(KnightsLog *klog)
{
    pimpl->knights_log = klog;
}

void KnightsServer::setMsgCountUpdateFlag(bool on)
{
    for (game_map::iterator it = pimpl->games.begin(); it != pimpl->games.end(); ++it) {
        it->second->setMsgCountUpdateFlag(on);
    }
}

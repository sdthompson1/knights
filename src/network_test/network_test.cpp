/*
 * network_test.cpp
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

/*
 * This is a small command line program that tries to connect to a
 * Knights server.
 *
 * It can be used to test that the connection to the server is working
 * correctly, without needing to start up a graphical Knights client.
 *
 */

#include "client_callbacks.hpp"
#include "dungeon_view.hpp"
#include "knights_callbacks.hpp"
#include "knights_client.hpp"
#include "mini_map.hpp"
#include "status_display.hpp"

// coercri includes
#include "network/network_connection.hpp"
#include "enet/enet_network_driver.hpp"
#include "timer/generic_timer.hpp"

// standard libraries
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <iostream>


//
// Message logging function
//

void Log(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    std::time_t time_since_epoch = std::time(0);
    std::string result = std::ctime(&time_since_epoch);
    result = result.substr(0, result.length() - 1);  // remove final '\n'

    printf(result.c_str());
    printf(": ");
    vprintf(format, args);
    printf("\n");
    
    va_end(args);
}


//
// Callback implementations
//

class TestClientCallbacks : public ClientCallbacks {
public:
    TestClientCallbacks() : join_game_accepted(false), game_started(false) { }

    void connectionLost() { Log("Connection lost"); }
    void connectionFailed() { Log("Connection failed"); }

    void serverError(const std::string &error) { Log("Server error: %s", error.c_str()); }
    void connectionAccepted(int server_version) { Log("Connection accepted. Server version = %d", server_version); }

    void joinGameAccepted(boost::shared_ptr<const ClientConfig> config,
                          int my_house_colour,
                          const std::vector<UTF8String> &player_names,
                          const std::vector<bool> &ready_flags,
                          const std::vector<int> &house_cols,
                          const std::vector<UTF8String> &observers)
    {
        Log("Join game accepted");
        join_game_accepted = true;
    }
    void joinGameDenied(const std::string &reason) { Log("Join game denied. Reason = %s", reason.c_str()); }
    void loadGraphic(const Graphic &g, const std::string &) { Log("Loading graphic"); }
    void loadSound(const Sound &s, const std::string &) { Log("Loading sound"); }

    void passwordRequested(bool first_attempt) { Log("Please enter password"); }

    void playerConnected(const UTF8String &name) { Log("Player connected. Name = %s", name.asUTF8().c_str()); }
    void playerDisconnected(const UTF8String &name) { Log("Player disconnected. Name = %s", name.asUTF8().c_str()); }

    void updateGame(const std::string &game_name, int num_players, int num_observers, GameStatus status)
    {
        Log("Update game. Game name = %s", game_name.c_str());
    }
    void dropGame(const std::string &game_name)
    {
        Log("Drop game. Game name = %s", game_name.c_str());
    }
    void updatePlayer(const UTF8String &player, const std::string &game, bool obs_flag)
    {
        Log("Update player. Player name = %s", player.asUTF8().c_str());
    }
    void playerList(const std::vector<ClientPlayerInfo> &player_list)
    {
        Log("Player list. Size = %d", player_list.size());
    }
    void setTimeRemaining(int milliseconds)
    {
        Log("Set time remaining. Milliseconds = %d", milliseconds);
    }
    void playerIsReadyToEnd(const UTF8String &player)
    {
        Log("Player is ready to end. Player name = %s", player.asUTF8().c_str());
    }

    void leaveGame() { Log("Leave game"); }
    void setMenuSelection(int item, int choice, const std::vector<int> &allowed_values)
    {
        Log("Set menu selection. Item = %d, Choice = %d", item, choice);
    }
    void setQuestDescription(const std::string &quest_descr)
    {
        Log("Set quest description. Descr = %s", quest_descr.c_str());
    }

    void startGame(int ndisplays, bool deathmatch_mode, const std::vector<UTF8String> &player_names, bool already_started)
    {
        Log("Start game. Ndisplays = %d", ndisplays);
        game_started = true;
    }
    void gotoMenu() { Log("Goto menu"); }

    void playerJoinedThisGame(const UTF8String &name, bool obs_flag, int house_col)
    {
        Log("Player joined this game. Name = %s", name.asUTF8().c_str());
    }
    void playerLeftThisGame(const UTF8String &name, bool obs_flag)
    {
        Log("Player left this game. Name = %s", name.asUTF8().c_str());
    }
    void setPlayerHouseColour(const UTF8String &name, int house_col)
    {
        Log("Set player house colour. Name = %s", name.asUTF8().c_str());
    }
    void setAvailableHouseColours(const std::vector<Coercri::Color> &cols)
    {
        Log("Set available house colours");
    }
    void setReady(const UTF8String &name, bool ready)
    {
        Log("Set ready. Name = %s, val = %d", name.asUTF8().c_str(), ready ? 1 : 0);
    }
    void deactivateReadyFlags()
    {
        Log("Deactivate ready flags.");
    }

    void setObsFlag(const UTF8String &name, bool new_obs_flag)
    {
        Log("Set obs flag. Name = %s, val = %d", name.asUTF8().c_str(), new_obs_flag ? 1 : 0);
    }
    
    void chat(const UTF8String &whofrom, bool observer, bool team, const std::string &msg)
    {
        Log("Chat. whofrom = %s, obs = %d, team = %d, msg = %s", whofrom.asUTF8().c_str(), observer ? 1 : 0, team ? 1 : 0, msg.c_str());
    }
    void announcement(const std::string &msg, bool is_err)
    {
        Log("Announcement: %s", msg.c_str());
    }

public:
    bool join_game_accepted;
    bool game_started;
};

class TestDungeonView : public DungeonView {
public:
    void setCurrentRoom(int r, int width, int height) { Log("Set current room"); }
    void addEntity(unsigned short int id, int x, int y, MapHeight ht, MapDirection facing,
                   const Anim * anim, const Overlay *ovr, int af, int atz_diff,
                   bool ainvis, bool ainvuln, // (anim data)
                   int cur_ofs, MotionType motion_type, int motion_time_remaining,
                   const UTF8String &name)
    {
        Log("dview: Add entity");
    }
    void rmEntity(unsigned short int id) { Log("dview: Remove entity"); }
    void repositionEntity(unsigned short int id, int new_x, int new_y) { Log("dview: Reposition entity"); }
    void moveEntity(unsigned short int id, MotionType motion_type, int motion_duration, bool missile_mode)
    {
        Log("dview: move entity");
    }
    void flipEntityMotion(unsigned short int id, int initial_delay, int motion_duration)
    {
        Log("dview: flip entity motion");
    }
    void setAnimData(unsigned short int id, const Anim *, const Overlay *, int af, int atz_diff,
                     bool ainvis, bool ainvuln, bool currently_moving)
    {
        Log("dview: set anim data");
    }
    void setFacing(unsigned short int id, MapDirection new_facing)
    {
        Log("dview: set facing");
    }
    void setSpeechBubble(unsigned short int id, bool show)
    {
        Log("dview: set speech bubble");
    }

    void clearTiles(int x, int y, bool force) { Log("dview: clear tiles"); }
    void setTile(int x, int y, int depth, const Graphic *gfx, boost::shared_ptr<const ColourChange>, bool force) 
    { 
        Log("dview: set tile"); 
    }

    void setItem(int x, int y, const Graphic *gfx, bool force) { Log("dview: set item"); }
    void placeIcon(int x, int y, const Graphic *gfx, int dur) { Log("dview: place icon"); }

    void flashMessage(const std::string &msg, int ntimes) { Log("dview: flash msg: %s", msg.c_str()); }
    void cancelContinuousMessages() { Log("dview: cancel continuous messages"); }
    void addContinuousMessage(const std::string &msg) { Log("dview: add cts msg: %s", msg.c_str()); }
};

class TestMiniMap : public MiniMap {
public:
    void setSize(int w, int h) { Log("map: set size"); }
    void setColour(int x, int y, MiniMapColour col) { Log("map: set colour"); }
    void wipeMap() { Log("map: wipe map"); }
    void mapKnightLocation(int n, int x, int y) { Log("map: map knight location"); }
    void mapItemLocation(int x, int y, bool on) { Log("map: map item location"); }
};

class TestStatusDisplay : public StatusDisplay {
public:
    void setBackpack(int slot, const Graphic *gfx, const Graphic *overdraw, int no_carried, int no_max)
    {
        Log("stat: set backpack");
    }
    void addSkull() { Log("stat: add skull"); }
    void setHealth(int h) { Log("stat: set health: %d", h); }
    void setPotionMagic(PotionMagic pm, bool poison_immunity) { Log("stat: set potion magic"); }
    void setQuestHints(const std::vector<std::string> &quest_hints) { Log("stat: set quest hints"); }
};

class TestKnightsCallbacks : public KnightsCallbacks {
public:
    virtual DungeonView & getDungeonView(int p) { return dungeon_view; }
    virtual MiniMap & getMiniMap(int p) { return mini_map; }
    virtual StatusDisplay & getStatusDisplay(int p) { return status_display; }

    void playSound(int player_num, const Sound &sound, int frequency) { Log("Play sound"); }
    void winGame(int player_num) { Log("Win game. Player = %d", player_num); }
    void loseGame(int player_num) { Log("Lose game. Player = %d", player_num); }
    
    void setAvailableControls(int player_num, const std::vector<std::pair<const UserControl*, bool> > &available_controls)
    {
        Log("Set available controls. Player = %d", player_num);
    }
    void setMenuHighlight(int player_num, const UserControl *highlight)
    {
        Log("Set menu highlight, player = %d", player_num);
    }
    void flashScreen(int player_num, int delay) { Log("Flash screen"); }
    void gameMsg(int player_num, const std::string &msg, bool is_err) { Log("Game msg. Player = %d, msg = %s", player_num, msg.c_str()); }
    void popUpWindow(const std::vector<TutorialWindow> &windows) { Log("Pop up window"); }
    void onElimination(int player_num) { Log("On elimination, player = %d", player_num); }
    void disableView(int player_num) { Log("Disable view, player = %d", player_num); }
    void goIntoObserverMode(int nplayers, const std::vector<UTF8String> &names) { Log("Go into observer mode"); }

private:
    TestDungeonView dungeon_view;
    TestMiniMap mini_map;
    TestStatusDisplay status_display;
};
                          
        

//
// Globals
//

// The KnightsClient and related objects
boost::shared_ptr<TestKnightsCallbacks> g_knights_callbacks;
boost::shared_ptr<TestClientCallbacks> g_client_callbacks;
boost::shared_ptr<KnightsClient> g_client;

// The network connection to the server
boost::shared_ptr<Coercri::NetworkDriver> g_driver;
boost::shared_ptr<Coercri::NetworkConnection> g_connection;

// Timer
boost::shared_ptr<Coercri::Timer> g_timer;

// Flag to make the main loop quit
bool g_quit_flag = false;



//
// Network functions
//

bool ProcessOutgoingNetMsgs()
{
    bool did_something = false;
    std::vector<unsigned char> net_msg;

    // find out what data needs to be sent to the server
    g_client->getOutputData(net_msg);

    // send it (if non-empty)
    if (!net_msg.empty()) {
        did_something = true;
        g_connection->send(net_msg);
    }

    return did_something;
}

bool ProcessIncomingNetMsgs()
{
    bool did_something = false;
    std::vector<unsigned char> net_msg;

    // see if a message has come in
    g_connection->receive(net_msg);
    if (!net_msg.empty()) {

        // forward the message to the KnightsClient
        // (This will generate calls to the callback interfaces)
        g_client->receiveInputData(net_msg);
        did_something = true;
    }

    return did_something;
}


//
// Function to send messages to the server
//

void SendMessagesIfRequired()
{
    static bool set_player_name_sent = false;

    // First message must always be "set player name"
    if (!set_player_name_sent) {
        g_client->setPlayerNameAndControls(UTF8String::fromUTF8("Network Testing Bot"), true);
        g_client->joinGame("Game 1");
        set_player_name_sent = true;
        return;
    } else if (g_client_callbacks->join_game_accepted) {
        g_client->setReady(true);
	g_client_callbacks->join_game_accepted = false;  // prevent multiple send of "set ready" msgs.
    } else if (g_client_callbacks->game_started) {
        g_client->finishedLoading();
	g_client_callbacks->game_started = false; 
    }
}



//
// Main program
//

int main(int argc, const char **argv)
{
    // Parse arguments
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <hostname> <port>" << std::endl;
        std::exit(1);
    }

    std::string hostname = argv[1];
    int port = std::atoi(argv[2]);

    std::cout << "Connecting to host: " << hostname << ", port: " << port << std::endl;

    // Create a timer
    g_timer.reset(new Coercri::GenericTimer);
    
    // Create the callback objects and the KnightsClient
    g_knights_callbacks.reset(new TestKnightsCallbacks);
    g_client_callbacks.reset(new TestClientCallbacks);
    g_client.reset(new KnightsClient);

    g_client->setClientCallbacks(g_client_callbacks.get());
    g_client->setKnightsCallbacks(g_knights_callbacks.get());

    // Open a connection to the server
    g_driver.reset(new Coercri::EnetNetworkDriver(0, 1, true));
    g_connection = g_driver->openConnection(hostname, port);

    // Main loop
    while (!g_quit_flag) {
        bool did_something = false;
        did_something = g_driver->doEvents() || did_something;
        did_something = ProcessOutgoingNetMsgs() || did_something;
        did_something = ProcessIncomingNetMsgs() || did_something;
        SendMessagesIfRequired();

        if (!did_something) {
            g_timer->sleepMsec(100);
        }

        switch (g_connection->getState()) {
        case Coercri::NetworkConnection::PENDING:
            Log("<Connection pending>");
            break;
        case Coercri::NetworkConnection::CLOSED:
            Log("<Connection closed! Exiting.>");
            g_quit_flag = true;
            break;
        case Coercri::NetworkConnection::FAILED:
            Log("<Connection failed! Exiting.>");
            g_quit_flag = true;
            break;
        }
    }

    // Clean up network connections and driver
    g_connection->close();
    g_connection.reset();
    g_driver.reset();

    // Clean up other objects
    g_client.reset();
    g_timer.reset();

    return 0;
}

/*
 * server_main.cpp
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
 * Command line Knights server program.
 *
 */

#include "misc.hpp"

#include "config.hpp"
#include "find_knights_data_dir.hpp"
#include "knights_config.hpp"
#include "knights_log.hpp"
#include "knights_server.hpp"
#include "metaserver_urls.hpp"
#include "my_exceptions.hpp"
#include "net_msgs.hpp"
#include "rng.hpp"
#include "rstream.hpp"
#include "version.hpp"

// coercri includes
#include "enet/enet_network_connection.hpp"
#include "enet/enet_network_driver.hpp"
#include "network/udp_socket.hpp"
#include "timer/generic_timer.hpp"

#include <curl/curl.h>

#include "boost/shared_ptr.hpp"
#include "boost/thread.hpp"

#include <algorithm>
#include <csignal>
#include <ctime>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// This is used when contacting the Metaserver
const char * user_agent_string = 
    "Knights-Server/" KNIGHTS_VERSION " (" KNIGHTS_PLATFORM "; " KNIGHTS_WEBSITE ")";


//
// Global Variables.
//

// The configuration
boost::shared_ptr<Config> g_config;

// The knights server. (Should only be accessed from main thread.)
boost::shared_ptr<KnightsServer> g_knights_server;

// The number of players connected to the knights server. Written by main thread, read by metaserver thread.
int g_num_players;
boost::mutex g_num_players_mutex;

// The network driver.
boost::shared_ptr<Coercri::NetworkDriver> net_driver;

// Incoming connections.
struct Conn {
    ServerConnection *server_conn;
    boost::shared_ptr<Coercri::NetworkConnection> remote;
};
std::vector<Conn> g_conns;

// UDP socket for replying to broadcasts.
boost::shared_ptr<Coercri::UDPSocket> broadcast_socket;
unsigned int broadcast_last_time = 0;

// Timer.
boost::shared_ptr<Coercri::Timer> timer;

// set to true when we receive SIGINT or SIGTERM
bool quit_flag = false;


//
// Load the KnightsConfig
//

boost::shared_ptr<KnightsConfig> LoadKnightsConfig()
{
    return boost::shared_ptr<KnightsConfig>(new KnightsConfig("main.lua", false));
}


//
// Background KnightsConfig loader thread.
// This makes sure there is always a "spare" KnightsConfig available
// for quickly starting new games.
// Lock the mutex before accessing knights_config.
//

struct KnightsConfigLoader
{
    boost::mutex mutex;
    boost::shared_ptr<KnightsConfig> knights_config;
    std::string err_msg;

    void operator()();
};

void KnightsConfigLoader::operator ()()
{
    try {
        while (1) {
            if (!knights_config) {
                boost::shared_ptr<KnightsConfig> cfg = LoadKnightsConfig();

                boost::lock_guard<boost::mutex> lock(mutex);
                knights_config = cfg;
            }

            boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        }
    } catch (std::exception &e) {
        boost::lock_guard<boost::mutex> lock(mutex);
        err_msg = e.what();
    } catch (...) {
        boost::lock_guard<boost::mutex> lock(mutex);
        err_msg = "<Unknown exception>";
    }
}


//
// LogWriter class -- does writing of log files in a separate thread.
// This is to ensure that I/O delays do not hold up the main
// thread(s).
//

class LogWriter {
public:
    explicit LogWriter(std::ostream &s) : str(s) { }

    void addText(const std::string &msg)
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        tbuf1.insert(tbuf1.end(), msg.begin(), msg.end());
    }

    void operator()()
    {
        // Loop forever (main thread will send us a thread_interrupted exception when it wants us to shut down).
        while (1) {
            // Periodically flush the log buffer to the output stream.
            boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
            flush();
        }
    }
    
    void flush()
    {
        // invariant: tbuf2 is empty at beginning and end of this routine.
        {
            boost::lock_guard<boost::mutex> lock(mutex);
            tbuf1.swap(tbuf2);
        }

        if (!tbuf2.empty()) {
            str.write(&tbuf2[0], tbuf2.size());
            str.flush();
            tbuf2.clear();
        }
    }
    
private:
    boost::mutex mutex;
    std::vector<char> tbuf1, tbuf2;

    // the output stream is written to ONLY by this thread.
    std::ostream &str;
};
    

//
// KnightsLog class. Prepends the time/date to all log messages sent.
// Log messages should be single lines (w/o newline character).
//

class MyLog : public KnightsLog {
public:
    explicit MyLog(LogWriter &w) : lw(w) { }

    virtual void logMessage(const std::string &msg);

private:
    std::string getTimeString();
    boost::mutex mutex;  // used to protect call to ctime in getTimeString

    LogWriter &lw;
};

void MyLog::logMessage(const std::string &msg)
{
    std::string m = getTimeString();
    m += "\t";
    m += msg;
    m += '\n';
    lw.addText(m);
}

std::string MyLog::getTimeString()
{
    std::time_t time_since_epoch = std::time(0);
    std::string result;
    {
        // ctime is not re-entrant, so must use mutex.
        boost::lock_guard<boost::mutex> lock(mutex);
        result = std::ctime(&time_since_epoch);
    }
    return result.substr(0, result.length()-1);  // remove final '\n'
}


//
// Signal Handler.
//

void SetQuitFlag(int)
{
    quit_flag = true;
}


//
// Network Functions.
//

bool ProcessOutgoingNetMsgs()
{
    bool did_something = false;
    std::vector<unsigned char> net_msg;

    for (std::vector<Conn>::iterator it = g_conns.begin(); it != g_conns.end(); ++it) {
        g_knights_server->setPingTime(*it->server_conn, it->remote->getPingTime());
    }

    for (std::vector<Conn>::iterator it = g_conns.begin(); it != g_conns.end(); ++it) {
        g_knights_server->getOutputData(*it->server_conn, net_msg);
        if (!net_msg.empty()) {
            did_something = true;
            it->remote->send(net_msg);
        }
    }

    return did_something;
}

bool ProcessIncomingNetMsgs()
{
    bool did_something = false;
    std::vector<unsigned char> net_msg;

    // Process existing connections
    for (int i = 0; i < int(g_conns.size()); /* incremented below */) {
        g_conns[i].remote->receive(net_msg);

        if (!net_msg.empty()) {
            g_knights_server->receiveInputData(*g_conns[i].server_conn, net_msg);
            did_something = true;
        }

        // Check to see if it has been closed by the remote host.
        const Coercri::NetworkConnection::State state = g_conns[i].remote->getState();
        ASSERT(state != Coercri::NetworkConnection::FAILED);  // incoming connections can't fail...
        if (state == Coercri::NetworkConnection::CLOSED) {
            g_knights_server->connectionClosed(*g_conns[i].server_conn);
            g_conns.erase(g_conns.begin() + i);
            did_something = true;
        } else {
            ++i;
        }
    }

    // Listen for new incoming connections
    Coercri::NetworkDriver::Connections new_conns = net_driver->pollIncomingConnections();
    for (Coercri::NetworkDriver::Connections::const_iterator it = new_conns.begin(); it != new_conns.end(); ++it) {
        Conn conn;
        conn.server_conn = &g_knights_server->newClientConnection((*it)->getAddress());
        conn.remote = *it;
        g_conns.push_back(conn);
        did_something = true;
    }

    return did_something;
}


//
// Report to Metaserver. (Done in a separate thread because it may block
// for an extended period.)
//

size_t DummyReadFunc(void *ptr, size_t size, size_t nmemb, void *stream)
{
    return 0;
}

size_t DummyWriteFunc(void *ptr, size_t size, size_t nmemb, void *my_log)
{
    // If the server is sending back data then it is probably an error message.
    // Print it on the log.
    std::string msg(static_cast<char*>(ptr), size * nmemb);

    static_cast<MyLog*>(my_log)->logMessage(msg);
    
    return size * nmemb;
}

struct CurlStuff {
    CURL *curl;

    CurlStuff()
        : curl(0)
    {
    }

    ~CurlStuff()
    {
        if (curl) curl_easy_cleanup(curl);
    }
};

int NonNegative(int x)
{
    return x >= 0 ? x : 0;
}

std::string Escape(CURL *curl, const std::string &x)
{
    char * escaped = curl_easy_escape(curl, x.c_str(), x.length());
    std::string result(escaped);
    curl_free(escaped);
    return result;
}

struct MetaserverThread {

    explicit MetaserverThread(MyLog &l) : my_log(l), prev_num_players(0) { }
    MyLog &my_log;
    int prev_num_players;
    char error_buffer[CURL_ERROR_SIZE];

    bool ReportToMetaserver(CurlStuff &curl_stuff, int num_players)
    {
        std::ostringstream str;

        str << "port=" << NonNegative(g_config->getPort());
        str << "&description=" << Escape(curl_stuff.curl, g_config->getDescription());
        str << "&num_players=" << NonNegative(num_players);
        str << "&password_required=" << (g_config->getPassword().empty() ? "0" : "1");
        
        prev_num_players = num_players;

        curl_easy_setopt(curl_stuff.curl, CURLOPT_COPYPOSTFIELDS, str.str().c_str());
        curl_easy_setopt(curl_stuff.curl, CURLOPT_URL, g_metaserver_update_url);

        my_log.logMessage(std::string("\tsending update to metaserver\t") + g_metaserver_update_url);

        const bool success = 
            curl_easy_perform(curl_stuff.curl) == 0;
        if (success) {
            my_log.logMessage("\tmetaserver update succeeded");
        } else {
            my_log.logMessage(std::string("\tmetaserver update failed\t") + error_buffer);
        }

        return success;
    }

    void RemoveFromMetaserver(CurlStuff &curl_stuff)
    {
        std::ostringstream str;
        str << "port=" << NonNegative(g_config->getPort());

        curl_easy_setopt(curl_stuff.curl, CURLOPT_COPYPOSTFIELDS, str.str().c_str());
        curl_easy_setopt(curl_stuff.curl, CURLOPT_URL, g_metaserver_remove_url);

        my_log.logMessage(std::string("\tremoving from metaserver\t") + g_metaserver_remove_url);
        
        const bool success =
            curl_easy_perform(curl_stuff.curl) == 0;
        if (success) {
            my_log.logMessage("\tremove from metaserver succeeded");
        } else {
            my_log.logMessage(std::string("\tremove from metaserver failed\t") + error_buffer);            
        }
    }

    void operator()()
    {
        try {

            CurlStuff curl_stuff;
            curl_stuff.curl = curl_easy_init();

            if (!curl_stuff.curl) {
                my_log.logMessage("\tcurl_easy_init failed. Disabling metaserver updates.");
                return;
            }

            curl_easy_setopt(curl_stuff.curl, CURLOPT_READFUNCTION, &DummyReadFunc);
            curl_easy_setopt(curl_stuff.curl, CURLOPT_WRITEFUNCTION, &DummyWriteFunc);
            curl_easy_setopt(curl_stuff.curl, CURLOPT_WRITEDATA, &my_log);
            curl_easy_setopt(curl_stuff.curl, CURLOPT_ERRORBUFFER, &error_buffer[0]);
            curl_easy_setopt(curl_stuff.curl, CURLOPT_USERAGENT, user_agent_string);
            curl_easy_setopt(curl_stuff.curl, CURLOPT_FOLLOWLOCATION, 1);
            curl_easy_setopt(curl_stuff.curl, CURLOPT_MAXREDIRS, 30);

            // Do initial report.
            unsigned int last_update_time = timer->getMsec();
            const bool success = ReportToMetaserver(curl_stuff, 0);
            int num_failed_updates = success ? 0 : 1;

            try {
            
                // Do further reports every 'long_interval' milliseconds, or whenever num_players changes.
                // 'short_interval' is used when there has been an error.
                const unsigned int long_interval = 10*60*1000;
                const unsigned int short_interval = 60*1000;
                
                while (1) {
                    const unsigned int time_now = timer->getMsec();
                    const unsigned int elapsed = time_now - last_update_time;

                    const bool use_short_interval = num_failed_updates > 0 && num_failed_updates <= 3;
                    const unsigned int interval = use_short_interval ? short_interval * num_failed_updates : long_interval;
                        
                    int num_players;
                    {
                        boost::lock_guard<boost::mutex> lock(g_num_players_mutex);
                        num_players = g_num_players;
                    }
                
                    if (elapsed >= interval || num_players != prev_num_players) {
                        const bool success = ReportToMetaserver(curl_stuff, num_players);
                        if (success) num_failed_updates = 0;
                        else ++num_failed_updates;
                        last_update_time = timer->getMsec();
                    }
                    
                    // Sleep for one second before making the next check.
                    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
                }
            } catch (boost::thread_interrupted&) {
                // fall through
            }

            // Now remove ourselves from the metaserver before leaving the thread.
            RemoveFromMetaserver(curl_stuff);

        } catch (boost::thread_interrupted&) {
            // this shouldn't happen.. but let it fall through.
        } catch (std::exception &e) {
            // this shouldn't happen
            my_log.logMessage(std::string("\tERROR: stopping metaserver updates: ") + e.what());
            return;
        } catch (...) {
            // this shouldn't happen
            my_log.logMessage("\tERROR: stopping metaserver updates: unknown exception caught");
            return;
        }
    }
};

void SetupBroadcastReplies(Coercri::NetworkDriver &net_driver)
{
    broadcast_socket = net_driver.createUDPSocket(BROADCAST_PORT, true);
    broadcast_last_time = 0;
}

bool DoBroadcastReplies()
{
    // don't run this more than once per second
    const unsigned int time_now = timer->getMsec();
    if (time_now - broadcast_last_time < 1000) return false;
    broadcast_last_time = time_now;

    // check for incoming msgs, send replies if necessary
    bool sent_reply = false;
    std::string msg, address;
    int port;
    while (broadcast_socket->receive(address, port, msg)) {
        // Received a client broadcast on the given address and port. Check whether it is a "PING"
        if (msg == BROADCAST_PING_MSG) {
            // It is. Send a reply back to the same address/port that the broadcast came from.
            const int num_players = g_knights_server->getNumberOfPlayers();
            std::string reply = BROADCAST_PONG_HDR;
            reply += static_cast<unsigned char>(g_config->getPort() >> 8);
            reply += static_cast<unsigned char>(g_config->getPort() & 0xff);
            reply += 'I';
            reply += static_cast<unsigned char>(num_players >> 8);
            reply += static_cast<unsigned char>(num_players & 0xff);
            reply += g_config->getDescription();
            broadcast_socket->send(address, port, reply);
            sent_reply = true;
        }
    }

    return sent_reply;
}


//
// Game Management
//

std::string GetNewGameName(const std::vector<GameInfo> &existing_games)
{
    // All games are of the form "Game N"
    // We find the lowest positive integer that has not already been taken
    std::set<int> game_nums;
    for (std::vector<GameInfo>::const_iterator it = existing_games.begin(); it != existing_games.end(); ++it) {
        std::istringstream str(it->game_name);
        std::string g;
        str >> g; // discard "Game"
        int n = 0;
        str >> n;
        game_nums.insert(n);
    }

    // find lowest available number.
    int n = 1;
    while (1) {
        if (game_nums.find(n) == game_nums.end()) {
            std::ostringstream str;
            str << "Game ";
            str << n;
            return str.str();
        }
        ++n;
    }
}

void CheckGames(KnightsConfigLoader &knights_config_loader)
{
    std::vector<GameInfo> game_infos = g_knights_server->getRunningGames();

    // Close any empty games beyond the first.
    bool found_empty_game = false;
    for (std::vector<GameInfo>::const_iterator it = game_infos.begin(); it != game_infos.end(); ++it) {
        if (it->num_players == 0) {
            if (!found_empty_game) {
                found_empty_game = true;
            } else {
                g_knights_server->closeGame(it->game_name);
            }
        }
    }

    const int max_games = g_config->getMaxGames();
    if (!found_empty_game && game_infos.size() < size_t(max_games)) {

        // See if a config is available yet
        boost::shared_ptr<KnightsConfig> config;
        {
            boost::lock_guard<boost::mutex> lock(knights_config_loader.mutex);

            // check for errors; re-throw them as InitError in the main thread.
            if (!knights_config_loader.err_msg.empty()) {
                throw InitError(knights_config_loader.err_msg);
            }

            config = knights_config_loader.knights_config;
            if (config) knights_config_loader.knights_config.reset(); // start loading the next config
        }

        if (config) {
            // Create a new game.
            g_knights_server->startNewGame(config, GetNewGameName(game_infos));
        }
    }
}


//
// RAII wrapper for curl
//

struct CurlWrapper {
    CurlWrapper() {
        // Initialize CURL
        // Note: don't initialize win32 socket libraries because that should already have been done by ENet.
        curl_global_init(CURL_GLOBAL_NOTHING);
    }

    ~CurlWrapper() {
        curl_global_cleanup();
    }
};


//
// Parse cmd line arguments
//

void PrintUsage(const std::string &program_name)
{
    std::cout << "Knights Server version " << KNIGHTS_VERSION << std::endl;
    std::cout << "Usage: " << program_name << " [-c config_file_name]" << std::endl;
    std::cout << "Default config file name (if -c is not given) is 'knights_config.txt'." << std::endl;
    std::cout << std::endl;
}

bool ParseArgs(int argc, char **argv, std::string &config_filename)
{
    config_filename = "knights_config.txt";  // default
    
    const std::string program_name = *argv++;
    char ** argv_end = argv + (argc - 1);

    while (argv != argv_end) {
        const std::string opt = *argv++;
        
        if (opt == "-c") {
            if (argv == argv_end) {
                PrintUsage(program_name);
                return false;
            }
            config_filename = *argv++;
            
        } else {
            // Unknown option.
            PrintUsage(program_name);
            return false;
        }
    }

    return true;
}


//
// Main Program.
//

int main(int argc, char **argv)
{
    std::string config_filename;

    // initialize rng
    g_rng.initialize();

    // Load the configuration. Exit if this fails.
    try {
        // Parse arguments
        bool success = ParseArgs(argc, argv, config_filename);
        if (!success) return 1;

        // Load the config file
        std::ifstream str(config_filename.c_str());
        if (!str) {
            std::cout << "Failed to open config file: " << config_filename << ". Exiting." << std::endl;
            return 1;
        }
        
        g_config.reset(new Config(str));
        
    } catch (const ConfigError &ce) {
        std::ostringstream msg;
        msg << "\t" << config_filename << ": line " << ce.getLine() << ": " << ce.getMessage() << ". Exiting.\n";
        std::cout << msg.str();
        return 1;
    } catch (...) {
        std::cout << "Error during initialization. Exiting.\n";
        return 1;
    }

    // Open log file if necessary
    std::unique_ptr<std::ostream> log_file_stream;
    if (!g_config->getLogFile().empty()) {
        log_file_stream.reset(new std::ofstream(g_config->getLogFile().c_str(), std::ios::out | std::ios::app));
        if (!*log_file_stream) {
            std::cout << "Failed to open log file: " << g_config->getLogFile() << ". Exiting.\n";
            return 1;
        }
    }

    // Send log msgs to log file, or to cout if no log file specified
    LogWriter log_writer(log_file_stream.get() ? *log_file_stream : std::cout);
    MyLog my_log(log_writer);
    
    // Start up the logging thread.
    boost::thread logging_thread(boost::ref(log_writer));

    
    // Set up timer.
    timer.reset(new Coercri::GenericTimer);

    try {

        // Get resource dir.
        boost::filesystem::path rdir;
        if (!g_config->getKnightsDataDir().empty()) {
            rdir = g_config->getKnightsDataDir();
        } else {
            rdir = FindKnightsDataDir();
        }
        my_log.logMessage(std::string("\tLoading data files from \"") 
                          + rdir.string() + std::string("\"."));
        RStream::Initialize(rdir);

        // Initialize CURL
        CurlWrapper curl_wrapper;
    
        // Write initial log messages.
        my_log.logMessage(std::string("\tKnights Server version ") + KNIGHTS_VERSION + " starting...");

        // Start the KnightsConfig loading thread
        KnightsConfigLoader knights_config_loader;
        boost::thread loader_thread(boost::ref(knights_config_loader));

        // Create the KnightsServer.
        g_knights_server.reset(new KnightsServer(timer, 
                                                 false,  // don't allow split screen
                                                 g_config->getMOTDFile(), g_config->getOldMOTDFile(),
                                                 g_config->getPassword()));
        g_knights_server->setKnightsLog(&my_log);
        
        // Set up Coercri network driver, and start listening for incoming connections.
        net_driver.reset(new Coercri::EnetNetworkDriver(g_config->getMaxPlayers(), 0, true));
        net_driver->setServerPort(g_config->getPort());
        net_driver->enableServer(true);

        // Start listening for broadcast messages (from clients looking for LAN servers)
        if (g_config->getReplyToBroadcast()) {
            SetupBroadcastReplies(*net_driver);
        }

        // Install our signal handler.
        std::signal(SIGINT, SetQuitFlag);
        std::signal(SIGTERM, SetQuitFlag);

        // We are up and running
        std::ostringstream msg_str;
        msg_str << "\tServer is now running on port " << g_config->getPort() << ".";
        my_log.logMessage(msg_str.str());

        // Start Metaserver thread if required.
        boost::thread metaserver_thread;
        MetaserverThread metaserver_thread_obj(my_log);
        if (g_config->getUseMetaserver()) {
            boost::thread new_thread(boost::ref(metaserver_thread_obj));
            metaserver_thread.swap(new_thread);
        }

        unsigned int last_misc_update = timer->getMsec();
        
        // Main Loop.
        while (!quit_flag) {
            bool did_something = false;
            int sleep_time = 10;

            // Gather outgoing messages.
            did_something = ProcessOutgoingNetMsgs() || did_something;

            // Process network events.
            while (net_driver->doEvents()) did_something = true;

            // Forward any incoming net messages.
            did_something = ProcessIncomingNetMsgs() || did_something;

            // Reply to any broadcast messages.
            if (g_config->getReplyToBroadcast()) {
                did_something = DoBroadcastReplies() || did_something;
            }

            // Misc Updates -- run every 1/10 second.
            const unsigned int time_now = timer->getMsec();
            if (time_now - last_misc_update > 100) {
                last_misc_update = time_now;

                // Spawn new games / clean up old games if needed.
                CheckGames(knights_config_loader);
            
                // Update the g_num_players variable
                const int num_players = g_knights_server->getNumberOfPlayers();
                {
                    boost::lock_guard<boost::mutex> lock(g_num_players_mutex);
                    g_num_players = num_players;
                }

                // if no players connected then increase sleep time.
                if (num_players == 0) sleep_time = 200;
            }

            // Sleep for a bit if nothing was done.
            if (!did_something) {
                timer->sleepMsec(sleep_time);
            }
        }

        my_log.logMessage("\tShutting down...");

        // interrupt the metaserver thread (this will trigger it to
        // send the remove command to the metaserver), and wait for it
        // to finish.
        if (g_config->getUseMetaserver()) {
            metaserver_thread.interrupt();
            metaserver_thread.join();
        }

        // interrupt the logging thread.
        logging_thread.interrupt();
        logging_thread.join();

    } catch (const std::exception &e) {
        my_log.logMessage(std::string("\tFatal Error: ") + e.what() + ". Exiting.");
    } catch (...) {
        my_log.logMessage("\tUnknown error! Exiting.");
    }

    // Shut everything down cleanly.

    // Clean up network connections
    for (std::vector<Conn>::iterator it = g_conns.begin(); it != g_conns.end(); ++it) {
        if (g_knights_server) g_knights_server->connectionClosed(*it->server_conn);
        it->remote->close();
    }
    g_conns.clear();

    // Shut down the server.
    g_knights_server.reset();
    
    // Clean up other objects.
    broadcast_socket.reset();
    if (net_driver) net_driver->enableServer(false);
    net_driver.reset();
    timer.reset();
    g_config.reset();

    // Flush any remaining log messages
    log_writer.flush();

    return 0;
}

// Fix "bug" with MSVC static libs + global object constructors.
#ifdef _MSC_VER
#pragma comment (linker, "/include:InitMagicActions")
#pragma comment (linker, "/include:InitScriptActions")
#pragma comment (linker, "/include:InitControls")
#endif

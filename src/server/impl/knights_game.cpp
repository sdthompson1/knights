/*
 * knights_game.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2024.
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

#include "misc.hpp"

#include "anim.hpp"
#include "graphic.hpp"
#include "knights_config.hpp"
#include "knights_engine.hpp"
#include "knights_game.hpp"
#include "knights_log.hpp"
#include "menu.hpp"
#include "my_ctype.hpp"
#include "my_menu_listeners.hpp"
#include "overlay.hpp"
#include "protocol.hpp"
#include "rng.hpp"
#include "rstream.hpp"
#include "server_callbacks.hpp"
#include "sh_ptr_eq.hpp"
#include "sound.hpp"
#include "user_control.hpp"

#include "boost/scoped_ptr.hpp"
#include "boost/weak_ptr.hpp"
#include "boost/thread/condition_variable.hpp"
#include "boost/thread/locks.hpp"
#include "boost/thread/thread.hpp"

#include <algorithm>
#include <set>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

using std::ios_base;
using std::string;
using std::vector;

// This is a hack for FastForwardUntil
bool g_hack_fast_forward_flag = false;

class GameConnection {
public:
    GameConnection(const UTF8String &n, const UTF8String &n2, bool new_obs_flag, int ver,
                   bool approach_based_ctrls, bool action_bar_ctrls)
        : name(n), name2(n2),
          is_ready(false), finished_loading(false), ready_to_end(false), 
          obs_flag(new_obs_flag), cancel_obs_mode_after_game(false), house_colour(0),
          client_version(ver),
          observer_num(0), player_num(-1),
          ping_time(0),
          speech_request(false), speech_bubble(false),
          approach_based_controls(approach_based_ctrls),
          action_bar_controls(action_bar_ctrls)
    { }
    
    UTF8String name, name2;  // if name2.empty() then normal connection, else split screen connection.
    
    bool is_ready;    // true=ready to start game, false=want to stay in lobby
    bool finished_loading;   // true=ready to play, false=still loading
    bool ready_to_end;   // true=clicked mouse on winner/loser screen, false=still waiting.
    bool obs_flag;   // true=observer, false=player
    bool cancel_obs_mode_after_game;
    int house_colour;  // Must fit in a ubyte. Set to zero for observers (but beware, zero is also a valid house colour for non-observers!).

    int client_version;
    int observer_num;   // 0 if not an observer, or not set yet. >0 if set.
    int player_num;     // 0..num_players-1, or -1 if the game is not running.
                        //  Also, observers have -1, unless they are eliminated players in which
                        //  case they retain their original player_num.
    int ping_time;
    
    std::vector<unsigned char> output_data;    

    std::vector<const UserControl*> control_queue[2];

    bool speech_request, speech_bubble;
    bool approach_based_controls;
    bool action_bar_controls;
};

typedef std::vector<boost::shared_ptr<GameConnection> > game_conn_vector;

class KnightsGameImpl {
public:
    boost::shared_ptr<KnightsConfig> knights_config;
    boost::shared_ptr<Coercri::Timer> timer;
    bool allow_split_screen;  // Whether to allow split screen and single player games.
    bool deathmatch_mode;

    std::vector<const UserControl*> controls;
    
    std::string quest_description;
    game_conn_vector connections;

    bool game_over;
    bool pause_mode;
    
    // methods of KnightsGame should (usually) lock this mutex.
    // the update thread will also lock this mutex when it is making changes to the KnightsGameImpl or GameConnection structures.
    boost::mutex my_mutex;

    boost::thread update_thread;
    volatile bool update_thread_wants_to_exit;
    volatile bool emergency_exit;  // lua_State is unusable; close the game asap.
    std::string emergency_err_msg;

    KnightsLog *knights_log;
    std::string game_name;

    std::vector<int> delete_observer_nums;
    std::vector<int> players_to_eliminate;
    std::vector<UTF8String> all_player_names;

    // used during game startup
    volatile bool startup_signal;
    std::string startup_err_msg;

    int msg_counter;  // used for logging when 'update' events occur.
    std::unique_ptr<std::deque<int> > update_counts;   // used for playback.
    std::unique_ptr<std::deque<int> > time_deltas;     // ditto
    std::unique_ptr<std::deque<unsigned int> > random_seeds; // ditto
    bool msg_count_update_flag;

    // Condition variable used to wake up the update thread when a control comes in
    // (instead of polling, like it used to do).
    // Note, this is controlled using the same mutex (my_mutex) as the rest of the data.
    boost::condition_variable wake_up_cond_var;
    bool wake_up_flag;
};

namespace {

    void CheckTeamChat(const std::string msg_orig, std::string &msg, bool &is_team)
    {
        is_team = false;

        // Left trim
        size_t idx = 0;
        while (idx < msg_orig.size() && msg_orig[idx] == ' ') ++idx;

        // Check for "/t"
        if (idx < msg_orig.size() && msg_orig[idx] == '/'
            && idx+1 < msg_orig.size() && msg_orig[idx+1] == 't') {
                idx += 2;
                is_team = true;
        }

        // Left trim again
        while (idx < msg_orig.size() && msg_orig[idx] == ' ') ++idx;

        // Copy rest of the message into msg
        msg = msg_orig.substr(idx);
    }

    void WaitForMsgCounter(KnightsGameImpl &kg)
    {
        if (kg.update_counts.get()) {
            while (1) {
                {
                    boost::lock_guard<boost::mutex> lock(kg.my_mutex);
                    if (kg.update_counts->empty() || kg.update_counts->front() > kg.msg_counter) {
                        // no update waiting
                        return;
                    }
                }
                // sleep until the update gets completed.
                boost::this_thread::sleep(boost::posix_time::milliseconds(1));
            }
        }
    }
    
    void Announcement(KnightsGameImpl &kg, const std::string &msg_latin1, bool is_err = false)
    {
        for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
            Coercri::OutputByteBuf buf((*it)->output_data);

            if (is_err) {
                buf.writeUbyte(SERVER_EXTENDED_MESSAGE);
                buf.writeVarInt(SERVER_EXT_NEXT_ANNOUNCEMENT_IS_ERROR);
                buf.writeUshort(0);
            }

            buf.writeUbyte(SERVER_ANNOUNCEMENT);
            buf.writeString(msg_latin1);
        }
    }

    void SendMessages(game_conn_vector &conns, const std::vector<std::string> &messages)
    {
        for (game_conn_vector::const_iterator it = conns.begin(); it != conns.end(); ++it) {
            Coercri::OutputByteBuf buf((*it)->output_data);
            for (std::vector<std::string>::const_iterator msg = messages.begin(); msg != messages.end(); ++msg) {
                buf.writeUbyte(SERVER_ANNOUNCEMENT);
                buf.writeString(*msg);
            }
        }
    }

    void DeactivateReadyFlags(KnightsGameImpl &kg)
    {
        for (game_conn_vector::iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
            (*it)->is_ready = false;
            (*it)->output_data.push_back(SERVER_DEACTIVATE_READY_FLAGS);
        }
    }
    
    int GetNumAvailHouseCols(const KnightsGameImpl &impl)
    {
        std::vector<Coercri::Color> hse_cols;
        impl.knights_config->getHouseColours(hse_cols);
        return hse_cols.size();
    }
    
    void SendAvailableHouseColours(const KnightsGameImpl &impl, Coercri::OutputByteBuf &buf)
    {
        std::vector<Coercri::Color> hse_cols;
        impl.knights_config->getHouseColours(hse_cols);
        buf.writeUbyte(SERVER_SET_AVAILABLE_HOUSE_COLOURS);

        const int n_hse_col = GetNumAvailHouseCols(impl);
        buf.writeUbyte(n_hse_col);
        
        for (int i = 0; i < n_hse_col; ++i) {
            buf.writeUbyte(hse_cols[i].r);
            buf.writeUbyte(hse_cols[i].g);
            buf.writeUbyte(hse_cols[i].b);
        }
    }        
    
    void SendJoinGameAccepted(KnightsGameImpl &impl, Coercri::OutputByteBuf &buf,
                              int my_house_colour)
    {
        // Send JOIN_GAME_ACCEPTED
        buf.writeUbyte(SERVER_JOIN_GAME_ACCEPTED);
        {
            std::vector<const Graphic*> graphics;
            impl.knights_config->getGraphics(graphics);
            buf.writeVarInt(graphics.size());
            for (size_t i = 0; i < graphics.size(); ++i) {
                ASSERT(graphics[i]->getID() == i+1);
                graphics[i]->serialize(buf);
            }
        }
        {
            std::vector<const Anim*> anims;
            impl.knights_config->getAnims(anims);
            buf.writeVarInt(anims.size());
            for (size_t i = 0; i < anims.size(); ++i) {
                ASSERT(anims[i]->getID() == i+1);
                anims[i]->serialize(buf);
            }
        }
        {
            std::vector<const Overlay*> overlays;
            impl.knights_config->getOverlays(overlays);
            buf.writeVarInt(overlays.size());
            for (size_t i = 0; i < overlays.size(); ++i) {
                ASSERT(overlays[i]->getID() == i+1);
                overlays[i]->serialize(buf);
            }
        }
        {
            std::vector<const Sound*> sounds;
            impl.knights_config->getSounds(sounds);
            buf.writeVarInt(sounds.size());
            for (size_t i = 0; i < sounds.size(); ++i) {
                ASSERT(sounds[i]->getID() == i+1);
                sounds[i]->serialize(buf);
            }
        }
        {
            std::vector<const UserControl*> controls;
            impl.knights_config->getStandardControls(controls);
            const int num_std_ctrls = controls.size();
            buf.writeVarInt(num_std_ctrls);
            for (int i = 0; i < num_std_ctrls; ++i) {
                ASSERT(controls[i]->getID() == i+1);
                controls[i]->serialize(buf);
            }
            impl.knights_config->getOtherControls(controls);
            buf.writeVarInt(controls.size());
            for (size_t i = 0; i < controls.size(); ++i) {
                ASSERT(controls[i]->getID() == i+1+num_std_ctrls);
                controls[i]->serialize(buf);
            }
        }
        impl.knights_config->getMenu().serialize(buf);
        buf.writeVarInt(impl.knights_config->getApproachOffset());

        // tell him whether he is an observer or a player, and the house colour
        buf.writeUbyte(my_house_colour);

        // tell him how many players there are, and their names
        int np = 0;
        for (game_conn_vector::const_iterator it = impl.connections.begin(); it != impl.connections.end(); ++it) {
            if (!(*it)->obs_flag) {
                ++np;
                if (!(*it)->name2.empty()) {
                    if (impl.connections.size() > 1) throw ProtocolError("split screen not supported for network games!");
                    ++np;
                }
            }
        }

        buf.writeVarInt(np);

        for (game_conn_vector::const_iterator it = impl.connections.begin(); it != impl.connections.end(); ++it) {
            if (!(*it)->obs_flag) {
                buf.writeString((*it)->name.asUTF8());
                buf.writeUbyte((*it)->is_ready ? 1 : 0);
                buf.writeUbyte((*it)->house_colour);
                if (!(*it)->name2.empty()) {
                    buf.writeString((*it)->name2.asUTF8());
                    buf.writeUbyte((*it)->is_ready ? 1 : 0);
                    buf.writeUbyte((*it)->house_colour + 1);
                }
            }
        }

        // tell him who the observers are
        // std::max necessary because connections.size()==1 but np==2 in split screen mode.
        buf.writeVarInt(std::max(0, int(impl.connections.size()) - np));
        for (game_conn_vector::const_iterator it = impl.connections.begin(); it != impl.connections.end(); ++it) {
            if ((*it)->obs_flag) {
                ASSERT((*it)->name2.empty());
                buf.writeString((*it)->name.asUTF8());
            }
        }
        
        // Send the current menu selections
        MyMenuListener listener;
        listener.addBuf(buf);
        impl.knights_config->getCurrentMenuSettings(listener);

        // Send the available knight house colours
        SendAvailableHouseColours(impl, buf);
    }

    int CountPlayers(const KnightsGameImpl &kg)
    {
        int nplayers = 0;
        for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
            if (!(*it)->obs_flag) {
                ++nplayers;
                if (!(*it)->name2.empty()) ++nplayers;
            }
        }
        return nplayers;
    }

    int CountTeams(const KnightsGameImpl &kg)
    {
        std::set<int> teams;
        for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
            if (!(*it)->obs_flag) {
                // not an observer
                if (!(*it)->name2.empty()) {
                    // split screen mode. the house colours are hard coded.
                    teams.insert(0);
                    teams.insert(1);
                } else {
                    teams.insert((*it)->house_colour);
                }
            }
        }
        return int(teams.size());
    }

    void UpdateNumPlayersAndTeams(KnightsGameImpl &kg)
    {
        MyMenuListener listener;
        for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
            listener.addBuf((*it)->output_data);
        }
        kg.knights_config->changeNumberOfPlayers(CountPlayers(kg), CountTeams(kg), listener);
    }
    
    void ReturnToMenu(KnightsGameImpl &kg)
    {
        // This routine sends SERVER_GOTO_MENU to all players, and
        // also does one or two other cleanup tasks at the end of a
        // game.
        
        // If the update thread is running then the mutex should be
        // LOCKED when calling this routine.
        
        for (game_conn_vector::iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
            (*it)->output_data.push_back(SERVER_GOTO_MENU);
            (*it)->observer_num = 0;
            (*it)->player_num = -1;
            if ((*it)->cancel_obs_mode_after_game) (*it)->obs_flag = false;
            (*it)->cancel_obs_mode_after_game = false;
        }

        // Take it out of pause mode if necessary
        kg.pause_mode = false;
        
        // Log end of game.
        if (kg.knights_log) {
            kg.knights_log->logMessage(kg.game_name + "\tgame ended");
        }
    }
    
    class UpdateThread {
    public:
        UpdateThread(KnightsGameImpl &kg_, boost::shared_ptr<Coercri::Timer> timer_)
            : kg(kg_),
              timer(timer_),
              nplayers(0),
              game_over_sent(false),
              time_to_player_list_update(0),
              prev_total_skulls(0),
              time_to_force_quit(0)
        {
            // note: mutex is locked at this point
        }

        void operator()()
        {
            try {

                // Initialize the game.
                // NOTE: Main thread is waiting for us to set kg.startup_signal.
                // We can do whatever we want to kg (without needing to lock it) before we set that flag.
                // After the flag is set, need to lock kg before accessing it.

                // create the random number generator for this thread
                g_rng.initialize();

                unsigned int seed;

                if (kg.random_seeds.get() && !kg.random_seeds->empty()) {
                    seed = kg.random_seeds->front();
                    kg.random_seeds->pop_front();
                } else {
                    // construct seed from current time and also address of
                    // this thread object (just in case two games are started
                    // at nearly the same time).
                    seed = static_cast<unsigned int>(reinterpret_cast<intptr_t>(this));
                    seed += static_cast<unsigned int>(time(0));
                    seed += timer->getMsec();
                }

                g_rng.setSeed(seed);
                
                // log what the seed was.
                if (kg.knights_log) kg.knights_log->logBinary("RNG", static_cast<int>(seed), 
                                                              kg.game_name.length(), kg.game_name.c_str());

                // Setup house colours and player names
                // Also count how many players on each team.
                std::vector<int> hse_cols;
                std::vector<UTF8String> player_names;
                std::map<int, int> team_counts;
                for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                    if (!(*it)->obs_flag) {
                        int col = (*it)->house_colour;
                        hse_cols.push_back(col);
                        ++team_counts[col];
                        player_names.push_back((*it)->name);
                        
                        if (!(*it)->name2.empty()) {
                            ++col;  // split screen mode: house colours are consecutive (0 and 1)
                            hse_cols.push_back(col);
                            ++team_counts[col];
                            player_names.push_back((*it)->name2);
                        }
                    }
                }

                kg.all_player_names = player_names;
                
                if (player_names.size() == 2 && kg.connections.size() == 1) {
                    // Split screen game.
                    // Disable the player names in this case.
                    for (size_t i = 0; i < player_names.size(); ++i) { player_names[i] = UTF8String(); }
                }

                nplayers = player_names.size();

                // Create the KnightsEngine. Pass any messages back to the players
                std::vector<std::string> messages;
                try {
                    engine.reset(new KnightsEngine(kg.knights_config, hse_cols, player_names,
                                                   kg.deathmatch_mode,  // output from KnightsEngine
                                                   messages));   // output from KnightsEngine

                } catch (LuaPanic &) {
                    // This is serious enough that we re-throw and let 
                    // the game close itself down.
                    throw;
                    
                } catch (const std::exception &e) {

                    SendMessages(kg.connections, messages);
                    
                    kg.startup_err_msg = e.what();
                    if (kg.startup_err_msg.empty()) kg.startup_err_msg = "Unknown error";
                    kg.update_thread_wants_to_exit = true;
                    return;
                }

                for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                    if (!(*it)->obs_flag) {
                        for (int p = 0; p < ((*it)->name2.empty() ? 1 : 2); ++p) {
                            engine->setApproachBasedControls((*it)->player_num + p, (*it)->approach_based_controls);
                            engine->setActionBarControls((*it)->player_num + p, (*it)->action_bar_controls);
                        }
                    }
                }
                
                callbacks.reset(new ServerCallbacks(hse_cols.size()));
                
                // The game has started successfully so signal the main thread to continue
                kg.startup_signal = true;
                
                // Wait until all players have set the 'finished_loading' flag
                while (1) {
                    {
                        boost::lock_guard<boost::mutex> lock(kg.my_mutex);
                        bool all_loaded = true;
                        for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                            if (!(*it)->obs_flag && !(*it)->finished_loading) {
                                all_loaded = false;
                                break;
                            }
                        }
                        if (all_loaded) break;
                    }
                    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
                }

                {
                    boost::lock_guard<boost::mutex> lock(kg.my_mutex);

                    // Send through any initialization msgs from lua.
                    SendMessages(kg.connections, messages);

                    // Send the team chat notification (but only if more than 1 player on the team; #151)
                    for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                        Coercri::OutputByteBuf buf((*it)->output_data);
                        
                        if (!(*it)->obs_flag && team_counts[(*it)->house_colour] > 1) {
                            buf.writeUbyte(SERVER_ANNOUNCEMENT);
                            buf.writeString("Note: Team chat is available. Type /t at the start of your message to send to your team only.");
                        }
                    }
                }

                // Go into game loop. NOTE: This will run forever until the main thread interrupts us
                // (or an exception occurs, or update() returns false).
                int last_time = timer->getMsec();
                int time_now = last_time;
                while (1) {
                    // work out how long since the last update
                    const int update_delta_t = time_now - last_time;

                    // run an update
                    const bool should_continue = update(update_delta_t);
                    if (!should_continue) break;   // End of game
                    last_time += update_delta_t;

                    // schedule the next update
                    const int max_time_to_update = 250;
                    const unsigned int time_of_next_update =
                        g_hack_fast_forward_flag
                        ?
                        timer->getMsec()
                        :
                        last_time + std::min(max_time_to_update, engine->getTimeToNextUpdate());
                    
                    // Sleep until main thread signals us, or until we reach time_of_next_update
                    while (1)
                    {
                        // Update time_now, and work out how long we need to wait
                        time_now = timer->getMsec();
                        int wait_time = time_of_next_update - time_now;
                        
                        if (wait_time <= 0) break;  // The update has arrived!

                        // OK, so we need to wait for a bit. Lock mutex
                        boost::unique_lock<boost::mutex> lock(kg.my_mutex);

                        // The call to timed_wait UNLOCKS the mutex, WAITS for either the time to expire, or
                        // the main loop to signal us, then LOCKS the mutex again before returning.
                        kg.wake_up_cond_var.timed_wait(lock, 
                            boost::posix_time::milliseconds(wait_time));

                        if (kg.wake_up_flag) {
                            // The reason that timed_wait() returned was that the main loop signalled us.
                            // Acknowledge the signal (by clearing the flag), then break (which will force
                            // the next update -- as well as releasing the mutex).
                            kg.wake_up_flag = false;
                            break;
                        }

                        // Go round again.
                        // If the return from timed_wait was spurious, then the "if wait_time<=0" check will
                        // fail and we will go round again.
                        // If the return from timed_wait was because the timer expired, then the "if" will
                        // succeed and we will break out, and do the next update.
                    }
                }
                
            } catch (boost::thread_interrupted &) {
                // Allow this to go through. The code that interrupted us knows what it's doing...

            } catch (LuaPanic &e) {
                // If this happens we need to exit game asap
                kg.emergency_exit = true;
                kg.emergency_err_msg = e.what();
                std::string msg = std::string("Fatal Lua error: ") + e.what();
                sendError(msg.c_str());
                
            } catch (std::exception &e) {
                sendError(e.what());
            } catch (...) {
                sendError("Unknown error in update thread");
            }

            // Before we go, delete the KnightsEngine. This will make sure that the Mediator
            // is still around while the KnightsEngine destructor runs.
            engine.reset();  // Never throws

            // Tell the main thread that the update thread wants to exit.
            // Note no need to lock mutex because there is only one writer (us) and one reader (the main thread)
            kg.update_thread_wants_to_exit = true;
        }

        void sendError(const char * msg) throw()
        {
            try {
                // send error to all players connected to this game.
                boost::lock_guard<boost::mutex> lock(kg.my_mutex);
                for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                    Coercri::OutputByteBuf buf((*it)->output_data);
                    buf.writeUbyte(SERVER_ERROR);
                    buf.writeString(msg);
                }
                // log the error as well.
                if (kg.knights_log) {
                    kg.knights_log->logMessage(kg.game_name + "\tError in update thread\t" + msg);
                }
            } catch (...) {
                // Disregard any exceptions here, as we don't want them to propagate up into the
                // boost::thread library internals (this would terminate the entire server if it happened).
            }
        }

        // Returns true if the game should continue or false if we should quit.
        // Note, if returning false, update() is responsible for sending out SERVER_GOTO_MENU (or whatever
        // else it wants to do).
        bool update(int time_delta)
        {
            int msg_counter;  // find out the value of the msg counter at the time when we read the controls (etc).

            // read controls (and see if paused)
            {
                boost::lock_guard<boost::mutex> lock(kg.my_mutex);

                msg_counter = kg.msg_counter;
                
                // Delete any "dead" observers.
                for (std::vector<int>::iterator it = kg.delete_observer_nums.begin(); it != kg.delete_observer_nums.end(); ++it) {
                    callbacks->rmObserverNum(*it);
                }
                kg.delete_observer_nums.clear();
                
                // Delete any disconnected players
                for (std::vector<int>::iterator it = kg.players_to_eliminate.begin(); it != kg.players_to_eliminate.end(); ++it) {
                    engine->eliminatePlayer(*it);
                }
                kg.players_to_eliminate.clear();

                // Put eliminated players into obs mode (Trac #10)
                std::vector<int> put_into_obs_mode = callbacks->getPlayersToPutIntoObsMode();
                for (std::vector<int>::const_iterator p = put_into_obs_mode.begin(); p != put_into_obs_mode.end(); ++p) {
                    for (game_conn_vector::iterator c = kg.connections.begin(); c != kg.connections.end(); ++c) {
                        if ((*c)->player_num == *p) {
                            // Send him the 'go into obs mode' command
                            Coercri::OutputByteBuf buf((*c)->output_data);
                            buf.writeUbyte(SERVER_GO_INTO_OBS_MODE);
                            buf.writeUbyte(kg.all_player_names.size());
                            for (std::vector<UTF8String>::const_iterator it = kg.all_player_names.begin(); it != kg.all_player_names.end(); ++it) {
                                buf.writeString(it->asUTF8());
                            }

                            // Set his obs_num to zero (which means "observer num not allocated yet")
                            // and his obs_flag to true. (Leave player_num unchanged.)
                            (*c)->obs_flag = true;
                            (*c)->observer_num = 0;
                            (*c)->cancel_obs_mode_after_game = true;
                        }
                    }
                }
                
                // we assume that allow_split_screen means this is a local game
                // (and therefore pause should be allowed)
                if (kg.pause_mode && kg.allow_split_screen) {
                    return true;  // ignore the update
                }

                // Check whether we are in "replay" mode, and if so, don't allow the update if the msg_counter hasn't reached
                // the correct value yet.
                if (kg.update_counts.get()) {
                    if (kg.update_counts->empty() || kg.update_counts->front() > msg_counter) {
                        boost::this_thread::interruption_point(); // ensure main thread can interrupt us
                        return true;  // ignore the update
                    } else {
                        kg.update_counts->pop_front();

                        // overwrite the time delta
                        time_delta = kg.time_deltas->front();
                        kg.time_deltas->pop_front();
                    }
                }

                for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                    if (!(*it)->obs_flag) {
                        for (int p = 0; p < ((*it)->name2.empty() ? 1 : 2); ++p) {

                            const UserControl *final_ctrl = 0;

                            for (std::vector<const UserControl*>::const_iterator c = (*it)->control_queue[p].begin();
                            c != (*it)->control_queue[p].end(); ++c) {
                                final_ctrl = *c;
                                engine->setControl((*it)->player_num + p, *c);
                            }
                            (*it)->control_queue[p].clear();

                            // if the final control is continuous then re-insert it into the queue for next time.
                            if (final_ctrl && final_ctrl->isContinuous()) {
                                (*it)->control_queue[p].push_back(final_ctrl);
                            }

                            // speech bubble handling.
                            if ((*it)->speech_request) {
                                (*it)->speech_request = false;
                                engine->setSpeechBubble((*it)->player_num, (*it)->speech_bubble);
                            }
                        }
                    } else {
                        // He is an observer. If he has not been allocated an observer_num
                        // yet then give him one, and send him the startup commands. (but only if
                        // he has finished loading.)
                        if ((*it)->observer_num == 0 && (*it)->finished_loading) {
                            std::vector<unsigned char> & buf = (*it)->output_data;

                            for (int p = 0; p < nplayers; ++p) {
                                buf.push_back(SERVER_SWITCH_PLAYER);
                                buf.push_back(p);

                                // This will send mini map and status display updates to ALL connected players/observers
                                // (even those who already have this data).
                                //
                                // (For the tile/item updates, there is an optimization in ViewManager -- the "force" 
                                // flag -- which should mean that only the new observer gets these updates, at least; 
                                // although I haven't explicitly tested this.)
                                //
                                // A previous version of this code used a hack to try to avoid the unnecessary updates,
                                // but that caused a bug (Trac #115), so it has been removed, and we just accept the 
                                // unnecessary updates for now.
                                engine->catchUp(p, *callbacks);
                            }
                            
                            (*it)->observer_num = callbacks->allocObserverNum();
                        }
                    }
                }
            }
            
            // run the update
            engine->update(time_delta, *callbacks);

            // log the msg_counter at the time of the update, and the time delta
            // note: int_arg stores the msg counter
            // we store the time delta in the string part.
            if (kg.knights_log) {
                std::vector<char> dat;
                dat.resize(kg.game_name.length() + sizeof(time_delta));
                *reinterpret_cast<int*>(&dat[0]) = time_delta;
                std::copy(kg.game_name.begin(), kg.game_name.end(), &dat[sizeof(time_delta)]);
                kg.knights_log->logBinary("UPD", msg_counter, dat.size(), &dat[0]);
            }

            // copy the results back to the GameConnection buffers
            {
                boost::lock_guard<boost::mutex> lock(kg.my_mutex);
                for (game_conn_vector::iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                    if ((*it)->obs_flag) {
                        if ((*it)->observer_num > 0) {
                            // Send observer cmds to that player. (We're assuming observation is always allowed at the moment.)
                            callbacks->appendObserverCmds((*it)->observer_num, (*it)->output_data);
                        }
                    } else {
                        const bool split_screen = !(*it)->name2.empty();
                        if (split_screen) {
                            // Select player 0 for split screen mode
                            (*it)->output_data.push_back(SERVER_SWITCH_PLAYER);
                            (*it)->output_data.push_back(0);
                        }

                        const int buf_size_before_p0 = (*it)->output_data.size();
                        callbacks->appendPlayerCmds((*it)->player_num, (*it)->output_data);

                        if (split_screen) {
                            if ((*it)->output_data.size() == buf_size_before_p0) {
                                // can remove the SWITCH_PLAYER 0 cmd
                                (*it)->output_data.pop_back();
                                (*it)->output_data.pop_back();
                            }

                            // Select player 1 for split screen mode
                            (*it)->output_data.push_back(SERVER_SWITCH_PLAYER);
                            (*it)->output_data.push_back(1);

                            // Send player 1's data
                            const int buf_size_before_p1 = (*it)->output_data.size();
                            callbacks->appendPlayerCmds((*it)->player_num + 1, (*it)->output_data);

                            if ((*it)->output_data.size() == buf_size_before_p1) {
                                // can remove the SWITCH_PLAYER 1 cmd
                                (*it)->output_data.pop_back();
                                (*it)->output_data.pop_back();
                            }
                        }
                    }
                }
                callbacks->clearCmds();
            }

            // do player list update every N seconds, or if someone has died (total skulls has changed)
            const int total_skulls = engine->getSkullsPlusKills();
            time_to_player_list_update -= time_delta;
            if (time_to_player_list_update <= 0 || total_skulls != prev_total_skulls) {

                std::vector<PlayerInfo> player_list;
                engine->getPlayerList(player_list);

                boost::lock_guard<boost::mutex> lock(kg.my_mutex);

                // first job: remove any players who don't have a corresponding connection.
                // (e.g. this can happen if a player disconnects after they are eliminated from the game.)
                for (size_t idx = 0; idx < player_list.size(); ++idx) {
                    const int num = player_list[idx].player_num;
                    bool found = false;
                    for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                        if ((*it)->player_num == num) {
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        ++idx;
                    } else {
                        player_list.erase(player_list.begin() + idx);
                    }
                }

                // associate a ping time with each player
                // -- Only do this if the timer ran out, not if we are updating following a death,
                // otherwise it looks strange when the pings update just because someone died.
                if (time_to_player_list_update <= 0) {
                    for (size_t idx = 0; idx < player_list.size(); ++idx) {
                        const int num = player_list[idx].player_num;
                        for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                            if ((*it)->player_num == num) {
                                pings[(*it)->name] = (*it)->ping_time;
                            }
                        }
                    }
                }

                // any eliminated player should have (Eliminated) after their name
                for (size_t idx = 0; idx < player_list.size(); ++idx) {
                    if (player_list[idx].eliminated) {
                        player_list[idx].name += UTF8String::fromUTF8(" (Eliminated)");
                    }
                }

                // now add observers to the list (Trac #26)
                for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                    if ((*it)->player_num == -1) {
                        ASSERT((*it)->obs_flag);  // when game is running, player_num == -1 implies obs_flag
                        PlayerInfo pi;
                        pi.name = (*it)->name + UTF8String::fromUTF8(" (Observer)");
                        pi.house_colour = Coercri::Color(0,0,0);
                        pi.player_num = -1;
                        pi.kills = -1;
                        pi.deaths = -1;
                        pi.frags = -1000;
                        pi.eliminated = false;
                        player_list.push_back(pi);
                        pings[pi.name] = (*it)->ping_time;
                    }
                }

                // get the time remaining as well.
                const int time_remaining = engine->getTimeRemaining();

                // now send out the updates
                for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                    Coercri::OutputByteBuf buf((*it)->output_data);
                    buf.writeUbyte(SERVER_PLAYER_LIST);
                    buf.writeVarInt(player_list.size());
                    for (size_t idx = 0; idx < player_list.size(); ++idx) {
                        buf.writeString(player_list[idx].name.asUTF8());
                        buf.writeUbyte(player_list[idx].house_colour.r);
                        buf.writeUbyte(player_list[idx].house_colour.g);
                        buf.writeUbyte(player_list[idx].house_colour.b);
                        buf.writeVarInt(player_list[idx].kills);
                        buf.writeVarInt(player_list[idx].deaths);
                        buf.writeVarInt(player_list[idx].frags);
                        buf.writeVarInt(pings[player_list[idx].name]);
                    }

                    if (time_remaining > -1) {
                        buf.writeUbyte(SERVER_TIME_REMAINING);
                        buf.writeVarInt(time_remaining);
                    }
                }

                // Reset the update timers etc
                prev_total_skulls = total_skulls;
                if (time_to_player_list_update <= 0) time_to_player_list_update = 3000;  // 3 seconds
            }

            // poll to see if the game is over.
            if (!game_over_sent && callbacks->isGameOver()) {
                boost::lock_guard<boost::mutex> lock(kg.my_mutex);
                kg.game_over = true;
                game_over_sent = true;
                time_to_force_quit = 60000;
                
                // find names of all players, and the winner
                std::vector<UTF8String> loser_names;
                UTF8String winner_name;
                for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                    if (!(*it)->obs_flag) {
                        if ((*it)->player_num == callbacks->getWinnerNum()) {
                            winner_name = (*it)->name;
                        } else {
                            loser_names.push_back((*it)->name);
                        }
                        // note: name2 not supported here currently.
                    }
                }
                
                if (kg.knights_log) {
                    std::string msg;
                    msg = kg.game_name + "\tgame won\t";
                    // the first name printed is the winner, then we list the losers (of which there will be
                    // only one at the moment).
                    msg += "winner=" + winner_name.asUTF8();
                    for (std::vector<UTF8String>::const_iterator it = loser_names.begin(); it != loser_names.end(); ++it) {
                        msg += std::string(", loser=") + it->asUTF8();
                    }
                    kg.knights_log->logMessage(msg);
                }
            }

            // 'force quit' countdown. this forces the winner/loser screen to close after a certain time (Trac #7)
            if (time_to_force_quit > 0) {
                time_to_force_quit -= time_delta;
                if (time_to_force_quit <= 0) {
                    boost::lock_guard<boost::mutex> lock(kg.my_mutex);
                    ReturnToMenu(kg);
                    return false;  // force game loop to quit
                }
            }

            return true;  // game loop should continue
        }

    private:
        KnightsGameImpl &kg;
        boost::shared_ptr<Coercri::Timer> timer;
        boost::shared_ptr<ServerCallbacks> callbacks;
        boost::shared_ptr<KnightsEngine> engine;
        std::map<UTF8String, int> pings;
        int nplayers;
        bool game_over_sent;
        int time_to_player_list_update;
        int prev_total_skulls;
        int time_to_force_quit;
    };

    void DoSetReady(KnightsGameImpl &kg, GameConnection &conn, bool ready)
    {
        conn.is_ready = ready;

        // send notification to players
        for (game_conn_vector::iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
            Coercri::OutputByteBuf out((*it)->output_data);
            out.writeUbyte(SERVER_SET_READY);
            out.writeString(conn.name.asUTF8());
            out.writeUbyte(ready ? 1 : 0);
            if (!conn.name2.empty()) {
                out.writeUbyte(SERVER_SET_READY);
                out.writeString(conn.name2.asUTF8());
                out.writeUbyte(ready ? 1 : 0);
            }
        }
    }
    
    struct CmpByName {
        bool operator()(const boost::shared_ptr<GameConnection> &lhs,
                        const boost::shared_ptr<GameConnection> &rhs) const
        {
            return lhs->name.toUpper() < rhs->name.toUpper();
        }
    };

    void StartGameIfReady(KnightsGameImpl &kg)
    {
        // mutex is UNLOCKED at this point

        if (kg.update_thread.joinable()) return;  // Game is already started.

        // game thread definitely not running at this point... so it is safe to access kg
        
        std::vector<UTF8String> names;
        
        // count how many players are ready to leave the lobby, and how many there are total
        int nready = 0;
        int nplayers = 0;
        for (game_conn_vector::iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
            if (!(*it)->obs_flag) {
                (*it)->player_num = nplayers;  // assign player_nums
                const int ncount = (*it)->name2.empty() ? 1 : 2;
                nplayers += ncount;
                if ((*it)->is_ready) {
                    nready += ncount;
                    names.push_back((*it)->name);
                    if (ncount==2) names.push_back((*it)->name2);
                }
            }
        }

        // If all players are ready then start the game.
        bool ready_to_start = (nready == nplayers && nplayers >= 1);

        // Don't start if there are insufficient players for the current quest
        std::string nplyrs_err_msg;
        if (ready_to_start && !kg.knights_config->checkNumPlayersStrict(nplyrs_err_msg)) {
            Announcement(kg, nplyrs_err_msg, true);
            ready_to_start = false;
        }

        // See if we are still ready to start
        if (ready_to_start) {

            // Clear everybody's "finished_loading" flag. Assign player nums.
            for (game_conn_vector::iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                (*it)->finished_loading = false;
                (*it)->ready_to_end = false;
                
                // clear all ready flags when the game starts.
                (*it)->is_ready = false;
            }

            // Clear 'players_to_eliminate' in case there is anything still in there from the previous game
            kg.players_to_eliminate.clear();
            
            // Start the update thread.
            kg.update_thread_wants_to_exit = false;
            kg.emergency_exit = false;
            kg.emergency_err_msg.clear();
            kg.startup_signal = false;
            kg.startup_err_msg.clear();
            UpdateThread thr(kg, kg.timer);
            boost::thread new_thread(thr);
            kg.update_thread.swap(new_thread);  // start the sub-thread -- game is now running.

            // Wait for the sub thread to set the "startup_signal" flag.
            while (1) {
                if (kg.startup_signal) {
                    // The sub-thread is ready to proceed
                    break;
                }

                if (kg.update_thread_wants_to_exit) {
                    // The sub-thread has exited. This usually signals an error of some sort.
                    kg.update_thread.join();

                    if (kg.emergency_exit) {
                        // very serious error -- escalate to our caller.
                        throw LuaError("Fatal Lua error during startup: " + kg.emergency_err_msg);
                    }
                    
                    if (kg.startup_err_msg.empty()) kg.startup_err_msg = "Update thread failed to start.";
                    break;
                }

                // sleep for a bit while we wait.
                boost::this_thread::sleep(boost::posix_time::milliseconds(100));  
            }

            // Lock the mutex
            boost::lock_guard<boost::mutex> lock(kg.my_mutex);
            
            // If the game failed to initialize then print a message and refuse to start the game.
            // Otherwise, send startup message to all players.
            const std::string err_msg = kg.startup_err_msg;
            if (!err_msg.empty()) {
                Announcement(kg, std::string("Couldn't start game! ") + err_msg, true);

                // log the error as well
                if (kg.knights_log) {
                    kg.knights_log->logMessage(kg.game_name + "\tError starting game\t" + err_msg);
                }
                
                kg.update_thread.join();

            } else {
                // No errors -- We can start the game now

                // Send startGame msgs
                for (game_conn_vector::iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                    
                    unsigned char num_displays;
                    if (!(*it)->name2.empty()) num_displays = 2;  // split screen mode
                    else if ((*it)->obs_flag) num_displays = nplayers;  // observer mode
                    else num_displays = 1;   // normal mode

                    if (!(*it)->obs_flag) {
                        // Player.
                        Coercri::OutputByteBuf buf((*it)->output_data);
                        buf.writeUbyte(SERVER_START_GAME);
                        buf.writeUbyte(num_displays);
                        buf.writeUbyte(kg.deathmatch_mode);
                    } else {
                        // Observer.
                        Coercri::OutputByteBuf buf((*it)->output_data);
                        buf.writeUbyte(SERVER_START_GAME_OBS);
                        buf.writeUbyte(num_displays);
                        buf.writeUbyte(kg.deathmatch_mode);
                        for (int i = 0; i < num_displays; ++i) {
                            buf.writeString(names[i].asUTF8());
                        }
                        buf.writeUbyte(0);  // already_started flag (false)
                    }
                }
            
                // Log a message if required.
                if (kg.knights_log) {
                    std::ostringstream str;
                    str << kg.game_name << "\tgame started\t";
                    for (std::vector<UTF8String>::const_iterator it = names.begin(); it != names.end(); ++it) {
                        if (it != names.begin()) str << ", ";
                        str << it->asUTF8();
                    }

                    LogMenuListener listener(str);
                    kg.knights_config->getCurrentMenuSettings(listener);
                    
                    kg.knights_log->logMessage(str.str());
                }
            }
        }
    }

    void StopGameAndReturnToMenu(KnightsGameImpl &kg)
    {
        // Interrupt the thread and tell all players to return to menu.
        // Called at game over (after all players have clicked mouse to continue)
        // or when somebody presses escape.

        // NOTE: Mutex should be UNLOCKED when calling this routine, otherwise we will get a 
        // deadlock if the update thread is currently waiting for the mutex. (Apparently
        // a thread can't be interrupted while it is waiting for a mutex.)
        
        kg.update_thread.interrupt();
        kg.update_thread.join();

        ReturnToMenu(kg);
    }

    void SendFile(Coercri::OutputByteBuf &buf, const FileInfo &fi)
    {
        // note: this will throw if file not found
        RStream str(fi.getPath());

        // get size
        str.seekg(0, ios_base::end);
        size_t filesize = str.tellg();
        str.seekg(0, ios_base::beg);

        // write size
        buf.writeVarInt(filesize);
        
        // write file contents
        size_t ct = 0;
        while (str) {
            char c;
            str.get(c);
            if (str) {
                buf.writeUbyte(static_cast<unsigned char>(c));
                ++ct;
            }
        }

        if (ct != filesize) {
            throw std::runtime_error("Could not read file (on server): " + fi.getPath().generic_string());
        }
    }
}

KnightsGame::KnightsGame(boost::shared_ptr<KnightsConfig> config,
                         boost::shared_ptr<Coercri::Timer> tmr,
                         bool allow_split_screen,
                         KnightsLog *knights_log,
                         const std::string &game_name,
                         std::unique_ptr<std::deque<int> > update_counts,
                         std::unique_ptr<std::deque<int> > time_deltas,
                         std::unique_ptr<std::deque<unsigned int> > random_seeds)
    : pimpl(new KnightsGameImpl)
{
    pimpl->knights_config = config;
    pimpl->timer = tmr;
    pimpl->allow_split_screen = allow_split_screen;
    pimpl->game_over = false;
    pimpl->pause_mode = false;
    pimpl->update_thread_wants_to_exit = false;
    pimpl->emergency_exit = false;

    // set up our own controls vector.
    pimpl->controls.clear();
    config->getStandardControls(pimpl->controls);
    std::vector<const UserControl*> other_ctrls;
    config->getOtherControls(other_ctrls);
    pimpl->controls.insert(pimpl->controls.end(), other_ctrls.begin(), other_ctrls.end());
    
    pimpl->knights_log = knights_log;
    pimpl->game_name = game_name;

    pimpl->msg_counter = 0;
    if (update_counts.get()) pimpl->update_counts = std::move(update_counts);
    if (time_deltas.get()) pimpl->time_deltas = std::move(time_deltas);
    if (random_seeds.get()) pimpl->random_seeds = std::move(random_seeds);
    pimpl->msg_count_update_flag = true;

    pimpl->wake_up_flag = false;
}

KnightsGame::~KnightsGame()
{
    // If the update thread is still running then we have to kill it
    // here. The thread dtor will not kill the thread for us, instead
    // the thread will just become "detached" and continue running,
    // with disastrous consequences if the KnightsGame object is being
    // destroyed!
    pimpl->update_thread.interrupt();
    pimpl->update_thread.join();
}

int KnightsGame::getNumPlayers() const
{
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    return CountPlayers(*pimpl);
}

int KnightsGame::getNumObservers() const
{
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    int n = 0;
    for (game_conn_vector::const_iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
        if ((*it)->obs_flag) ++n;
    }
    return n;
}

bool KnightsGame::isSplitScreenAllowed() const
{
    // sub thread won't be changing allow_split_screen so don't need to lock
    return pimpl->allow_split_screen;
}

GameStatus KnightsGame::getStatus() const
{
    if (pimpl->update_thread.joinable()) return GS_RUNNING;
    else if (getNumPlayers() < 2) return GS_WAITING_FOR_PLAYERS;
    else return GS_SELECTING_QUEST;
}

bool KnightsGame::getObsFlag(GameConnection &conn) const
{
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    return conn.obs_flag;
}

GameConnection & KnightsGame::newClientConnection(const UTF8String &client_name, const UTF8String &client_name_2, 
                                                  int client_version, bool approach_based_controls, bool action_bar_controls)
{
    WaitForMsgCounter(*pimpl);
    
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    if (pimpl->msg_count_update_flag) pimpl->msg_counter++;
    
    // check preconditions. (caller should have checked these already.)
    if (client_name.empty()) throw UnexpectedError("invalid client name");
    for (game_conn_vector::const_iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
        if ((*it)->name == client_name) throw UnexpectedError("Duplicate client name");
        if (!client_name_2.empty() && (*it)->name == client_name_2) throw UnexpectedError("Duplicate client name");
    }
    if (!pimpl->allow_split_screen && !client_name_2.empty()) throw UnexpectedError("Split screen mode not allowed");
    if (!client_name_2.empty() && !pimpl->connections.empty()) throw UnexpectedError("Cannot join in split screen mode while connections exist");

    // If the game is running, they join as an observer, otherwise, they join as a player
    const bool new_obs_flag = pimpl->update_thread.joinable();

    // create the GameConnection
    boost::shared_ptr<GameConnection> conn(new GameConnection(client_name, client_name_2, 
                                                              new_obs_flag, client_version,
                                                              approach_based_controls, action_bar_controls));
    pimpl->connections.push_back(conn);

    // sort connections by name
    // (this makes the player list appear in alphabetical order.)
    std::sort(pimpl->connections.begin(), pimpl->connections.end(), CmpByName());

    // modify the house colour if necessary.
    if (!new_obs_flag) {
        bool col_ok = false;
        const int n_hse_cols = GetNumAvailHouseCols(*pimpl);
        while (!col_ok) {
            col_ok = true;
            for (game_conn_vector::const_iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
                if (*it != conn && (*it)->house_colour == conn->house_colour && !(*it)->obs_flag) {
                    col_ok = false;
                    // try the next colour
                    conn->house_colour ++;
                    std::vector<Coercri::Color> hse_cols;
                    pimpl->knights_config->getHouseColours(hse_cols);  // a bit wasteful as we only want the size. never mind.
                    if (conn->house_colour == n_hse_cols - 1) {
                        // run out of colours so will just have to accept the last one
                        col_ok = true; 
                    }
                    break;
                }
            }
        }
    }

    // Send the SERVER_JOIN_GAME_ACCEPTED message (includes initial configuration messages e.g. menu settings)
    Coercri::OutputByteBuf buf(conn->output_data);
    SendJoinGameAccepted(*pimpl, buf, conn->house_colour);

    // Send any necessary SERVER_PLAYER_JOINED_THIS_GAME messages (but not to the player who just joined).
    for (game_conn_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
        if (*it != conn) {
            Coercri::OutputByteBuf out((*it)->output_data);
            out.writeUbyte(SERVER_PLAYER_JOINED_THIS_GAME);
            out.writeString(client_name.asUTF8());
            out.writeUbyte(new_obs_flag);
            out.writeUbyte(conn->house_colour);
            if (!client_name_2.empty()) {
                out.writeUbyte(SERVER_PLAYER_JOINED_THIS_GAME);
                out.writeString(client_name_2.asUTF8());
                out.writeUbyte(new_obs_flag);
                out.writeUbyte(conn->house_colour);
            }
        }
    }

    // If the new player is an observer and the game is running then
    // send him the START_GAME_OBS msg immediately
    if (new_obs_flag && pimpl->update_thread.joinable()) {
        // send the msg
        buf.writeUbyte(SERVER_START_GAME_OBS);
        buf.writeUbyte(pimpl->all_player_names.size());
        buf.writeUbyte(pimpl->deathmatch_mode);
        for (std::vector<UTF8String>::const_iterator it = pimpl->all_player_names.begin(); it != pimpl->all_player_names.end(); ++it) {
            buf.writeString(it->asUTF8());
        }
        buf.writeUbyte(1);  // already_started flag (true)
    }

    if (!new_obs_flag) {
        // May need to update menu settings, since no of players has changed
        UpdateNumPlayersAndTeams(*pimpl);
    }
    
    return *conn;
}

void KnightsGame::clientLeftGame(GameConnection &conn)
{
    // This is called when a player has left the game (either because
    // KnightsServer sent him a SERVER_LEAVE_GAME msg, or because he
    // disconnected.)

    bool is_player = false;
    
    UTF8String name, name2;
    int player_num = 0;
    int num_player_connections = 0;

    WaitForMsgCounter(*pimpl);
    
    {
        boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
        if (pimpl->msg_count_update_flag) pimpl->msg_counter++;
        
        // Find the client in our connections list. 
        game_conn_vector::iterator where = std::find_if(pimpl->connections.begin(), 
                                                        pimpl->connections.end(), ShPtrEq<GameConnection>(&conn));
        ASSERT(where != pimpl->connections.end());
        
        // Determine whether he is a player or observer. Also save his name(s)
        is_player = !(*where)->obs_flag;
        player_num = (*where)->player_num;
        name = (*where)->name;
        name2 = (*where)->name2;
        if ((*where)->observer_num > 0) {
            pimpl->delete_observer_nums.push_back( (*where)->observer_num );
        }
        
        // Erase the connection from our list. Note 'conn' is invalid from now on
        pimpl->connections.erase(where);

        // find out how many player connections are remaining
        for (game_conn_vector::const_iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
            if (!(*it)->obs_flag) {
                ++num_player_connections;
            }
        }
    }
    
    if (is_player && pimpl->update_thread.joinable()) {
        // The leaving client is one of the players (as opposed to an observer).
        if (num_player_connections <= 1) {
            // Only one player connection is left. We should just terminate the game at this point.
            StopGameAndReturnToMenu(*pimpl);
        } else {
            // There are still players left in so eliminate that player and allow the game to continue.
            boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
            pimpl->players_to_eliminate.push_back(player_num);
        }
    }
    
    {
        boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
        
        // Send SERVER_PLAYER_LEFT_THIS_GAME message to all remaining players
        for (game_conn_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
            Coercri::OutputByteBuf buf((*it)->output_data);
            buf.writeUbyte(SERVER_PLAYER_LEFT_THIS_GAME);
            buf.writeString(name.asUTF8());
            buf.writeUbyte(is_player ? 0 : 1);
            if (!name2.empty()) {
                buf.writeUbyte(SERVER_PLAYER_LEFT_THIS_GAME);
                buf.writeString(name2.asUTF8());
                buf.writeUbyte(is_player ? 0 : 1);
            }
        }

        if (pimpl->connections.empty()) {
            // If there are no connections left then reset the menu selections.
            pimpl->knights_config->resetMenu();
        } else if (is_player) {
            // Number of players has changed, may need to update menu constraints.
            UpdateNumPlayersAndTeams(*pimpl);
        }
    }
    
    // If all remaining players are ready, then the game should start (Trac #25)
    StartGameIfReady(*pimpl);    
}

void KnightsGame::sendChatMessage(GameConnection &conn, const std::string &msg_orig)
{
    // Determine whether this is a Team chat message
    bool is_team;
    std::string msg;
    if (!pimpl->update_thread.joinable() || conn.obs_flag) {
        // Team chat not available (either because we are on the quest selection menu, 
        // or because sender is an observer).
        is_team = false;
        msg = msg_orig;
    } else {
        // Check whether they typed /t, and strip it out (and set is_team flag) if it's there
        CheckTeamChat(msg_orig, msg, is_team);
    }

    // Forward the message to everybody (including the originator)
    // (Except team chat msgs which go to team mates only)
    WaitForMsgCounter(*pimpl);
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    if (pimpl->msg_count_update_flag) pimpl->msg_counter++;
    for (game_conn_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {

        if (is_team) {
            // Skip over players of different team, or observers
            if ((*it)->obs_flag || (*it)->house_colour != conn.house_colour) continue;
        }

        Coercri::OutputByteBuf buf((*it)->output_data);
        buf.writeUbyte(SERVER_CHAT);
        buf.writeString(conn.name.asUTF8());

        if (is_team) {
            buf.writeUbyte(3);  // Team Message
        } else if (conn.obs_flag) {
            buf.writeUbyte(2);  // Message from an observer
        } else {
            buf.writeUbyte(1);  // Normal Message
        }
        buf.writeString(msg);
    }
}

void KnightsGame::setReady(GameConnection &conn, bool ready)
{
    if (pimpl->update_thread.joinable()) return;  // Game is running.

    // (We know the game is not running at this point, so no need to lock mutex)
    
    WaitForMsgCounter(*pimpl);
    
    if (pimpl->msg_count_update_flag) pimpl->msg_counter++;
    if (!conn.obs_flag) {
        // Set conn.is_ready and send out notifications
        DoSetReady(*pimpl, conn, ready);

        // Start the game if all players are now ready.
        StartGameIfReady(*pimpl);
    }
}

void KnightsGame::setHouseColour(GameConnection &conn, int hse_col)
{
    if (pimpl->update_thread.joinable()) return;  // Game is running

    WaitForMsgCounter(*pimpl);
    
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    if (pimpl->msg_count_update_flag) pimpl->msg_counter++;
    if (!conn.obs_flag) {
        conn.house_colour = hse_col;

        // send notification to players
        for (game_conn_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
            Coercri::OutputByteBuf out((*it)->output_data);
            out.writeUbyte(SERVER_SET_HOUSE_COLOUR);
            // note we don't support house colours in split screen mode at the moment. we assume it's the first player
            // on the connection who is having the house colours set.
            out.writeString(conn.name.asUTF8());
            out.writeUbyte(hse_col);
        }

        UpdateNumPlayersAndTeams(*pimpl);
    }
}
    
void KnightsGame::finishedLoading(GameConnection &conn)
{
    WaitForMsgCounter(*pimpl);
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    if (pimpl->msg_count_update_flag) pimpl->msg_counter++;
    conn.finished_loading = true;
}

void KnightsGame::readyToEnd(GameConnection &conn)
{
    if (!pimpl->game_over) return;

    WaitForMsgCounter(*pimpl);
    
    bool all_ready = false;
    {
        boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
        if (pimpl->msg_count_update_flag) pimpl->msg_counter++;
        if (!conn.obs_flag) {
            conn.ready_to_end = true;
            all_ready = true;
            for (game_conn_vector::const_iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
                if (!(*it)->obs_flag && !(*it)->ready_to_end) {
                    all_ready = false;
                    break;
                }
            }
        }
    }
    
    if (all_ready) {
        StopGameAndReturnToMenu(*pimpl);
    } else {
        // tell everyone that this player is now ready to go back to the menu.
        for (game_conn_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
            Coercri::OutputByteBuf out((*it)->output_data);
            out.writeUbyte(SERVER_READY_TO_END);
            out.writeString(conn.name.asUTF8());
        }
    }
}

bool KnightsGame::requestQuit(GameConnection &conn)
{
    if (!pimpl->update_thread.joinable()) return false;  // Game is not running

    WaitForMsgCounter(*pimpl);
    
    bool obs_flag;
    {
        boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
        if (pimpl->msg_count_update_flag) pimpl->msg_counter++;
        obs_flag = conn.obs_flag;
    }
    if (!obs_flag) {

        if (pimpl->knights_log) {
            pimpl->knights_log->logMessage(pimpl->game_name + "\tquit requested\t" + conn.name.asUTF8());
        }

        // If there are only two players then stop the game (with appropriate message), 
        // otherwise, kick this player out (but the game continues).
        if (getNumPlayers() > 2) {
            boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
            pimpl->players_to_eliminate.push_back(conn.player_num);
            return true;
        } else {
            {
                boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
                Announcement(*pimpl, conn.name.asLatin1() + " quit the game.");
            }
            StopGameAndReturnToMenu(*pimpl);
            return false;
        }
    }

    return false;
}

void KnightsGame::setPauseMode(bool pm)
{
    WaitForMsgCounter(*pimpl);
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    if (pimpl->msg_count_update_flag) pimpl->msg_counter++;
    pimpl->pause_mode = pm;
}

void KnightsGame::setMenuSelection(GameConnection &conn, int item_num, int choice_num)
{
    if (pimpl->update_thread.joinable()) return;  // Game is running

    WaitForMsgCounter(*pimpl);

    // validate input
    const int num_items = pimpl->knights_config->getMenu().getNumItems();
    if (item_num < 0 || item_num >= num_items) return;
    const MenuItem & item = pimpl->knights_config->getMenu().getItem(item_num);
    if (!item.isNumeric() && (choice_num < 0 || choice_num >= item.getNumChoices())) return;

    // update msgcounter, also check we are a player
    // note: we know update thread is not running (see above) so no need to lock
    if (pimpl->msg_count_update_flag) pimpl->msg_counter++;
    if (conn.obs_flag) return;   // only players can adjust the menu.

    // send out the settings change(s) to all players
    setMenuSelectionWork(&conn, item_num, choice_num);
}

void KnightsGame::setMenuSelectionWork(GameConnection *conn, int item_num, int choice_num)
{
    if (pimpl->update_thread.joinable()) return;  // game is running

    // since we now know the update thread is not running, there is no need to lock.

    MyMenuListener listener;
    for (game_conn_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
        listener.addBuf((*it)->output_data);
    }
    pimpl->knights_config->changeMenuSetting(item_num, choice_num, listener);
    
    if (conn && listener.wereThereChanges()) {
        // send announcement to all players
        const MenuItem & item = pimpl->knights_config->getMenu().getItem(item_num);
        std::ostringstream str;
        str << conn->name.asLatin1() << " set \"" << item.getTitleString() << "\" to ";
        
        if (item.isNumeric()) {
            str << choice_num;
        } else {
            str << '"' << item.getChoiceString(choice_num) << '"';
        }
        str << '.';
        Announcement(*pimpl, str.str());

        // deactivate all ready flags
        DeactivateReadyFlags(*pimpl);
    }
}

void KnightsGame::randomQuest(GameConnection &conn)
{
    if (pimpl->update_thread.joinable()) return;  // Game is running
    
    WaitForMsgCounter(*pimpl);

    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    if (pimpl->msg_count_update_flag) pimpl->msg_counter++;
    if (conn.obs_flag) return;   // only players can set quests.

    MyMenuListener listener;
    for (game_conn_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
        listener.addBuf((*it)->output_data);
    }
    pimpl->knights_config->randomQuest(listener);

    // send announcement to all players
    Announcement(*pimpl, conn.name.asLatin1() + " selected a Random Quest.");    

    // deactivate all ready flags
    DeactivateReadyFlags(*pimpl);

    // Also have to save the quest in the binary log
    // (This is so that replays work correctly. It is no good just having 
    // "random quest" in the log, we need to save what the quest was actually set to.)
    if (pimpl->knights_log) {
        std::ostringstream random_quest_str;
        random_quest_str << pimpl->game_name << '\0';
        RandomQuestMenuListener random_quest_listener(random_quest_str);
        pimpl->knights_config->getCurrentMenuSettings(random_quest_listener);
        const std::string &s = random_quest_str.str();
        pimpl->knights_log->logBinary("QST", 0, s.length(), s.c_str());
    }
}

void KnightsGame::requestGraphics(Coercri::OutputByteBuf &buf, const std::vector<int> &ids)
{
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    std::vector<const Graphic *> graphics;
    pimpl->knights_config->getGraphics(graphics);
    
    buf.writeUbyte(SERVER_SEND_GRAPHICS);
    buf.writeVarInt(ids.size());
    
    for (std::vector<int>::const_iterator it = ids.begin(); it != ids.end(); ++it) {
        if (*it <= 0 || static_cast<size_t>(*it) > graphics.size()) {
            throw ProtocolError("graphic id out of range");
        }
        buf.writeVarInt(*it);
        ASSERT(graphics[*it - 1]->getID() == *it);
        SendFile(buf, graphics[*it - 1]->getFileInfo());
    }
}

void KnightsGame::requestSounds(Coercri::OutputByteBuf &buf, const std::vector<int> &ids)
{
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    std::vector<const Sound *> sounds;
    pimpl->knights_config->getSounds(sounds);
    
    buf.writeUbyte(SERVER_SEND_SOUNDS);
    buf.writeVarInt(ids.size());
    
    for (std::vector<int>::const_iterator it = ids.begin(); it != ids.end(); ++it) {
        if (*it <= 0 || static_cast<size_t>(*it) > sounds.size()) {
            throw ProtocolError("sound id out of range");
        }
        buf.writeVarInt(*it);
        ASSERT(sounds[*it - 1]->getID() == *it);
        SendFile(buf, sounds[*it - 1]->getFileInfo());
    }
}

void KnightsGame::sendControl(GameConnection &conn, int p, unsigned char control_num)
{
    if (!pimpl->update_thread.joinable()) return; // Game is not running

    WaitForMsgCounter(*pimpl);

    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    if (pimpl->msg_count_update_flag) pimpl->msg_counter++;
    if (!conn.obs_flag && p >= 0 && p < (conn.name2.empty() ? 1 : 2)) {
        const UserControl * control = control_num == 0 ? 0 : pimpl->controls.at(control_num - 1);
        conn.control_queue[p].push_back(control);
    }
}

void KnightsGame::requestSpeechBubble(GameConnection &conn, bool show)
{
    if (!pimpl->update_thread.joinable()) return;  // Game is not running

    WaitForMsgCounter(*pimpl);

    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    if (pimpl->msg_count_update_flag) pimpl->msg_counter++;
    if (!conn.obs_flag) {
        conn.speech_request = true;
        conn.speech_bubble = show;
    }
}

void KnightsGame::endOfMessagePacket()
{
    // This should always be called after a batch of sendControl()s or requestSpeechBubble()s are done.

    // It checks whether wake_up_flag is set, and if so, notifies the UpdateThread.
    // That thread will respond by doing a game update (thus responding to the players' controls and/or
    // speech bubble requests.)

    // This is better than the previous model which was to poll every 50ms or so, because that just added
    // (upto) 50ms of lag on the controls.

    // (Note: There is still at least one polling delay, and that is in KnightTask which
    // only runs every 'control_poll_interval', currently 50ms.)

    bool flag;
    {
        boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
        flag = (pimpl->wake_up_flag);
    }
    if (flag) pimpl->wake_up_cond_var.notify_one();
}

void KnightsGame::setObsFlag(GameConnection &conn, bool new_obs_flag)
{
    WaitForMsgCounter(*pimpl);
    
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    if (pimpl->msg_count_update_flag) pimpl->msg_counter++;
    
    if (conn.obs_flag == new_obs_flag) return;  // Nothing to do
    
    // If the game is in progress then don't allow players to change obs-flag setting
    if (pimpl->update_thread.joinable()) {
        Coercri::OutputByteBuf buf(conn.output_data);
        buf.writeUbyte(SERVER_ANNOUNCEMENT);
        buf.writeString("Cannot change observer status at this time. Please wait until the game has finished.");
        return;
    }

    // If he is becoming an observer, and he is currently 'ready to start', then make him non-ready first.
    if (new_obs_flag && conn.is_ready) {
        DoSetReady(*pimpl, conn, false);
    }
    
    // changing obs_flag does not apply to split screen games, so we can ignore name2 and just use name.
    const UTF8String & my_name = conn.name;

    // If he is a player then assign him a house colour.
    int new_col = 0;
    if (!new_obs_flag) {
        std::vector<Coercri::Color> hse_cols;
        pimpl->knights_config->getHouseColours(hse_cols);

        while (new_col < int(hse_cols.size()) - 1) {
            bool col_taken = false;
            for (game_conn_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
                if (!(*it)->obs_flag && (*it)->house_colour == new_col) {
                    col_taken = true;
                    break;
                }
            }
            if (col_taken) ++new_col;
            else break;
        }
    }

    // Update our data, then send announcements to all players.
    conn.obs_flag = new_obs_flag;
    conn.house_colour = new_col;
    for (game_conn_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
        Coercri::OutputByteBuf buf((*it)->output_data);

        buf.writeUbyte(SERVER_SET_OBS_FLAG);
        buf.writeString(my_name.asUTF8());
        buf.writeUbyte(new_obs_flag ? 1 : 0);
        
        if (!new_obs_flag) {
            buf.writeUbyte(SERVER_SET_HOUSE_COLOUR);
            buf.writeString(my_name.asUTF8());
            buf.writeUbyte(new_col);
        }
    }

    // May need to re-apply max players constraints
    UpdateNumPlayersAndTeams(*pimpl);
}

void KnightsGame::getOutputData(GameConnection &conn, std::vector<unsigned char> &data)
{
    bool do_wait;

    {
        boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
        std::swap(conn.output_data, data);
        conn.output_data.clear();
        do_wait = pimpl->update_thread_wants_to_exit;
    }

    // Since getOutputData is called regularly, this is a good place
    // to join() with the update thread, if the update thread has
    // finished.
    if (do_wait) {
        if (pimpl->update_thread.joinable()) pimpl->update_thread.join();
        pimpl->update_thread_wants_to_exit = false;
        if (pimpl->emergency_exit) {
            // Throw an exception and let the caller deal with it.
            // Currently (16-Jul-2012) this results in the entire server being shut down.
            // It would be better if only this one game could be closed, but I can't work 
            // out a clean way of doing that at the moment.
            throw LuaPanic("Fatal Lua error in update thread: " + pimpl->emergency_err_msg);
        }
    }
}

void KnightsGame::setPingTime(GameConnection &conn, int ping)
{
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    conn.ping_time = ping;
}

void KnightsGame::setMsgCountUpdateFlag(bool on)
{
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    pimpl->msg_count_update_flag = on;
}

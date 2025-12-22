/*
 * knights_game.cpp
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
#include "read_write_loc.hpp"
#include "rng.hpp"
#include "rstream.hpp"
#include "server_callbacks.hpp"
#include "sh_ptr_eq.hpp"
#include "sound.hpp"
#include "user_control.hpp"

#include "boost/scoped_ptr.hpp"
#include "boost/weak_ptr.hpp"

#ifdef VIRTUAL_SERVER
#include "syscalls.hpp"
#else
#include "boost/thread/condition_variable.hpp"
#include "boost/thread/locks.hpp"
#include "boost/thread/thread.hpp"
#include <random>
#endif

#include <algorithm>
#include <ctime>
#include <map>
#include <memory>
#include <set>
#include <sstream>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

using std::ios_base;
using std::string;
using std::vector;

#ifdef VIRTUAL_SERVER
namespace {
    class UpdateThread;
}
#endif

class GameConnection {
public:
    GameConnection(const PlayerID &id1, const PlayerID &id2, bool new_obs_flag, int ver,
                   bool approach_based_ctrls, bool action_bar_ctrls)
        : id1(id1), id2(id2),
          is_ready(false), finished_loading(false), ready_to_end(false), 
          obs_flag(new_obs_flag), cancel_obs_mode_after_game(false),
          requires_catchup(false),
          house_colour(0),
          client_version(ver),
          observer_num(0), player_num(-1),
          ping_time(0),
          speech_request(false), speech_bubble(false),
          approach_based_controls(approach_based_ctrls),
          action_bar_controls(action_bar_ctrls)
    { }

    // These will be Steam IDs (or other platform IDs) in online-platform games,
    // or UTF8 player names otherwise.
    // Also, !id2.empty() means it is a split-screen connection (and hence a local 2-player game).
    PlayerID id1, id2;
    
    bool is_ready;    // true=ready to start game, false=want to stay in lobby
    bool finished_loading;   // true=ready to play, false=still loading
    bool ready_to_end;   // true=clicked mouse on winner/loser screen, false=still waiting.
    bool obs_flag;   // true=observer, false=player
    bool cancel_obs_mode_after_game;
    bool requires_catchup;
    int house_colour;  // Must fit in a ubyte. Set to zero for observers (but beware, zero is also a valid house colour for non-observers!).

    int client_version;
    int observer_num;   // 0 if not an observer, or not set yet. >0 if set.
    int player_num;     // 0..num_players-1, or -1 if the game is not running.
                        //  Also, observers have -1, unless they are eliminated players,
                        //  in which case they retain their original player_num.
    int ping_time;
    
    std::vector<unsigned char> output_data;    

    std::vector<const UserControl*> control_queue[2];

    bool speech_request, speech_bubble;
    bool approach_based_controls;
    bool action_bar_controls;
};

typedef std::vector<boost::shared_ptr<GameConnection> > game_conn_vector;

const int HOUSE_COLOUR_CIRCULAR_BUFFER_SIZE = 30;

class KnightsGameImpl {
public:
    boost::shared_ptr<KnightsConfig> knights_config;
    boost::shared_ptr<Coercri::Timer> timer;
    bool allow_split_screen;  // Whether to allow split screen and single player games.
    bool deathmatch_mode;

    std::vector<const UserControl*> controls;

    std::string quest_description;
    game_conn_vector connections;
    game_conn_vector incoming_connections;

    bool game_over;
    bool pause_mode;

#ifdef VIRTUAL_SERVER
    std::unique_ptr<UpdateThread> update_thread;
#else
    // methods of KnightsGame should (usually) lock this mutex.
    // the update thread will also lock this mutex when it is making changes to the KnightsGameImpl or GameConnection structures.
    boost::mutex my_mutex;

    boost::thread update_thread;
#endif

    volatile bool update_thread_wants_to_exit;
    volatile bool emergency_exit;  // lua_State is unusable; close the game asap.
    std::string emergency_err_msg;

    KnightsLog *knights_log;
    std::string game_name;

    std::vector<int> delete_observer_nums;
    std::vector<int> pending_disconnections;
    std::vector<PlayerID> all_player_ids;

    // used during game startup
    volatile bool startup_signal;
    LocalMsg startup_err;

    // Condition variable used to wake up the update thread when a control comes in
    // (instead of polling, like it used to do).
    // Note, this is controlled using the same mutex (my_mutex) as the rest of the data.
#ifndef VIRTUAL_SERVER
    boost::condition_variable wake_up_cond_var;
#endif
    bool wake_up_flag;

    // Track previous house colours for reconnecting players (circular buffer)
    std::vector<std::pair<PlayerID, int>> previous_house_colours{HOUSE_COLOUR_CIRCULAR_BUFFER_SIZE};
    int previous_house_colours_next = 0;  // Next index to write to
};

namespace {

    // Search for a PlayerID in the previous_house_colours circular buffer.
    // Returns index if found, -1 otherwise.
    int FindPreviousHouseColour(const KnightsGameImpl &impl, const PlayerID &id)
    {
        for (int i = 0; i < HOUSE_COLOUR_CIRCULAR_BUFFER_SIZE; ++i) {
            if (impl.previous_house_colours[i].first == id) {
                return i;
            }
        }
        return -1;
    }

    void CheckTeamChat(const Coercri::UTF8String &msg_orig_utf8,
                       Coercri::UTF8String &msg,
                       bool &is_team)
    {
        std::string msg_orig = msg_orig_utf8.asUTF8();

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
        msg = Coercri::UTF8String::fromUTF8(msg_orig.substr(idx));
    }

    void Announcement(KnightsGameImpl &kg, const LocalMsg &msg, bool is_err = false)
    {
        for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
            Coercri::OutputByteBuf buf((*it)->output_data);

            if (is_err) {
                buf.writeUbyte(SERVER_EXTENDED_MESSAGE);
                buf.writeVarInt(SERVER_EXT_NEXT_ANNOUNCEMENT_IS_ERROR);
                buf.writeUshort(0);
            }

            buf.writeUbyte(SERVER_ANNOUNCEMENT_LOC);
            WriteLocalMsg(buf, msg);
        }
    }

    void SendMessages(game_conn_vector &conns, const std::vector<LocalMsg> &messages)
    {
        for (game_conn_vector::const_iterator it = conns.begin(); it != conns.end(); ++it) {
            Coercri::OutputByteBuf buf((*it)->output_data);
            for (const auto &msg : messages) {
                buf.writeUbyte(SERVER_ANNOUNCEMENT_LOC);
                WriteLocalMsg(buf, msg);
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
                              int my_house_colour, bool already_started)
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
                if (!(*it)->id2.empty()) {
                    if (impl.connections.size() > 1) throw ProtocolError(LocalKey("split_screen_not_allowed"));
                    ++np;
                }
            }
        }

        buf.writeVarInt(np);

        for (game_conn_vector::const_iterator it = impl.connections.begin(); it != impl.connections.end(); ++it) {
            if (!(*it)->obs_flag) {
                buf.writeString((*it)->id1.asString());
                buf.writeUbyte((*it)->is_ready ? 1 : 0);
                buf.writeUbyte((*it)->house_colour);
                if (!(*it)->id2.empty()) {
                    buf.writeString((*it)->id2.asString());
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
                ASSERT((*it)->id2.empty());
                buf.writeString((*it)->id1.asString());
            }
        }

        // Final byte of SERVER_JOIN_GAME_ACCEPTED tells whether the game has already
        // been started
        buf.writeUbyte(already_started ? 1 : 0);

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
                if (!(*it)->id2.empty()) ++nplayers;
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
                if (!(*it)->id2.empty()) {
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

    void AddNewPlayer(KnightsGameImpl &kg,
                      boost::shared_ptr<GameConnection> conn,
                      KnightsEngine *engine,
                      ServerCallbacks *callbacks)
    {
        // Determine if the player is an observer, and find their house colour if required.
        bool observer = false;
        bool enter_game = false;
        if (engine) {
            observer = true;
            enter_game = true;

            std::vector<PlayerInfo> players;
            engine->getPlayerList(players);

            for (auto const& info : players) {
                // Compare by ID
                if (info.id == conn->id1) {
                    // Found the player in the game
                    if (info.player_state == PlayerState::DISCONNECTED || info.player_state == PlayerState::NORMAL) {
                        // This player is actually playing, not just observing
                        observer = false;
                        conn->house_colour = info.house_colour_index;
                        conn->player_num = info.player_num;
                    }
                    break;
                }
            }
        }

        conn->obs_flag = observer;

        // Add to connections list
        kg.connections.push_back(conn);

        // Send the SERVER_JOIN_GAME_ACCEPTED message (includes initial configuration messages e.g. menu settings)
        Coercri::OutputByteBuf buf(conn->output_data);
        SendJoinGameAccepted(kg, buf, conn->house_colour, enter_game);

        // Send any necessary SERVER_PLAYER_JOINED_THIS_GAME messages (but not to the player who just joined).
        for (game_conn_vector::iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
            if (*it != conn) {
                Coercri::OutputByteBuf out((*it)->output_data);
                out.writeUbyte(SERVER_PLAYER_JOINED_THIS_GAME);
                out.writeString(conn->id1.asString());
                out.writeUbyte(observer);
                out.writeUbyte(conn->house_colour);
                if (!conn->id2.empty()) {
                    out.writeUbyte(SERVER_PLAYER_JOINED_THIS_GAME);
                    out.writeString(conn->id2.asString());
                    out.writeUbyte(observer);
                    out.writeUbyte(conn->house_colour);
                }
            }
        }

        // If the game is running then send START_GAME or START_GAME_OBS immediately
        if (enter_game) {
            buf.writeUbyte(observer ? SERVER_START_GAME_OBS : SERVER_START_GAME);
            buf.writeUbyte(observer ? kg.all_player_ids.size() : 1);
            buf.writeUbyte(kg.deathmatch_mode);
            if (observer) {
                for (std::vector<PlayerID>::const_iterator it = kg.all_player_ids.begin(); it != kg.all_player_ids.end(); ++it) {
                    buf.writeString(it->asString());
                }
            }
            buf.writeUbyte(1);  // already_started flag (true)

            // If the game is over, and this is a player (not
            // observer), then send SERVER_WIN_GAME or
            // SERVER_LOSE_GAME to this player (to put them on the
            // winner/loser screen)
            if (kg.game_over && conn->player_num >= 0) {
                if (callbacks->isLoser(conn->player_num)) {
                    buf.writeUbyte(SERVER_LOSE_GAME);
                } else {
                    buf.writeUbyte(SERVER_WIN_GAME);
                }
            }
        }

        // May need to update menu settings, since no of players has changed
        UpdateNumPlayersAndTeams(kg);
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

                // Setup house colours and player IDs
                // Also count how many players on each team.
                std::vector<int> hse_cols;
                std::vector<PlayerID> player_ids;
                std::map<int, int> team_counts;
                for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                    if (!(*it)->obs_flag) {
                        int col = (*it)->house_colour;
                        hse_cols.push_back(col);
                        ++team_counts[col];
                        player_ids.push_back((*it)->id1);
                        
                        if (!(*it)->id2.empty()) {
                            ++col;  // split screen mode: house colours are consecutive (0 and 1)
                            hse_cols.push_back(col);
                            ++team_counts[col];
                            player_ids.push_back((*it)->id2);
                        }
                    }
                }

                kg.all_player_ids = player_ids;
                
                if (player_ids.size() == 2 && kg.connections.size() == 1) {
                    // Split screen game.
                    // Disable the player names in this case.
                    for (size_t i = 0; i < player_ids.size(); ++i) { player_ids[i] = PlayerID(); }
                }

                nplayers = player_ids.size();

                // Create the KnightsEngine. Pass any messages back to the players
                std::vector<LocalMsg> messages;
                try {
                    engine.reset(new KnightsEngine(kg.knights_config, hse_cols, player_ids,
                                                   kg.deathmatch_mode,  // output from KnightsEngine
                                                   messages));   // output from KnightsEngine

                } catch (LuaPanic &) {
                    // This is serious enough that we re-throw and let 
                    // the game close itself down.
                    throw;

                } catch (const ExceptionBase &e) {

                    SendMessages(kg.connections, messages);

                    kg.startup_err = e.getMsg();

                    kg.update_thread_wants_to_exit = true;
#ifdef VIRTUAL_SERVER
                    vs_exit_game_thread();
#endif
                    return;

                } catch (const std::exception &e) {

                    SendMessages(kg.connections, messages);

                    kg.startup_err = {LocalKey("cxx_error_is"), {LocalParam(UTF8String::fromUTF8Safe(e.what()))}};

                    kg.update_thread_wants_to_exit = true;
#ifdef VIRTUAL_SERVER
                    vs_exit_game_thread();
#endif
                    return;
                }

                for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                    if (!(*it)->obs_flag) {
                        for (int p = 0; p < ((*it)->id2.empty() ? 1 : 2); ++p) {
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
#ifndef VIRTUAL_SERVER
                        boost::lock_guard<boost::mutex> lock(kg.my_mutex);
#endif
                        bool all_loaded = true;
                        for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                            if (!(*it)->obs_flag && !(*it)->finished_loading) {
                                all_loaded = false;
                                break;
                            }
                        }
                        if (all_loaded) break;
                    }
#ifdef VIRTUAL_SERVER
                    vs_switch_to_main_thread(100);
#else
                    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
#endif
                }

                {
#ifndef VIRTUAL_SERVER
                    boost::lock_guard<boost::mutex> lock(kg.my_mutex);
#endif

                    // Send through any initialization msgs from lua.
                    SendMessages(kg.connections, messages);

                    // Send the team chat notification (but only if more than 1 player on the team; #151)
                    for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                        Coercri::OutputByteBuf buf((*it)->output_data);

                        if (!(*it)->obs_flag && team_counts[(*it)->house_colour] > 1) {
                            buf.writeUbyte(SERVER_ANNOUNCEMENT_LOC);
                            WriteLocalMsg(buf, LocalMsg{LocalKey("team_chat_avail")});
                        }
                    }
                }

                // Go into game loop. NOTE: This will run forever until the main thread interrupts us
                // (or an exception occurs, or update() returns false).
                mainGameLoop();

            } catch (boost::thread_interrupted &) {
                // Allow this to go through. The code that interrupted us knows what it's doing...

            } catch (LuaPanic &e) {
                // If this happens we need to exit game asap
                kg.emergency_exit = true;
                kg.emergency_err_msg = e.what();
                std::string msg = e.what();
                sendError(LocalMsg{LocalKey("lua_error_is"), {LocalParam(UTF8String::fromUTF8Safe(msg))}});

            } catch (...) {
                sendError(LocalMsg{LocalKey("unknown_error")});
            }

            // Before we go, delete the KnightsEngine. This will make sure that the Mediator
            // is still around while the KnightsEngine destructor runs.
            engine.reset();  // Never throws

            // Tell the main thread that the update thread wants to exit.
            // Note no need to lock mutex because there is only one writer (us) and one reader (the main thread)
            kg.update_thread_wants_to_exit = true;

#ifdef VIRTUAL_SERVER
            // Exit the thread
            vs_exit_game_thread();
#endif
        }

        void sendError(const LocalMsg &msg)
        {
            try {
                // send error to all players connected to this game.
#ifndef VIRTUAL_SERVER
                boost::lock_guard<boost::mutex> lock(kg.my_mutex);
#endif
                for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                    Coercri::OutputByteBuf buf((*it)->output_data);
                    buf.writeUbyte(SERVER_ERROR);
                    WriteLocalMsg(buf, msg);
                }
                // log the error as well.
                if (kg.knights_log) {
                    kg.knights_log->logMessage(kg.game_name + "\tError in update thread\t" + msg.key.getKey());
                }
            } catch (...) {
                // Disregard any exceptions here, as we don't want them to propagate up into the
                // boost::thread library internals (this would terminate the entire server if it happened).
            }
        }

        void mainGameLoop()
        {
            // Get current wall clock time.
            unsigned int wall_clock_time = timer->getMsec();

            // Track "simulated time" inside the dungeon.

            // Initially, we imagine that the dungeon clocks are
            // synchronised with the actual wall clock time
            // (timer->getMsec()).

            // Our goal will be to keep advancing dungeon_time to try
            // to keep up with wall clock time, by making "update"
            // calls.
            unsigned int dungeon_time = wall_clock_time;

            while (1) {
                // Invariant: at this point, wall_clock_time equals
                // the current value of timer->getMsec(), or as close
                // as possible to it.

                // The dungeon time is always at, or behind, wall
                // clock time. Calculate exactly how much behind it
                // is.
                int update_delta_t = wall_clock_time - dungeon_time;

                // Simulate the dungeon forward in time until it
                // catches up with wall_clock_time (if necessary).
                if (update_delta_t > 0) {

                    // We cap the actual dungeon update to 1000 ms;
                    // this prevents excessively long updates.
                    int capped_update_delta_t = update_delta_t > 1000 ? 1000 : update_delta_t;

                    // Do the actual game update. This simulates all
                    // knights, monsters, etc., for a period of
                    // "capped_update_delta_t".
                    const bool should_continue = update(capped_update_delta_t);

                    // We always advance dungeon_time by the full
                    // amount, even if the update was capped.
                    dungeon_time += update_delta_t;

                    // Is the game over? If so, we are done here.
                    if (!should_continue) return;

                }

                // Note: at this point, dungeon_time should be equal
                // to wall_clock_time. (It could also be greater, but
                // only if wall_clock_time ran backwards, which should
                // never happen.)

                // Schedule the next update.
                const unsigned int next_update_time = wall_clock_time + calculateUpdateDelay();

                // Sleep until next_update_time, or until something of
                // interest happens (e.g. a new player input is
                // received).
                wall_clock_time = sleepUntil(next_update_time);
            }
        }

        int calculateUpdateDelay() const
        {
            const int delay = engine->getTimeToNextUpdate();
            const int cap = 250;
            if (delay > cap) {
                // Make sure to do at least one update every "cap" ms,
                // just to keep things moving.
                return cap;
            } else if (delay < 1) {
                // Always delay at least 1 millisecond, to avoid loops
                // where we just busy-wait until the timer advances by
                // one!
                return 1;
            } else {
                return delay;
            }
        }

        // This sleeps until either the main thread signals us, or
        // timer->getMsec() reaches (approximately) target_time. It
        // returns the new value of timer->getMsec().
        unsigned int sleepUntil(unsigned int target_time)
        {
            while (true) {

                // Calculate how long we need to wait
                unsigned int time_now = timer->getMsec();
                int wait_time = target_time - time_now;

                // If wait time is zero or negative, then we are done
                if (wait_time <= 0) {
                    return time_now;
                }

                // Otherwise, sleep for an appropriate period.
#ifndef VIRTUAL_SERVER
                boost::unique_lock<boost::mutex> lock(kg.my_mutex);
#endif

                // The call to timed_wait UNLOCKS the mutex, WAITS for
                // either the time to expire, or the main loop to
                // signal us, then LOCKS the mutex again before
                // returning.
#ifdef VIRTUAL_SERVER
                vs_switch_to_main_thread(wait_time);
#else
                kg.wake_up_cond_var.timed_wait(lock,
                    boost::posix_time::milliseconds(wait_time));
#endif

                if (kg.wake_up_flag) {
                    // The reason that timed_wait() returned was that
                    // the main loop signalled us. Acknowledge the
                    // signal (by clearing the flag), then return
                    // early.
                    kg.wake_up_flag = false;
                    return timer->getMsec();
                }

                // Loop again; this will either return (if the
                // target_time has been reached) or go back to sleep
                // (if the wake was spurious).
            }
        }

        // Main function for dungeon update.
        // Returns true if the game should continue or false if we should quit.
        // Note, if returning false, update() is responsible for sending out
        // SERVER_GOTO_MENU (or whatever else it wants to do).
        bool update(int time_delta)
        {
            // Note: mutex must be locked during the whole update cycle, because
            // KnightsEngine is accessing things like the lua state, which the
            // main thread is also accessing at the same time.
#ifndef VIRTUAL_SERVER
            boost::lock_guard<boost::mutex> lock(kg.my_mutex);
#endif

            // Pre-update activities
            if (!preUpdate()) return true;

            // Update the dungeon
            engine->update(time_delta, *callbacks);

            // Post-update activities
            return postUpdate(time_delta);
        }

        // Prepare for a dungeon update, e.g. put eliminated players into observer mode,
        // catch up new observers (or reconnecting players).
        // Returns true if should continue to the main update, or false if game is paused.
        bool preUpdate()
        {
            // Add any new players to the game.
            for (boost::shared_ptr<GameConnection> conn : kg.incoming_connections) {
                AddNewPlayer(kg, conn, engine.get(), callbacks.get());
            }
            kg.incoming_connections.clear();

            // Delete any "dead" observers.
            for (std::vector<int>::iterator it = kg.delete_observer_nums.begin(); it != kg.delete_observer_nums.end(); ++it) {
                callbacks->rmObserverNum(*it);
            }
            kg.delete_observer_nums.clear();

            // Disconnect any leaving players.
            for (int player_num : kg.pending_disconnections) {
                engine->changePlayerState(player_num, PlayerState::DISCONNECTED);
            }
            kg.pending_disconnections.clear();

            // Put eliminated players into obs mode (Trac #10)
            std::vector<int> put_into_obs_mode = callbacks->getPlayersToPutIntoObsMode();
            for (std::vector<int>::const_iterator p = put_into_obs_mode.begin(); p != put_into_obs_mode.end(); ++p) {
                for (game_conn_vector::iterator c = kg.connections.begin(); c != kg.connections.end(); ++c) {
                    if ((*c)->player_num == *p) {
                        // Send him the 'go into obs mode' command
                        Coercri::OutputByteBuf buf((*c)->output_data);
                        buf.writeUbyte(SERVER_GO_INTO_OBS_MODE);
                        buf.writeUbyte(kg.all_player_ids.size());
                        for (auto const & id : kg.all_player_ids) {
                            buf.writeString(id.asString());
                        }

                        // Set his obs_num to zero (which means "observer num not allocated yet")
                        // and his obs_flag to true. (Leave player_num unchanged.)
                        (*c)->obs_flag = true;
                        (*c)->observer_num = 0;
                        (*c)->cancel_obs_mode_after_game = true;
                    }
                }
            }

            // Detect whether the game is paused
            // (Pausing is only allowed in split-screen mode)
            if (kg.pause_mode && kg.allow_split_screen) {
                return false;  // Do not proceed any further
            }

            // Check for players that need to catch up
            for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                if ((*it)->requires_catchup && (*it)->finished_loading) {
                    std::vector<unsigned char> & buf = (*it)->output_data;

                    if ((*it)->obs_flag) {
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

                    } else {
                        // Catchup for reconnecting players (as opposed to observers)
                        engine->catchUp((*it)->player_num, *callbacks);
                        engine->changePlayerState((*it)->player_num, PlayerState::NORMAL);
                    }

                    (*it)->requires_catchup = false;
                }
            }

            // Ready to proceed with the actual update!
            return true;
        }

        // Do stuff that needs to happen after the dungeon has caught up to the current
        // wall clock time, like inputting player actions, and copying any outputs back to
        // the GameConnection buffers (ready for sending to the network).
        // Returns true if game is still running, or false if it has ended.
        bool postUpdate(int time_delta)
        {
            // Read control inputs
            for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                if (!(*it)->obs_flag) {
                    for (int p = 0; p < ((*it)->id2.empty() ? 1 : 2); ++p) {

                        const UserControl *final_ctrl = 0;

                        for (std::vector<const UserControl*>::const_iterator c = (*it)->control_queue[p].begin();
                        c != (*it)->control_queue[p].end();
                        ++c) {
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
                }
            }

            // Copy output data back to GameConnection buffers
            for (game_conn_vector::iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                if (!(*it)->finished_loading) {
                    // Skip players who haven't finished loading yet
                    continue;
                }
                if ((*it)->obs_flag) {
                    if ((*it)->observer_num > 0) {
                        // Send observer cmds to that player. (We're assuming observation is always allowed at the moment.)
                        callbacks->appendObserverCmds((*it)->observer_num, (*it)->output_data);
                    }
                } else {
                    const bool split_screen = !(*it)->id2.empty();
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

            // Do player list update every N seconds, or if engine says it is "dirty"
            time_to_player_list_update -= time_delta;
            if (engine->isPlayerListDirty() || time_to_player_list_update <= 0) {
                doPlayerListUpdate();
                if (time_to_player_list_update <= 0) time_to_player_list_update = 3000;  // 3 seconds
            }

            // Check to see if the game is over
            if (!game_over_sent && callbacks->isGameOver()) {
                kg.game_over = true;
                game_over_sent = true;
                time_to_force_quit = 60000;

                // find player IDs of all players, and the winner
                std::vector<PlayerID> loser_ids;
                PlayerID winner_id;
                for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                    if (!(*it)->obs_flag) {
                        if ((*it)->player_num == callbacks->getWinnerNum()) {
                            winner_id = (*it)->id1;
                        } else {
                            loser_ids.push_back((*it)->id1);
                        }
                        // note: id2 not supported here currently.
                    }
                }

                if (kg.knights_log) {
                    std::string msg;
                    msg = kg.game_name + "\tgame won\t";
                    // the first name printed is the winner, then we list the losers (of which there will be
                    // only one at the moment).
                    msg += "winner=" + winner_id.asString();
                    for (auto const& loser_id : loser_ids) {
                        msg += std::string(", loser=") + loser_id.asString();
                    }
                    kg.knights_log->logMessage(msg);
                }
            }

            // "Force quit" countdown. This forces the winner/loser
            // screen to close after a certain time (Trac #7).
            if (time_to_force_quit > 0) {
                time_to_force_quit -= time_delta;
                if (time_to_force_quit <= 0) {
                    ReturnToMenu(kg);
                    return false;  // force game loop to quit
                }
            }

            // Let the game continue!
            return true;
        }

        // Player list update function - this is called every few seconds or
        // when the number of kills changes.
        // The mutex is locked when this is called.
        void doPlayerListUpdate()
        {
            std::vector<PlayerInfo> player_list;
            engine->getPlayerList(player_list);

            // Filter out former players who have left (i.e. they are no longer connected to the game).
            // Note: Temporarily disconnected players (DISCONNECTED state) are an exception to
            // this rule, because they might come back later.
            for (size_t idx = 0; idx < player_list.size(); ++idx) {
                const int num = player_list[idx].player_num;
                bool found = false;
                for (auto const & connection : kg.connections) {
                    if (connection->player_num == num) {
                        found = true;
                        break;
                    }
                }
                if (found || player_list[idx].player_state == PlayerState::DISCONNECTED) {
                    ++idx;
                } else {
                    player_list.erase(player_list.begin() + idx);
                }
            }

            // Associate a ping time with each player.
            // -- Only do this if the timer ran out, not if we are updating following a death,
            // otherwise it looks strange when the pings update just because someone died.
            if (time_to_player_list_update <= 0) {
                for (size_t idx = 0; idx < player_list.size(); ++idx) {
                    const int num = player_list[idx].player_num;
                    for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                        if ((*it)->player_num == num) {
                            pings[(*it)->id1] = (*it)->ping_time;
                        }
                    }
                }
            }

            // now add observers to the list (Trac #26)
            for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                if ((*it)->player_num == -1) {
                    ASSERT((*it)->obs_flag);  // when game is running, player_num == -1 implies obs_flag
                    PlayerInfo pi;
                    pi.id = (*it)->id1;
                    pi.house_colour = Coercri::Color(0,0,0);
                    pi.player_num = -1;
                    pi.kills = -1;
                    pi.deaths = -1;
                    pi.frags = -1000;
                    pi.player_state = PlayerState::NORMAL;  // player_state doesn't matter for observers
                    player_list.push_back(pi);
                    pings[pi.id] = (*it)->ping_time;
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
                    buf.writeString(player_list[idx].id.asString());
                    buf.writeUbyte(player_list[idx].house_colour.r);
                    buf.writeUbyte(player_list[idx].house_colour.g);
                    buf.writeUbyte(player_list[idx].house_colour.b);
                    buf.writeVarInt(player_list[idx].kills);
                    buf.writeVarInt(player_list[idx].deaths);
                    buf.writeVarInt(player_list[idx].frags);
                    buf.writeVarInt(pings[player_list[idx].id]);
                    
                    // Add status byte: 0=NORMAL, 1=ELIMINATED, 2=DISCONNECTED, 3=OBSERVER
                    unsigned char status_byte = 0;
                    if (player_list[idx].player_num == -1) {
                        status_byte = 3;  // Observer
                    } else {
                        switch (player_list[idx].player_state) {
                        case PlayerState::NORMAL:
                            status_byte = 0;
                            break;
                        case PlayerState::ELIMINATED:
                            status_byte = 1;
                            break;
                        case PlayerState::DISCONNECTED:
                            status_byte = 2;
                            break;
                        }
                    }
                    buf.writeUbyte(status_byte);
                }

                if (time_remaining > -1) {
                    buf.writeUbyte(SERVER_TIME_REMAINING);
                    buf.writeVarInt(time_remaining);
                }
            }
        }

    private:
        KnightsGameImpl &kg;
        boost::shared_ptr<Coercri::Timer> timer;
        boost::shared_ptr<ServerCallbacks> callbacks;
        boost::shared_ptr<KnightsEngine> engine;
        std::map<PlayerID, int> pings;
        int nplayers;
        bool game_over_sent;
        int time_to_player_list_update;
        int time_to_force_quit;
    };

    void DoSetReady(KnightsGameImpl &kg, GameConnection &conn, bool ready)
    {
        conn.is_ready = ready;

        // send notification to players
        for (game_conn_vector::iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
            Coercri::OutputByteBuf out((*it)->output_data);
            out.writeUbyte(SERVER_SET_READY);
            out.writeString(conn.id1.asString());
            out.writeUbyte(ready ? 1 : 0);
            if (!conn.id2.empty()) {
                out.writeUbyte(SERVER_SET_READY);
                out.writeString(conn.id2.asString());
                out.writeUbyte(ready ? 1 : 0);
            }
        }
    }

    void StartGameIfReady(KnightsGameImpl &kg)
    {
        // mutex is UNLOCKED at this point

#ifdef VIRTUAL_SERVER
        if (vs_game_thread_running()) return;  // Game is already started
#else
        if (kg.update_thread.joinable()) return;  // Game is already started.
#endif

        // game thread definitely not running at this point... so it is safe to access kg
        
        std::vector<PlayerID> ids;
        
        // count how many players are ready to leave the lobby, and how many there are total
        int nready = 0;
        int nplayers = 0;
        for (game_conn_vector::iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
            if (!(*it)->obs_flag) {
                (*it)->player_num = nplayers;  // assign player_nums
                const int ncount = (*it)->id2.empty() ? 1 : 2;
                nplayers += ncount;
                if ((*it)->is_ready) {
                    nready += ncount;
                    ids.push_back((*it)->id1);
                    if (ncount==2) ids.push_back((*it)->id2);
                }
            }
        }

        // If all players are ready then start the game.
        bool ready_to_start = (nready == nplayers && nplayers >= 1);

        // Don't start if there are insufficient players for the current quest
        LocalMsg nplyrs_err_msg;
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

                // observers always require an initial catchup
                (*it)->requires_catchup = (*it)->obs_flag;
            }

            // Clear 'pending_disconnections' in case there is anything still in there from the previous game
            kg.pending_disconnections.clear();

            // Reset game_over to false for the new game
            kg.game_over = false;

            // Start the update thread.
            kg.update_thread_wants_to_exit = false;
            kg.emergency_exit = false;
            kg.emergency_err_msg.clear();
            kg.startup_signal = false;
            kg.startup_err = {};

#ifdef VIRTUAL_SERVER
            kg.update_thread.reset(new UpdateThread(kg, kg.timer));
            vs_start_game_thread(reinterpret_cast<void*>(&UpdateThread::operator()),
                                 static_cast<void*>(kg.update_thread.get()));
#else
            UpdateThread thr(kg, kg.timer);
            boost::thread new_thread(thr);
            kg.update_thread.swap(new_thread);  // start the sub-thread -- game is now running.
#endif

            // Wait for the sub thread to set the "startup_signal" flag.
            while (1) {
                if (kg.startup_signal) {
                    // The sub-thread is ready to proceed
                    break;
                }

                if (kg.update_thread_wants_to_exit) {
                    // The sub-thread has exited. This usually signals an error of some sort.
#ifdef VIRTUAL_SERVER
                    // Sanity check: Game thread should have exited at this point
                    if (vs_game_thread_running()) {
                        throw std::runtime_error("Game thread should have exited");
                    }
                    kg.update_thread.reset();
#else
                    kg.update_thread.join();
#endif

                    if (kg.emergency_exit) {
                        // very serious error -- escalate to our caller.
                        throw LuaError("Fatal Lua error during startup: " + kg.emergency_err_msg);
                    }

                    if (kg.startup_err.key == LocalKey()) {
                        kg.startup_err = {LocalKey("update_thread_failed")};
                    }

                    break;
                }

#ifdef VIRTUAL_SERVER
                // Allow game thread init code to run
                vs_switch_to_game_thread();

                // The game thread should have set either kg.startup_signal or
                // kg.update_thread_wants_to_exit before switching back to main thread.
                // If not, it is an error.
                if (!kg.startup_signal && !kg.update_thread_wants_to_exit) {
                    throw std::runtime_error("Game thread did not init properly");
                }
#else
                // Sleep while we wait for the game thread init code to complete.
                boost::this_thread::sleep(boost::posix_time::milliseconds(100));
#endif
            }

            // Lock the mutex
#ifndef VIRTUAL_SERVER
            boost::lock_guard<boost::mutex> lock(kg.my_mutex);
#endif
            
            // If the game failed to initialize then print a message and refuse to start the game.
            // Otherwise, send startup message to all players.
            const LocalMsg err_msg = kg.startup_err;
            if (err_msg.key != LocalKey()) {

                Announcement(kg, LocalMsg{LocalKey("couldnt_start_game")}, false);
                Announcement(kg, err_msg, true);

                // log the error as well
                if (kg.knights_log) {
                    kg.knights_log->logMessage(kg.game_name + "\tError starting game\t" + err_msg.key.getKey());
                }

#ifdef VIRTUAL_SERVER
                // Sanity check: Game thread should have exited if err_msg was set
                if (vs_game_thread_running()) {
                    throw std::runtime_error("Game thread should have exited at this point!");
                }
                kg.update_thread.reset();
#else
                kg.update_thread.join();
#endif

            } else {
                // No errors -- We can start the game now

                // Send startGame msgs
                for (game_conn_vector::iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {

                    unsigned char num_displays;
                    if (!(*it)->id2.empty()) num_displays = 2;  // split screen mode
                    else if ((*it)->obs_flag) num_displays = nplayers;  // observer mode
                    else num_displays = 1;   // normal mode

                    if (!(*it)->obs_flag) {
                        // Player.
                        Coercri::OutputByteBuf buf((*it)->output_data);
                        buf.writeUbyte(SERVER_START_GAME);
                        buf.writeUbyte(num_displays);
                        buf.writeUbyte(kg.deathmatch_mode);
                        buf.writeUbyte(0);  // already_started flag (false)
                    } else {
                        // Observer.
                        Coercri::OutputByteBuf buf((*it)->output_data);
                        buf.writeUbyte(SERVER_START_GAME_OBS);
                        buf.writeUbyte(num_displays);
                        buf.writeUbyte(kg.deathmatch_mode);
                        for (int i = 0; i < num_displays; ++i) {
                            buf.writeString(ids[i].asString());
                        }
                        buf.writeUbyte(0);  // already_started flag (false)
                    }
                }

                // Log a message if required.
                if (kg.knights_log) {
                    std::ostringstream str;
                    str << kg.game_name << "\tgame started\t";
                    for (auto it = ids.begin(); it != ids.end(); ++it) {
                        if (it != ids.begin()) str << ", ";
                        str << it->asString();
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

#ifdef VIRTUAL_SERVER
        vs_interrupt_game_thread();
        kg.update_thread.reset();
#else
        kg.update_thread.interrupt();
        kg.update_thread.join();
#endif

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
            throw std::runtime_error("Could not read file (on server): " + fi.getPath());
        }
    }
}

KnightsGame::KnightsGame(boost::shared_ptr<KnightsConfig> config,
                         boost::shared_ptr<Coercri::Timer> tmr,
                         bool allow_split_screen,
                         KnightsLog *knights_log,
                         const std::string &game_name)
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

    pimpl->wake_up_flag = false;
}

KnightsGame::~KnightsGame()
{
    // If the update thread is still running then we have to kill it
    // here. The thread dtor will not kill the thread for us, instead
    // the thread will just become "detached" and continue running,
    // with disastrous consequences if the KnightsGame object is being
    // destroyed!
#ifdef VIRTUAL_SERVER
    vs_interrupt_game_thread();
    pimpl->update_thread.reset();
#else
    pimpl->update_thread.interrupt();
    pimpl->update_thread.join();
#endif
}

int KnightsGame::getNumPlayers() const
{
#ifndef VIRTUAL_SERVER
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif
    return CountPlayers(*pimpl);
}

int KnightsGame::getNumObservers() const
{
#ifndef VIRTUAL_SERVER
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif
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
#ifdef VIRTUAL_SERVER
    if (vs_game_thread_running()) return GS_RUNNING;
#else
    if (pimpl->update_thread.joinable()) return GS_RUNNING;
#endif
    else if (getNumPlayers() < 2) return GS_WAITING_FOR_PLAYERS;
    else return GS_SELECTING_QUEST;
}

bool KnightsGame::getObsFlag(GameConnection &conn) const
{
#ifndef VIRTUAL_SERVER
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif
    return conn.obs_flag;
}

GameConnection & KnightsGame::newClientConnection(const PlayerID &client_id, const PlayerID &client_id_2,
                                                  int client_version, bool approach_based_controls, bool action_bar_controls)
{
#ifndef VIRTUAL_SERVER
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif

    // check preconditions. (caller should have checked these already.)
    if (client_id.empty()) throw UnexpectedError("invalid client id");
    for (game_conn_vector::const_iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
        if ((*it)->id1 == client_id || (*it)->id2 == client_id) throw UnexpectedError("Duplicate client ID");
        if (!client_id_2.empty() && ((*it)->id1 == client_id_2 || (*it)->id2 == client_id_2)) throw UnexpectedError("Duplicate client ID");
    }
    if (!pimpl->allow_split_screen && !client_id_2.empty()) throw UnexpectedError("Split screen mode not allowed");
    if (!client_id_2.empty() && !pimpl->connections.empty()) throw UnexpectedError("Cannot join in split screen mode while connections exist");

    const bool game_in_progress =
#ifdef VIRTUAL_SERVER
        vs_game_thread_running();
#else
        pimpl->update_thread.joinable();
#endif

    // create the GameConnection
    boost::shared_ptr<GameConnection> conn(new GameConnection(client_id, client_id_2,
                                                              false, client_version,
                                                              approach_based_controls, action_bar_controls));

    // If game in progress, push to "incoming_connections" and exit early
    if (game_in_progress) {
        pimpl->incoming_connections.push_back(conn);
        conn->requires_catchup = true;
        return *conn;
    }

    // Game not in progress.

    // Assign the player a unique house colour if possible.
    // First check if this player previously had a house colour.
    int prev_idx = FindPreviousHouseColour(*pimpl, client_id);
    if (prev_idx >= 0) {
        // Restore previous colour
        conn->house_colour = pimpl->previous_house_colours[prev_idx].second;
    } else {
        // New player: find first available colour
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

    // Add the player to 'connections' and send joining messages
    AddNewPlayer(*pimpl, conn, nullptr, nullptr);

    return *conn;
}

void KnightsGame::clientLeftGame(GameConnection &conn)
{
    // This makes the given client leave the game. (Either they are going
    // back to the main lobby, or they are leaving the server entirely.)

    bool is_player = false;
    
    PlayerID id1, id2;
    int player_num = 0;
    int num_player_connections = 0;

    {
#ifndef VIRTUAL_SERVER
        boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif

        // Find the client in our connections list.
        game_conn_vector::iterator where = std::find_if(pimpl->connections.begin(), 
                                                        pimpl->connections.end(), ShPtrEq<GameConnection>(&conn));
        ASSERT(where != pimpl->connections.end());
        
        // Determine whether he is a player or observer. Also save his ID(s)
        is_player = !(*where)->obs_flag;
        player_num = (*where)->player_num;
        id1 = (*where)->id1;
        id2 = (*where)->id2;
        if ((*where)->observer_num > 0) {
            pimpl->delete_observer_nums.push_back( (*where)->observer_num );
        }

        // Save house colour for potential reconnection (only for players, not observers)
        if (is_player) {
            int idx = FindPreviousHouseColour(*pimpl, id1);
            if (idx >= 0) {
                // Update existing entry
                pimpl->previous_house_colours[idx].second = (*where)->house_colour;
            } else {
                // Add new entry at next position (circular)
                pimpl->previous_house_colours[pimpl->previous_house_colours_next] = std::make_pair(id1, (*where)->house_colour);
                pimpl->previous_house_colours_next = (pimpl->previous_house_colours_next + 1) % HOUSE_COLOUR_CIRCULAR_BUFFER_SIZE;
            }
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

    bool is_running;
#ifdef VIRTUAL_SERVER
    is_running = vs_game_thread_running();
#else
    is_running = pimpl->update_thread.joinable();
#endif
    if (is_player && is_running) {
        // The leaving client is one of the players (as opposed to an observer).

#ifndef VIRTUAL_SERVER
        if (num_player_connections == 0) {
            // This is a non-virtual server and the last player left the game.
            // Go back to the quest selection menu.
            StopGameAndReturnToMenu(*pimpl);
        }
#endif

        // Mark this player disconnected and allow the game to continue.
#ifndef VIRTUAL_SERVER
        boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif
        pimpl->pending_disconnections.push_back(player_num);
    }

    {
#ifndef VIRTUAL_SERVER
        boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif

        // Send SERVER_PLAYER_LEFT_THIS_GAME message to all remaining players
        for (game_conn_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
            Coercri::OutputByteBuf buf((*it)->output_data);
            buf.writeUbyte(SERVER_PLAYER_LEFT_THIS_GAME);
            buf.writeString(id1.asString());
            buf.writeUbyte(is_player ? 0 : 1);
            if (!id2.empty()) {
                buf.writeUbyte(SERVER_PLAYER_LEFT_THIS_GAME);
                buf.writeString(id2.asString());
                buf.writeUbyte(is_player ? 0 : 1);
            }
        }

        if (pimpl->connections.empty()) {
            // If there are no connections left (and we are not in virtual server mode)
            // then reset the menu selections.
#ifndef VIRTUAL_SERVER
            pimpl->knights_config->resetMenu();
#endif
        } else if (is_player) {
            // Number of players has changed, may need to update menu constraints.
            UpdateNumPlayersAndTeams(*pimpl);
        }
    }

    // If all remaining players are ready, then the game should start (Trac #25)
    StartGameIfReady(*pimpl);
}


void KnightsGame::sendChatMessage(GameConnection &conn, const Coercri::UTF8String &msg_orig)
{
    bool is_running;
#ifdef VIRTUAL_SERVER
    is_running = vs_game_thread_running();
#else
    is_running = pimpl->update_thread.joinable();
#endif

    // Determine whether this is a Team chat message
    bool is_team;
    UTF8String msg;
    if (!is_running || conn.obs_flag) {
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
#ifndef VIRTUAL_SERVER
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif
    for (game_conn_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {

        if (is_team) {
            // Skip over players of different team, or observers
            if ((*it)->obs_flag || (*it)->house_colour != conn.house_colour) continue;
        }

        Coercri::OutputByteBuf buf((*it)->output_data);
        buf.writeUbyte(SERVER_CHAT);
        buf.writeString(conn.id1.asString());

        if (is_team) {
            buf.writeUbyte(3);  // Team Message
        } else if (conn.obs_flag) {
            buf.writeUbyte(2);  // Message from an observer
        } else {
            buf.writeUbyte(1);  // Normal Message
        }
        buf.writeString(msg.asUTF8());
    }
}

void KnightsGame::setReady(GameConnection &conn, bool ready)
{
#ifdef VIRTUAL_SERVER
    if (vs_game_thread_running()) return;
#else
    if (pimpl->update_thread.joinable()) return;  // Game is running.
#endif

    // (We know the game is not running at this point, so no need to lock mutex)
    
    if (!conn.obs_flag) {
        // Set conn.is_ready and send out notifications
        DoSetReady(*pimpl, conn, ready);

        // Start the game if all players are now ready.
        StartGameIfReady(*pimpl);
    }
}

void KnightsGame::setHouseColour(GameConnection &conn, int hse_col)
{
#ifdef VIRTUAL_SERVER
    if (vs_game_thread_running()) return;
#else
    if (pimpl->update_thread.joinable()) return;  // Game is running
#endif

#ifndef VIRTUAL_SERVER
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif
    if (!conn.obs_flag) {
        conn.house_colour = hse_col;

        // send notification to players
        for (game_conn_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
            Coercri::OutputByteBuf out((*it)->output_data);
            out.writeUbyte(SERVER_SET_HOUSE_COLOUR);
            // note we don't support house colours in split screen mode at the moment. we assume it's the first player
            // on the connection who is having the house colours set.
            out.writeString(conn.id1.asString());
            out.writeUbyte(hse_col);
        }

        UpdateNumPlayersAndTeams(*pimpl);
    }
}

void KnightsGame::finishedLoading(GameConnection &conn)
{
#ifndef VIRTUAL_SERVER
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif
    conn.finished_loading = true;
}

void KnightsGame::readyToEnd(GameConnection &conn)
{
    if (!pimpl->game_over) return;

    bool all_ready = false;
    {
#ifndef VIRTUAL_SERVER
        boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif
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
            out.writeString(conn.id1.asString());
        }
    }
}

void KnightsGame::setPauseMode(bool pm)
{
#ifndef VIRTUAL_SERVER
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif
    pimpl->pause_mode = pm;
}

void KnightsGame::setMenuSelection(GameConnection &conn, int item_num, int choice_num)
{
#ifdef VIRTUAL_SERVER
    if (vs_game_thread_running()) return;  // Game is running
#else
    if (pimpl->update_thread.joinable()) return;  // Game is running
#endif

    // validate input
    const int num_items = pimpl->knights_config->getMenu().getNumItems();
    if (item_num < 0 || item_num >= num_items) return;
    const MenuItem & item = pimpl->knights_config->getMenu().getItem(item_num);
    if (!item.isNumeric() && (choice_num < 0 || choice_num >= item.getNumChoices())) return;

    // check we are a player
    // note: we know update thread is not running (see above) so no need to lock
    if (conn.obs_flag) return;   // only players can adjust the menu.

    // send out the settings change(s) to all players
    MyMenuListener listener;
    for (game_conn_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
        listener.addBuf((*it)->output_data);
    }
    pimpl->knights_config->changeMenuSetting(item_num, choice_num, listener);

    if (listener.wereThereChanges()) {
        // send announcement to all players
        const MenuItem & item = pimpl->knights_config->getMenu().getItem(item_num);

        LocalMsg msg{LocalKey("player_set_menu")};
        msg.params.push_back(LocalParam(conn.id1));
        msg.params.push_back(LocalParam(item.getTitleKey()));

        if (item.isNumeric()) {
            msg.params.push_back(LocalParam(choice_num));
        } else {
            const LocalKeyOrInteger &lki = item.getChoice(choice_num);
            if (lki.is_integer) {
                msg.params.push_back(LocalParam(lki.integer));
            } else {
                msg.params.push_back(LocalParam(lki.local_key));
            }
        }

        Announcement(*pimpl, msg);

        // deactivate all ready flags
        DeactivateReadyFlags(*pimpl);
    }
}

void KnightsGame::randomQuest(GameConnection &conn)
{
#ifdef VIRTUAL_SERVER
    if (vs_game_thread_running()) return;   // Game is running
#else
    if (pimpl->update_thread.joinable()) return;  // Game is running
#endif
    
#ifndef VIRTUAL_SERVER
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif
    if (conn.obs_flag) return;   // only players can set quests.

    MyMenuListener listener;
    for (game_conn_vector::iterator it = pimpl->connections.begin(); it != pimpl->connections.end(); ++it) {
        listener.addBuf((*it)->output_data);
    }
    pimpl->knights_config->randomQuest(listener);

    // send announcement to all players
    LocalMsg msg{LocalKey("player_set_random"), {LocalParam(conn.id1)}};
    Announcement(*pimpl, msg);

    // deactivate all ready flags
    DeactivateReadyFlags(*pimpl);
}

void KnightsGame::requestGraphics(Coercri::OutputByteBuf &buf, const std::vector<int> &ids)
{
#ifndef VIRTUAL_SERVER
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif
    std::vector<const Graphic *> graphics;
    pimpl->knights_config->getGraphics(graphics);
    
    buf.writeUbyte(SERVER_SEND_GRAPHICS);
    buf.writeVarInt(ids.size());
    
    for (std::vector<int>::const_iterator it = ids.begin(); it != ids.end(); ++it) {
        if (*it <= 0 || static_cast<size_t>(*it) > graphics.size()) {
            throw ProtocolError(LocalKey("invalid_id_server"));
        }
        buf.writeVarInt(*it);
        ASSERT(graphics[*it - 1]->getID() == *it);
        SendFile(buf, graphics[*it - 1]->getFileInfo());
    }
}

void KnightsGame::requestSounds(Coercri::OutputByteBuf &buf, const std::vector<int> &ids)
{
#ifndef VIRTUAL_SERVER
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif
    std::vector<const Sound *> sounds;
    pimpl->knights_config->getSounds(sounds);
    
    buf.writeUbyte(SERVER_SEND_SOUNDS);
    buf.writeVarInt(ids.size());
    
    for (std::vector<int>::const_iterator it = ids.begin(); it != ids.end(); ++it) {
        if (*it <= 0 || static_cast<size_t>(*it) > sounds.size()) {
            throw ProtocolError(LocalKey("invalid_id_server"));
        }
        buf.writeVarInt(*it);
        ASSERT(sounds[*it - 1]->getID() == *it);
        SendFile(buf, sounds[*it - 1]->getFileInfo());
    }
}

void KnightsGame::sendControl(GameConnection &conn, int p, unsigned char control_num)
{
#ifdef VIRTUAL_SERVER
    if (!vs_game_thread_running()) return;  // Game is not running
#else
    if (!pimpl->update_thread.joinable()) return; // Game is not running
#endif

#ifndef VIRTUAL_SERVER
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif
    if (!conn.obs_flag && p >= 0 && p < (conn.id2.empty() ? 1 : 2)) {
        const UserControl * control = control_num == 0 ? 0 : pimpl->controls.at(control_num - 1);
        conn.control_queue[p].push_back(control);
        pimpl->wake_up_flag = true;
    }
}

void KnightsGame::requestSpeechBubble(GameConnection &conn, bool show)
{
#ifdef VIRTUAL_SERVER
    if (!vs_game_thread_running()) return;   // Game is not running
#else
    if (!pimpl->update_thread.joinable()) return;  // Game is not running
#endif

#ifndef VIRTUAL_SERVER
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif
    if (!conn.obs_flag) {
        conn.speech_request = true;
        conn.speech_bubble = show;
        pimpl->wake_up_flag = true;
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

#ifndef VIRTUAL_SERVER
    bool flag;
    {
        boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
        flag = (pimpl->wake_up_flag);
    }
    if (flag) pimpl->wake_up_cond_var.notify_one();
#endif
}

void KnightsGame::setObsFlag(GameConnection &conn, bool new_obs_flag)
{
#ifndef VIRTUAL_SERVER
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif

    if (conn.obs_flag == new_obs_flag) return;  // Nothing to do
    
    // If the game is in progress then don't allow players to change obs-flag setting
#ifdef VIRTUAL_SERVER
    if (vs_game_thread_running()) {
#else
    if (pimpl->update_thread.joinable()) {
#endif
        Coercri::OutputByteBuf buf(conn.output_data);
        buf.writeUbyte(SERVER_ANNOUNCEMENT_LOC);
        WriteLocalMsg(buf, LocalMsg{LocalKey("cant_change_obs")});
        return;
    }

    // If he is becoming an observer, and he is currently 'ready to start', then make him non-ready first.
    if (new_obs_flag && conn.is_ready) {
        DoSetReady(*pimpl, conn, false);
    }
    
    // changing obs_flag does not apply to split screen games, so we can ignore id2 and just use id1.
    const PlayerID & my_id = conn.id1;

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
        buf.writeString(my_id.asString());
        buf.writeUbyte(new_obs_flag ? 1 : 0);
        
        if (!new_obs_flag) {
            buf.writeUbyte(SERVER_SET_HOUSE_COLOUR);
            buf.writeString(my_id.asString());
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
#ifndef VIRTUAL_SERVER
        boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif
        std::swap(conn.output_data, data);
        conn.output_data.clear();
        do_wait = pimpl->update_thread_wants_to_exit;
    }

    // Since getOutputData is called regularly, this is a good place
    // to join() with the update thread, if the update thread has
    // finished.
    if (do_wait) {
#ifdef VIRTUAL_SERVER
        // If pimpl->update_thread_wants_to_exit is true, then game thread should have exited by now
        if (vs_game_thread_running()) {
            throw std::runtime_error("Game thread did not exit when expected to");
        }
        pimpl->update_thread.reset();
#else
        if (pimpl->update_thread.joinable()) pimpl->update_thread.join();
#endif
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
#ifndef VIRTUAL_SERVER
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
#endif
    conn.ping_time = ping;
}

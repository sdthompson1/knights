/*
 * knights_game.cpp
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

#include "anim.hpp"
#include "graphic.hpp"
#include "knights_config.hpp"
#include "knights_engine.hpp"
#include "knights_game.hpp"
#include "knights_log.hpp"
#include "menu.hpp"
#include "menu_constraints.hpp"
#include "menu_selections.hpp"
#include "overlay.hpp"
#include "protocol.hpp"
#include "rng.hpp"
#include "server_callbacks.hpp"
#include "sh_ptr_eq.hpp"
#include "sound.hpp"
#include "user_control.hpp"

#include "boost/scoped_ptr.hpp"
#include "boost/weak_ptr.hpp"
#include "boost/thread/locks.hpp"
#include "boost/thread/thread.hpp"

#include <algorithm>
#include <cctype>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

class GameConnection {
public:
    GameConnection(const std::string &n, const std::string &n2, bool new_obs_flag, int ver,
                   bool approach_based_ctrls, bool action_bar_ctrls)
        : name(n), name2(n2),
          is_ready(false), finished_loading(false), ready_to_end(false), 
          obs_flag(new_obs_flag), cancel_obs_mode_after_game(false), house_colour(0),
          client_version(ver),
          observer_num(0),
          ping_time(0),
          speech_request(false), speech_bubble(false),
          approach_based_controls(approach_based_ctrls),
          action_bar_controls(action_bar_ctrls)
    { }
    
    std::string name, name2;  // if name2.empty() then normal connection, else split screen connection.
    
    bool is_ready;    // true=ready to start game, false=want to stay in lobby
    bool finished_loading;   // true=ready to play, false=still loading
    bool ready_to_end;   // true=clicked mouse on winner/loser screen, false=still waiting.
    bool obs_flag;   // true=observer, false=player
    bool cancel_obs_mode_after_game;
    int house_colour;  // Must fit in a ubyte. Set to zero for observers (but beware, zero is also a valid house colour for non-observers!).

    int client_version;
    int observer_num;   // 0 if not an observer, or not set yet. >0 if set.
    int player_num;     // 0..num_players-1, or undefined if the game is not running.
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
    bool tutorial_mode;

    std::vector<const UserControl*> controls;
    
    MenuSelections menu_selections;
    std::string quest_description;
    game_conn_vector connections;

    bool game_over;
    bool pause_mode;
    
    // methods of KnightsGame should (usually) lock this mutex.
    // the update thread will also lock this mutex when it is making changes to the KnightsGameImpl or GameConnection structures.
    boost::mutex my_mutex;

    boost::thread update_thread;
    volatile bool update_thread_wants_to_exit;

    KnightsLog *knights_log;
    std::string game_name;

    std::vector<int> delete_observer_nums;
    std::vector<int> players_to_eliminate;
    std::vector<std::string> all_player_names;

    // used during game startup
    volatile bool startup_signal;
    std::string startup_err_msg;

    int msg_counter;  // used for logging when 'update' events occur.
    std::auto_ptr<std::deque<int> > update_counts;   // used for playback.
    std::auto_ptr<std::deque<int> > time_deltas;     // ditto
    std::auto_ptr<std::deque<unsigned int> > random_seeds; // ditto
    bool msg_count_update_flag;

    // These are set during initialization (and are only valid when game is running!)
    bool is_team_game;   // used to decide whether to parse the "/t" team chat signal.
    bool is_deathmatch;  // used when sending "start_game" message
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
                boost::this_thread::sleep(boost::posix_time::milliseconds(50));
            }
        }
    }
    
    void Announcement(KnightsGameImpl &kg, const std::string &msg)
    {
        for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
            Coercri::OutputByteBuf buf((*it)->output_data);
            buf.writeUbyte(SERVER_ANNOUNCEMENT);
            buf.writeString(msg);
        }
    }
    
    int GetSelection(const MenuSelections &msel, const std::string &key)
    {
        std::map<std::string, MenuSelections::Sel>::const_iterator it = msel.selections.find(key);
        if (it == msel.selections.end()) return -1;
        else return it->second.value;
    }
    
    std::string GenerateQuestDescription(const MenuSelections &msel, const KnightsConfig &knights_config)
    {
        const int quest = GetSelection(msel, "quest");
        const int exit = GetSelection(msel, "exit");
        std::string exit_string;

        switch (exit) {
        case 1:
            exit_string = "your entry point";
            break;
        case 2:
            exit_string = "your opponent's entry point";
            break;
        case 4:
            exit_string = "the guarded exit";
            break;
        default:
            exit_string = "an unknown exit point";
            break;
        }

        std::string result;

        const int mission = GetSelection(msel, "mission");

        if (mission == 5) {
            // Team Duel to the Death
            result = "Team Duel to the Death\n\n"
                "In this quest players divide into two or more teams. Set your Knight House Colours to "
                "decide which team you are playing for.\n\n"
                "The objective is to secure all of the entry points using the Wand of Securing, then kill all of "
                "the other teams' knights to win the game.";
        } else if (mission == 6) {
            // Deathmatch
            result = "Deathmatch\n\n"
                "Players get 1 frag for killing an enemy knight, and -1 for a suicide. Being killed by a monster doesn't affect your frags total. "
                "The player with the most frags when time runs out is the winner.";
        } else if (quest > 0) {
            result = knights_config.getQuestDescription(quest, exit_string);
        } else if (quest == 0) {
            // Custom quest
            std::ostringstream str;

            str << "Custom Quest\n\n";

            const int num_gems = GetSelection(msel, "num_gems");
            const int gems_needed = GetSelection(msel, "gems_needed");
            const int book_type = GetSelection(msel, "book");
            const int wand_type = GetSelection(msel, "wand");

            if (mission == 0) {
                str << "You must secure all entry points to prevent your opponent entering the dungeon";
            } else if (mission == 4) {
                str << "You must strike the book with the wand in the special pentagram";
            } else {
                if (mission == 1) {
                    str << "Your mission is to ";
                } else {
                    str << "You must retrieve the ";
                    if (mission == 2) str << "book ";
                    else str << "wand ";
                    str << "and ";
                }
                str << "escape via " << exit_string;
            }
            if (gems_needed > 0) {
                str << " with ";
                str << char('0' + gems_needed);
                str << " out of ";
                str << char('0' + num_gems);
                str << " gems";
            }
            str << ".";

            switch (book_type) {
            case 1:
                str << "\n\nThe Book of Knowledge reveals knowledge of the dungeon.";
                break;
            case 2:
                str << "\n\nThe Lost Book of Ashur is somewhere in the dungeon.";
                break;
            case 3:
                str << "\n\nThe Necronomicon is sealed behind locked doors. It has powers to raise the undead.";
                break;
            case 4:
                str << "\n\nThe Tome of Gnomes is hidden behind unknown traps and riddles.";
                break;
            }

            switch (wand_type) {
            case 1:
                str << "\n\nThe Wand of Destruction terminates targets.";
                break;
            case 2:
                str << "\n\nThe Wand of Open Ways may be used to open any item.";
                break;
            case 3:
                str << "\n\nThe Wand of Securing is used to secure entry points.";
                break;
            case 4:
                str << "\n\nThe Wand of Undeath controls and slays the undead.";
                break;
            }
            
            result = str.str();
        }

        const int time_limit = GetSelection(msel, "#time"); // in mins, or 0 for none
        if (time_limit > 0) {
            std::ostringstream str;
            str << "\n\n";
            if (mission == 6) {
                str << "The game will last for ";
            } else {
                str << "You must complete this quest within ";
            }
            str << time_limit
                << " minute"
                << (time_limit > 1 ? "s" : "")
                << ".";
            result += str.str();
        }

        return result;
    }

    bool IsTeam(const MenuSelections &msel)
    {
        return GetSelection(msel, "mission") == 5;
    }

    bool IsDeathmatch(const MenuSelections &msel)
    {
        return GetSelection(msel, "mission") == 6;
    }

    // returns true if any difference between msel & msel_old was detected
    // (i.e. if anything was sent).
    bool SendMenuSelections(Coercri::OutputByteBuf &buf, const MenuSelections &msel_old, const MenuSelections &msel_new,
                            bool * need_hse_col_update)
    {
        bool retval = false;
        for (std::map<std::string, MenuSelections::Sel>::const_iterator it = msel_new.selections.begin(); 
        it != msel_new.selections.end(); ++it) {
            std::map<std::string, MenuSelections::Sel>::const_iterator it_old 
                = msel_old.selections.find(it->first);
            const bool needs_update = (it_old == msel_old.selections.end() 
                                     || it_old->second.value != it->second.value
                                     || it_old->second.allowed_values != it->second.allowed_values);
            if (needs_update) {
                retval = true;
                buf.writeUbyte(SERVER_SET_MENU_SELECTION);
                buf.writeString(it->first);
                buf.writeVarInt(it->second.value);
                buf.writeVarInt(it->second.allowed_values.size());
                for (std::vector<int>::const_iterator av = it->second.allowed_values.begin(); 
                av != it->second.allowed_values.end(); ++av) {
                    buf.writeVarInt(*av);
                }
            }
        }

        const bool old_is_team = IsTeam(msel_old);
        const bool new_is_team = IsTeam(msel_new);
        if (need_hse_col_update) *need_hse_col_update = old_is_team != new_is_team;
        
        return retval;
    }

    void SendQuestDescription(Coercri::OutputByteBuf &buf, const std::string &descr_old, const std::string &descr_new)
    {
        if (descr_new != descr_old) {
            buf.writeUbyte(SERVER_SET_QUEST_DESCRIPTION);
            buf.writeString(descr_new);
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
                buf.writeString((*it)->name);
                buf.writeUbyte((*it)->is_ready ? 1 : 0);
                buf.writeUbyte((*it)->house_colour);
                if (!(*it)->name2.empty()) {
                    buf.writeString((*it)->name2);
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
                buf.writeString((*it)->name);
            }
        }
        
        // Send the current menu selections
        SendMenuSelections(buf, MenuSelections(), impl.menu_selections, 0);
        SendQuestDescription(buf, "", impl.quest_description);

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

    void DoSetHouseColour(KnightsGameImpl &impl, GameConnection &conn, int hse_col)
    {
        conn.house_colour = hse_col;

        // send notification to players
        for (game_conn_vector::iterator it = impl.connections.begin(); it != impl.connections.end(); ++it) {
            Coercri::OutputByteBuf out((*it)->output_data);
            out.writeUbyte(SERVER_SET_HOUSE_COLOUR);
            // note we don't support house colours in split screen mode at the moment. we assume it's the first player
            // on the connection who is having the house colours set.
            out.writeString(conn.name);
            out.writeUbyte(hse_col);
        }            
    }

    // validate the menu selections, also send updates to clients.
    // called by SetMenuSelection and by Random Quests code.
    // returns true if something was changed.
    bool ValidateMenuSelections(KnightsGameImpl &impl, 
                                const MenuSelections &msel_old, 
                                const std::string &quest_descr_old)
    {
        // Update the quest description
        impl.quest_description = GenerateQuestDescription(impl.menu_selections, *impl.knights_config);
        
        // Broadcast new menu selections to all clients. (Only if something changed though.)
        // Also deactivate 'ready' flags if something changed.
        bool updated = false;
        for (game_conn_vector::iterator it = impl.connections.begin(); it != impl.connections.end(); ++it) {
            Coercri::OutputByteBuf buf((*it)->output_data);

            bool need_hse_col_update;
            updated = SendMenuSelections(buf, msel_old, impl.menu_selections, &need_hse_col_update);

            if (need_hse_col_update) {
                const int n_hse_col = GetNumAvailHouseCols(impl);
                if ((*it)->house_colour >= n_hse_col && !(*it)->obs_flag) {
                    // re-set his house colour.
                    DoSetHouseColour(impl, **it, n_hse_col - 1);
                }
                // re-send him the list of available house colours.
                SendAvailableHouseColours(impl, buf);
            }
                        
            if (!updated) break;
            SendQuestDescription(buf, quest_descr_old, impl.quest_description);
            (*it)->is_ready = false;
        }
        
        return updated;
    }
    
    // returns true if something was changed.
    bool SetMenuSelection(KnightsGameImpl &impl, const std::string &key, int value)
    {
        // If it's an invalid key then ignore it
        bool do_update = true;
        if (impl.menu_selections.selections.find(key) == impl.menu_selections.selections.end()) do_update = false;
        
        // Save the old settings
        MenuSelections msel_old = impl.menu_selections;
        std::string quest_descr_old = impl.quest_description;

        if (do_update) {
            // If they've changed a non-quest item then change the quest to CUSTOM
            if (key != "quest" && impl.menu_selections.getValue(key) != value) {
                impl.menu_selections.setValue("quest", 0);
            }
            
            // Set the new setting
            impl.menu_selections.setValue(key, value);
        }

        // Count the number of players, this is needed for the constraints
        const int nplayers = CountPlayers(impl);
        
        // Apply menu constraints
        // NOTE: We allow quests to be set as if (at least) min_players were present,
        // this is so you can setup a 2-player quest on the server when only the first player has arrived yet.
        // (If you try to start a 2-player quest with only 1 player present you will get an error msg later on.)
        const int min_players_for_constraint = impl.allow_split_screen ? 1 : 2;
        impl.knights_config->getMenuConstraints().apply(impl.knights_config->getMenu(), impl.menu_selections,
                                                        std::max(min_players_for_constraint, nplayers));

        return ValidateMenuSelections(impl, msel_old, quest_descr_old);
    }

    void ResetMenuSelections(KnightsGameImpl &impl)
    {
        // set all menu selections to zero
        for (int i = 0; i < impl.knights_config->getMenu().getNumItems(); ++i) {
            const std::string & key = impl.knights_config->getMenu().getItem(i).getKey();
            impl.menu_selections.setValue(key, 0);
        }

        // now explicitly set "quest" to 1, this will also ensure that menu constraints get set correctly.
        SetMenuSelection(impl, "quest", 1);

        // Update the quest description also
        impl.quest_description = GenerateQuestDescription(impl.menu_selections, *impl.knights_config);
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
            (*it)->player_num = 0;
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
                std::vector<int> hse_cols;
                std::vector<std::string> player_names;
                for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                    if (!(*it)->obs_flag) {
                        hse_cols.push_back((*it)->house_colour);
                        player_names.push_back((*it)->name);
                        if (!(*it)->name2.empty()) {
                            hse_cols.push_back((*it)->house_colour + 1);
                            player_names.push_back((*it)->name2);
                        }
                    }
                }

                kg.all_player_names = player_names;
                
                if (player_names.size() == 2 && kg.connections.size() == 1) {
                    // Split screen game.
                    // Disable the player names in this case.
                    for (size_t i = 0; i < player_names.size(); ++i) player_names[i].clear();
                }

                nplayers = player_names.size();
                
                std::string warning_msg;
                try {
                    engine.reset(new KnightsEngine(kg.knights_config, kg.menu_selections, hse_cols, player_names,
                                                   kg.tutorial_mode, kg.is_deathmatch, warning_msg));
                } catch (const std::exception &e) {
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

                // Send the warning message (from the dungeon generator) to all players and observers (Trac #47)
                // Also send the team chat notification (if applicable)
                for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                    Coercri::OutputByteBuf buf((*it)->output_data);

                    if (!warning_msg.empty()) {
                        buf.writeUbyte(SERVER_ANNOUNCEMENT);
                        buf.writeString(warning_msg);
                    }

                    if (kg.is_team_game && !(*it)->obs_flag) {
                        buf.writeUbyte(SERVER_ANNOUNCEMENT);
                        buf.writeString("Note: Team chat is available. Type /t at the start of your message to send to your team only.");
                    }
                }
                
                // Go into game loop. NOTE: This will run forever until the main thread interrupts us
                // (or an exception occurs, or update() returns false).
                int last_time = timer->getMsec();
                while (1) {
                    // work out how long since the last update
                    const int new_time = timer->getMsec();
                    const int time_delta = new_time - last_time;

                    // only run an update if delta is min_time_delta or more.
                    // if > max_time_delta then only run an update of max_time_delta, and 'drop'
                    // the rest of the time (ie run more slowly than real-time).
                    const int min_time_delta = 50;
                    const int max_time_delta = 200;
                    if (time_delta > min_time_delta) {                        
                        const bool should_continue = update(std::min(time_delta, max_time_delta));
                        if (!should_continue) break;
                        last_time += time_delta;
                    }

                    // work out how long until the next update (taking
                    // into account how long the update itself took).
                    const int time_since_update = timer->getMsec() - last_time;
                    const int time_to_wait = std::max(0, min_time_delta - time_since_update) + 1;
                    boost::this_thread::sleep(boost::posix_time::milliseconds(time_to_wait));
                }
                
            } catch (boost::thread_interrupted &) {
                // Allow this to go through. The code that interrupted us knows what it's doing...
                
            } catch (std::exception &e) {
                sendError(e.what());
            } catch (...) {
                sendError("Unknown error in update thread");
            }

            // Before we go, delete the KnightsEngine. This will make sure that the Mediator
            // is still around while the KnightsEngine destructor runs.
            try {
                engine.reset();
            } catch (...) {
                sendError("Error during KnightsEngine shutdown");
            }

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
                            for (std::vector<std::string>::const_iterator it = kg.all_player_names.begin(); it != kg.all_player_names.end(); ++it) {
                                buf.writeString(*it);
                            }

                            // Set his player_num and obs_num to zero, and his obs_flag to true.
                            (*c)->player_num = 0;
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
                                engine->catchUp(p, 
                                                callbacks->getDungeonView(p),
                                                callbacks->getMiniMap(p),
                                                callbacks->getStatusDisplay(p));
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

                // associate a ping time with each player
                // -- Only do this if the timer ran out, not if we are updating following a death,
                // otherwise it looks strange when the pings update just because someone died.
                if (time_to_player_list_update <= 0) {
                    for (size_t idx = 0; idx < player_list.size(); ++idx) {
                        const std::string &player_name = player_list[idx].name;
                        for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                            if ((*it)->name == player_name) {
                                pings[player_name] = (*it)->ping_time;
                            }
                        }
                    }
                }

                // now add observers to the list (Trac #26)
                for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                    if ((*it)->observer_num > 0) {

                        bool found = false;
                        for (size_t idx = 0; idx < player_list.size(); ++idx) {
                            if (player_list[idx].name == (*it)->name) {
                                // found an eliminated player corresponding to this observer.
                                // We clear the eliminated flag at this point. (The eliminated flag is not sent to
                                // the client, so it is safe to do this.)
                                player_list[idx].eliminated = false;
                                player_list[idx].name += " (Eliminated)";
                                pings[player_list[idx].name] = (*it)->ping_time;
                                found = true;
                                break;
                            }
                        }

                        if (!found) {
                            // This observer is not one of the eliminated players so add him to the list.
                            PlayerInfo pi;
                            pi.name = (*it)->name + " (Observer)";
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
                }

                // Now remove any eliminated players. (The ones who are still observing are not
                // removed, because their eliminated flag was cleared above.)
                for (size_t idx = 0; idx < player_list.size(); /* incremented below */) {
                    if (player_list[idx].eliminated) {
                        player_list.erase(player_list.begin() + idx);
                    } else {
                        ++idx;
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
                        buf.writeString(player_list[idx].name);
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
                std::vector<std::string> loser_names;
                std::string winner_name;
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
                    msg += "winner=" + winner_name;
                    for (std::vector<string>::const_iterator it = loser_names.begin(); it != loser_names.end(); ++it) {
                        msg += std::string(", loser=") + *it;
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
        std::map<std::string, int> pings;
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
            out.writeString(conn.name);
            out.writeUbyte(ready ? 1 : 0);
            if (!conn.name2.empty()) {
                out.writeUbyte(SERVER_SET_READY);
                out.writeString(conn.name2);
                out.writeUbyte(ready ? 1 : 0);
            }
        }
    }
    
    struct CmpByName {
        bool operator()(const boost::shared_ptr<GameConnection> &lhs,
                        const boost::shared_ptr<GameConnection> &rhs) const
        {
            return ToUpper(lhs->name) < ToUpper(rhs->name);
        }
        static std::string ToUpper(const std::string &input)
        {
            std::string result;
            for (std::string::const_iterator it = input.begin(); it != input.end(); ++it) {
                result += std::toupper(*it);
            }
            return result;
        }
    };

    void StartGameIfReady(KnightsGameImpl &kg)
    {
        // mutex is UNLOCKED at this point

        if (kg.update_thread.joinable()) return;  // Game is already started.

        // game thread definitely not running at this point... so it is safe to access kg
        
        std::vector<std::string> names;
        
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
        const int min_players_required = kg.knights_config->getMenuConstraints().getMinPlayers(kg.menu_selections);
        if (ready_to_start && nplayers < min_players_required) {
            std::ostringstream str;
            str << "This quest requires at least " << min_players_required << " players.";
            Announcement(kg, str.str());
            ready_to_start = false;
        }

        // Don't start if it's a team game and all players are on the same team
        kg.is_team_game = IsTeam(kg.menu_selections);
        kg.is_deathmatch = IsDeathmatch(kg.menu_selections);
        if (ready_to_start && kg.is_team_game) {
            int team_found = -1;
            bool two_teams_found = false;
            for (game_conn_vector::const_iterator it = kg.connections.begin(); it != kg.connections.end(); ++it) {
                if (!(*it)->obs_flag) {
                    if (!(*it)->name2.empty()) {
                        two_teams_found = true;
                        break;
                    } else if (team_found == -1) {
                        team_found = (*it)->house_colour;
                    } else if ((*it)->house_colour != team_found) {
                        two_teams_found = true;
                        break;
                    }
                }
            }
            if (!two_teams_found) {
                Announcement(kg, "Cannot start game if all players are on the same team!");
                ready_to_start = false;
            }
        }

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
                Announcement(kg, std::string("Couldn't start game! ") + err_msg);

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
                        buf.writeUbyte(kg.is_deathmatch);
                    } else {
                        // Observer.
                        Coercri::OutputByteBuf buf((*it)->output_data);
                        buf.writeUbyte(SERVER_START_GAME_OBS);
                        buf.writeUbyte(num_displays);
                        buf.writeUbyte(kg.is_deathmatch);
                        for (int i = 0; i < num_displays; ++i) {
                            buf.writeString(names[i]);
                        }
                        buf.writeUbyte(0);  // already_started flag (false)
                    }
                }
            
                // Log a message if required.
                if (kg.knights_log) {
                    std::ostringstream str;
                    str << kg.game_name << "\tgame started\t";
                    for (std::vector<std::string>::const_iterator it = names.begin(); it != names.end(); ++it) {
                        if (it != names.begin()) str << ", ";
                        str << *it;
                    }
                    
                    for (std::map<std::string, MenuSelections::Sel>::const_iterator it = kg.menu_selections.selections.begin();
                    it != kg.menu_selections.selections.end(); ++it) {
                        // TODO: could try to convert this to a summary message rather than dumping all settings?
                        str << ", " << it->first << "=" << it->second.value;
                    }
                    
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
}

KnightsGame::KnightsGame(boost::shared_ptr<KnightsConfig> config,
                         boost::shared_ptr<Coercri::Timer> tmr,
                         bool allow_split_screen,
                         bool tutorial_mode,
                         KnightsLog *knights_log,
                         const std::string &game_name,
                         std::auto_ptr<std::deque<int> > update_counts,
                         std::auto_ptr<std::deque<int> > time_deltas,
                         std::auto_ptr<std::deque<unsigned int> > random_seeds)
    : pimpl(new KnightsGameImpl)
{
    pimpl->knights_config = config;
    pimpl->timer = tmr;
    pimpl->allow_split_screen = allow_split_screen;
    pimpl->tutorial_mode = tutorial_mode;
    pimpl->game_over = false;
    pimpl->pause_mode = false;
    pimpl->update_thread_wants_to_exit = false;

    // set up our own controls vector.
    pimpl->controls.clear();
    config->getStandardControls(pimpl->controls);
    std::vector<const UserControl*> other_ctrls;
    config->getOtherControls(other_ctrls);
    pimpl->controls.insert(pimpl->controls.end(), other_ctrls.begin(), other_ctrls.end());
    
    // initialize all menu settings to 0
    ResetMenuSelections(*pimpl);

    pimpl->knights_log = knights_log;
    pimpl->game_name = game_name;

    pimpl->msg_counter = 0;
    if (update_counts.get()) pimpl->update_counts = update_counts;
    if (time_deltas.get()) pimpl->time_deltas = time_deltas;
    if (random_seeds.get()) pimpl->random_seeds = random_seeds;
    pimpl->msg_count_update_flag = true;
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

GameConnection & KnightsGame::newClientConnection(const std::string &client_name, const std::string &client_name_2, 
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
            out.writeString(client_name);
            out.writeUbyte(new_obs_flag);
            out.writeUbyte(conn->house_colour);
            if (!client_name_2.empty()) {
                out.writeUbyte(SERVER_PLAYER_JOINED_THIS_GAME);
                out.writeString(client_name_2);
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
        buf.writeUbyte(pimpl->is_deathmatch);
        for (std::vector<std::string>::const_iterator it = pimpl->all_player_names.begin(); it != pimpl->all_player_names.end(); ++it) {
            buf.writeString(*it);
        }
        buf.writeUbyte(1);  // already_started flag (true)
    }

    if (!new_obs_flag) {
        // May need to update menu settings, since no of players has changed
        SetMenuSelection(*pimpl, "", 0);
    }
    
    return *conn;
}

void KnightsGame::clientLeftGame(GameConnection &conn)
{
    // This is called when a player has left the game (either because
    // KnightsServer sent him a SERVER_LEAVE_GAME msg, or because he
    // disconnected.)

    bool is_player = false;
    
    std::string name, name2;
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
            buf.writeString(name);
            buf.writeUbyte(is_player ? 0 : 1);
            if (!name2.empty()) {
                buf.writeUbyte(SERVER_PLAYER_LEFT_THIS_GAME);
                buf.writeString(name2);
                buf.writeUbyte(is_player ? 0 : 1);
            }
        }
        
        if (pimpl->connections.empty()) {
            // If there are no connections left then reset the menu selections.
            ResetMenuSelections(*pimpl);
        } else if (is_player) {
            // Number of players has changed, may need to update menu constraints.
            SetMenuSelection(*pimpl, "", 0);
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
    if (!pimpl->update_thread.joinable() || !pimpl->is_team_game || conn.obs_flag) {
        // Team chat not available (either because we are on the quest selection menu, 
        // or this is not a team game, or because sender is an observer).
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
        buf.writeString(conn.name);

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
        DoSetHouseColour(*pimpl, conn, hse_col);
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
            out.writeString(conn.name);
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
            pimpl->knights_log->logMessage(pimpl->game_name + "\tquit requested\t" + conn.name);
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
                Announcement(*pimpl, conn.name + " quit the game.");
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

void KnightsGame::setMenuSelection(GameConnection &conn, const std::string &key, int value)
{
    if (pimpl->update_thread.joinable()) return;  // Game is running

    WaitForMsgCounter(*pimpl);
    
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    if (pimpl->msg_count_update_flag) pimpl->msg_counter++;
    if (conn.obs_flag) return;   // only players can adjust the menu.

    const bool changed = SetMenuSelection(*pimpl, key, value);

    if (changed) {
        // send announcement to all players.
        int x = 0;
        for (; x < pimpl->knights_config->getMenu().getNumItems(); ++x) {
            if (pimpl->knights_config->getMenu().getItem(x).getKey() == key) break;
        }
        std::string str;
        if (x >= 0 && x < pimpl->knights_config->getMenu().getNumItems()) {
            const MenuItem &item = pimpl->knights_config->getMenu().getItem(x);
            str = conn.name + " set \"" + item.getTitleString() + '"';
            std::map<std::string, MenuSelections::Sel>::const_iterator it = pimpl->menu_selections.selections.find(key);
            if (it != pimpl->menu_selections.selections.end()) {
                const std::string val_str = item.getValueString(it->second.value);
                
                bool all_digit = true;
                for (std::string::const_iterator v = val_str.begin(); v != val_str.end(); ++v) {
                    if (!std::isdigit(*v)) {
                        all_digit = false;
                        break;
                    }
                }

                str += " to ";
                if (!all_digit) str += '"';
                str += item.getValueString(it->second.value);
                if (!all_digit) str += '"';
                str += '.';
            }
        }
        Announcement(*pimpl, str);
    }
}

namespace {
    struct GtrThan {
        GtrThan(int x_) : x(x_) {} 
        bool operator()(int y) const { return y > x; }
        int x;
    };
}

void KnightsGame::randomQuest(GameConnection &conn)
{
    if (pimpl->update_thread.joinable()) return;  // Game is running

    WaitForMsgCounter(*pimpl);

    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    if (pimpl->msg_count_update_flag) pimpl->msg_counter++;
    if (conn.obs_flag) return;   // only players can set quests.

    // Save the old settings
    MenuSelections msel_old = pimpl->menu_selections;
    std::string quest_descr_old = pimpl->quest_description;

    // Make sure "quest" is set to "custom"
    SetMenuSelection(*pimpl, "quest", 0);

    // Calculate number of players for constraint purposes (do this once up front)
    // NOTE: We generate a quest appropriate for the current number of players;
    // i.e. if only one player is online, we do not create the two-player quests.
    const int nplayers = CountPlayers(*pimpl);

    // Build up a list of keys that we are going to randomize.
    // (Can do this once at the beginning)
    const Menu & menu = pimpl->knights_config->getMenu();
    std::vector<const std::string *> keys;
    keys.reserve(menu.getNumItems());
    for (int i = 0; i < menu.getNumItems(); ++i) {
        const std::string & key = menu.getItem(i).getKey();
        
        // special case: "quest" should not be randomized because that would
        // undo all our work randomizing the settings.
        // also: "#time" is not randomized (currently) since we don't really know what a
        // reasonable time limit would be for various quests.
        if (key != "quest" && key != "#time") {
            keys.push_back(&key);
        }
    }
    
    RNG_Wrapper myrng(g_rng);
    std::vector<int> allowed_values;
    
    // Iterate a number of times, to make sure we get a good randomization
    for (int iterations = 0; iterations < 3; ++iterations) {

        // Shuffle the menu keys into a random order
        // Note: we are using the global rng (from the main thread), as opposed to
        // the rng's from the game threads. This should mean that the replay feature
        // is not messed up by the extra random numbers being generated here.
        std::random_shuffle(keys.begin(), keys.end(), myrng);

        // for each key in the random ordering:
        for (std::vector<const std::string *>::const_iterator key_it = keys.begin(); key_it != keys.end(); ++key_it) {

            // find out the allowed values
            allowed_values = pimpl->menu_selections.getAllowedValues(**key_it);

            // special case: "num_wands" may not be bigger than the current number of players plus two
            // This is to prevent silly 8-wand quests when there are only 2 players present (for example)...
            // NOTE: Don't really want special cases like this here. (Ideally "random quest" would generate exactly
            // the same set of quests that you can enter manually.)
            if (**key_it == "num_wands") {
                allowed_values.erase(std::remove_if(allowed_values.begin(), allowed_values.end(), GtrThan(nplayers+2)),
                                     allowed_values.end());
            }

            // another special case: don't generate "team duel to the death" unless there are at least four players
            if (**key_it == "mission" && nplayers < 4) {
                allowed_values.erase(std::remove(allowed_values.begin(), allowed_values.end(), 5),
                                     allowed_values.end());
            }

            if (!allowed_values.empty()) {
                // pick one at random
                const int selected_value = allowed_values[g_rng.getInt(0, allowed_values.size())];

                // set it to that value, updating constraints as required.
                pimpl->menu_selections.setValue(**key_it, selected_value);
                pimpl->knights_config->getMenuConstraints().apply(menu,
                                                                  pimpl->menu_selections,
                                                                  nplayers);
            }
        }
    }

    // ensure the results are valid; send updates to clients
    ValidateMenuSelections(*pimpl, msel_old, quest_descr_old);
   
    if (pimpl->knights_log) {
        // Save the quest in the binary log (This is so that replays work correctly. It is no good just having 
        // "random quest" in the log, we need to save what the quest was actually set to.)
        std::ostringstream random_quest_str;
        random_quest_str << pimpl->game_name << '\0';
        for (std::map<std::string, MenuSelections::Sel>::const_iterator it = pimpl->menu_selections.selections.begin();
        it != pimpl->menu_selections.selections.end(); ++it) {
            random_quest_str << it->first << '\0';
            random_quest_str << it->second.value << '\0';
        }
        const std::string & s = random_quest_str.str();
        pimpl->knights_log->logBinary("QST", 0, s.length(), s.c_str());
    }

    // Send announcement to all players
    Announcement(*pimpl, conn.name + " selected a Random Quest.");
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
    const std::string & my_name = conn.name;

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
        buf.writeString(my_name);
        buf.writeUbyte(new_obs_flag ? 1 : 0);
        
        if (!new_obs_flag) {
            buf.writeUbyte(SERVER_SET_HOUSE_COLOUR);
            buf.writeString(my_name);
            buf.writeUbyte(new_col);
        }
    }

    // May need to re-apply max players constraints
    SetMenuSelection(*pimpl, "", 0);
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

void KnightsGame::internalSetMenuSelection(const std::string &key, int value)
{
    if (pimpl->update_thread.joinable()) return; // Game is running
    boost::lock_guard<boost::mutex> lock(pimpl->my_mutex);
    SetMenuSelection(*pimpl, key, value);
}

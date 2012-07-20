/*
 * options.cpp
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

#include "config_map.hpp"
#include "options.hpp"

#include <cctype>
#include <cstring>

Options::Options()
{
    first_time = true;
    
    ctrls[0][0] = Coercri::RK_W;
    ctrls[0][1] = Coercri::RK_S;
    ctrls[0][2] = Coercri::RK_A;
    ctrls[0][3] = Coercri::RK_D;
    ctrls[0][4] = Coercri::RK_Q;
    ctrls[0][5] = Coercri::RK_F1;

    ctrls[1][0] = Coercri::RK_UP;
    ctrls[1][1] = Coercri::RK_DOWN;
    ctrls[1][2] = Coercri::RK_LEFT;
    ctrls[1][3] = Coercri::RK_RIGHT;
    ctrls[1][4] = Coercri::RK_RIGHT_CONTROL;
    ctrls[1][5] = Coercri::RK_F12;

    ctrls[2][0] = Coercri::RK_W;
    ctrls[2][1] = Coercri::RK_S;
    ctrls[2][2] = Coercri::RK_A;
    ctrls[2][3] = Coercri::RK_D;
    ctrls[2][4] = Coercri::RK_Q;
    ctrls[2][5] = Coercri::RK_F1;

    // New control system is the default for new players.
    new_control_system = true;
    action_bar_tool_tips = true;
    
    use_scale2x = true;
    fullscreen = false;  // Default to windowed mode (#169)

    allow_non_integer_scaling = false;
    window_width = 1000;
    window_height = 780;

    global_chat_key = Coercri::RK_TAB;
    team_chat_key = Coercri::RK_BACKQUOTE;
}

Options LoadOptions(std::istream &str)
{
    Options o;
    o.first_time = false;

    char buf[8] = {0};
    str.getline(buf, 6);

    if (buf[0] != 'K'
        || buf[1] != 'O'
        || buf[2] != 'P'
        || buf[3] != 'T'
        || !std::isdigit(buf[4])
        || buf[5] != '\0') return Options();

    const int version = buf[4] - '0';

    if (version < 1 || version > 5) return Options();
    
    bool load_ok = false;

    // All Versions
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 6; ++j) {
            if (i<2 || version >= 3) {
                // Read from file.
                int x;
                str >> x;
                o.ctrls[i][j] = Coercri::RawKey(x);
            } else {
                // Versions prior to 3 only have 2 sets of controls in the file.
                // Copy the "network games" controls from the P2 controls.
                o.ctrls[i][j] = o.ctrls[1][j];
            }
        }
    }
    str >> o.use_scale2x;
    str >> o.fullscreen;

    // Version 2+
    if (version >= 2) {
        str >> o.allow_non_integer_scaling;
        str >> o.window_width >> o.window_height;
    }

    // Version 4+
    if (version >= 4) {
        str >> o.new_control_system;
        str >> o.action_bar_tool_tips;
    } else {
        // If they have played Knights before then put them on the old control system.
        // They can set up the new controls through the options screen if they wish.
        o.new_control_system = false;
        o.action_bar_tool_tips = true;
    }

    // Version 5+
    if (version >= 5) {
        int x;
        str >> x;
        o.global_chat_key = Coercri::RawKey(x);

        str >> x;
        o.team_chat_key = Coercri::RawKey(x);
    }
    // (for versions < 5, Options::Options() will have set up default
    // chat keys, which are fine for us.)
    
    
    // Version 3+ -- Player Name
    // (Leave this to the end since we want to ignore loading errors)
    if (version >= 3) {
        // Ignore any errors on loading the player name
        // (in particular we want to ignore any EOF error if the player name is blank).
        if (str) load_ok = true;

        // eat white space
        char c;
        while (str.get(c)) {
            if (!std::isspace(c)) {
                str.putback(c);
                break;
            }
        }
        
        std::getline(str, o.player_name);
    }

    if (!str && !load_ok) return Options();  // In case of load failure, return default options
    else return o;
}

void SaveOptions(const Options &o, std::ostream &str)
{
    str << "KOPT5\n";
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 6; ++j) {
            str << o.ctrls[i][j] << " ";
        }
        str << "\n";
    }
    str << o.use_scale2x << "\n";
    str << o.fullscreen << "\n";
    str << o.allow_non_integer_scaling << "\n";
    str << o.window_width << " " << o.window_height << "\n";
    str << o.new_control_system << " " << o.action_bar_tool_tips << "\n";
    str << o.global_chat_key << " " << o.team_chat_key << "\n";
    str << o.player_name << "\n";
}

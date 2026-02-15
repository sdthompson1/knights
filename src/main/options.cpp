/*
 * options.cpp
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

#include "config_map.hpp"
#include "my_ctype.hpp"
#include "options.hpp"

#include <cstring>

Options::Options()
{
    first_time = true;
    
    ctrls[0][0] = Coercri::Scancode("w");
    ctrls[0][1] = Coercri::Scancode("s");
    ctrls[0][2] = Coercri::Scancode("a");
    ctrls[0][3] = Coercri::Scancode("d");
    ctrls[0][4] = Coercri::Scancode("q");
    ctrls[0][5] = Coercri::Scancode("f1");

    ctrls[1][0] = Coercri::Scancode("up");
    ctrls[1][1] = Coercri::Scancode("down");
    ctrls[1][2] = Coercri::Scancode("left");
    ctrls[1][3] = Coercri::Scancode("right");
    ctrls[1][4] = Coercri::Scancode("right_control");
    ctrls[1][5] = Coercri::Scancode("f12");

    ctrls[2][0] = Coercri::Scancode("w");
    ctrls[2][1] = Coercri::Scancode("s");
    ctrls[2][2] = Coercri::Scancode("a");
    ctrls[2][3] = Coercri::Scancode("d");
    ctrls[2][4] = Coercri::Scancode("q");
    ctrls[2][5] = Coercri::Scancode("f1");

    // New control system is the default for new players.
    new_control_system = true;
    action_bar_tool_tips = true;

    use_scale2x = true;
    fullscreen = false;  // Default to windowed mode (#169)

    allow_non_integer_scaling = false;
    window_width = 1000;
    window_height = 780;

    global_chat_key = Coercri::Scancode("tab");
    team_chat_key = Coercri::Scancode("backquote");

    maximized = true;
    allow_screen_flash = true;
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
        || !IsDigit(buf[4])
        || buf[5] != '\0') return Options();

    const int version = buf[4] - '0';

    if (version < 1 || version > 7) return Options();
    
    bool load_ok = false;

    // All Versions
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 6; ++j) {
            if (version < 7) {
                // Versions prior to 7 use integer keycodes which we
                // can no longer read. Just skip over them, leaving
                // keys at the default.
                // Also, versions prior to 3 had only two sets of
                // controls, not three.
                if (i<2 || version >= 3) {
                    int x;
                    str >> x;
                }
            } else {
                // Newer versions write the keys as strings.
                std::string x;
                str >> x;
                o.ctrls[i][j] = Coercri::Scancode(x);
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
        if (version >= 7) {
            std::string x;
            str >> x;
            o.global_chat_key = Coercri::Scancode(x);
            str >> x;
            o.team_chat_key = Coercri::Scancode(x);
        } else {
            // Skip numeric keycodes in earlier versions
            int x;
            str >> x;
            str >> x;
        }
    }
    // (for versions < 5, Options::Options() will have set up default
    // chat keys, which are fine for us.)

    // Version 6+
    if (version >= 6) {
        str >> o.maximized;
    }

    // Version 7+
    if (version >= 7) {
        str >> o.allow_screen_flash;
    }
    
    // Version 3+ -- Player Name
    // (Leave this to the end since we want to ignore loading errors)
    if (version >= 3) {
        // Ignore any errors on loading the player name
        // (in particular we want to ignore any EOF error if the player name is blank).
        if (str) load_ok = true;

        // eat white space
        char c;
        while (str.get(c)) {
            if (!IsSpace(c)) {
                str.putback(c);
                break;
            }
        }
        
        std::string player_name_utf8;
        std::getline(str, player_name_utf8);
        o.player_name = UTF8String::fromUTF8Safe(player_name_utf8);
    }

    if (!str && !load_ok) return Options();  // In case of load failure, return default options
    else return o;
}

void SaveOptions(const Options &o, std::ostream &str)
{
    str << "KOPT7\n";
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 6; ++j) {
            str << o.ctrls[i][j].getSymbolicName() << " ";
        }
        str << "\n";
    }
    str << o.use_scale2x << "\n";
    str << o.fullscreen << "\n";
    str << o.allow_non_integer_scaling << "\n";
    str << o.window_width << " " << o.window_height << "\n";
    str << o.new_control_system << " " << o.action_bar_tool_tips << "\n";
    str << o.global_chat_key.getSymbolicName() << " " << o.team_chat_key.getSymbolicName() << "\n";
    str << o.maximized << "\n";
    str << o.allow_screen_flash << "\n";
    str << o.player_name.asUTF8() << "\n";
}

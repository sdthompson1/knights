/*
 * options.hpp
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
 * Storage for game options
 * 
 */

#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include "gfx/key_code.hpp"  // from coercri

#include <iostream>
#include <string>

class ConfigMap;

class Options {
public:
    Options();  // Sets the defaults

    bool first_time;  // Set to true by ctor, false by a successful LoadOptions.
    
    // Controls for 3 players (P1/P2/Network games) and in order up/down/left/right/action/suicide.
    // 3rd player added in v011 of Knights.
    Coercri::RawKey ctrls[3][6];
    
    // Display settings
    bool use_scale2x;
    bool fullscreen;

    // added in version 2 of options file (version 009 of Knights)
    bool allow_non_integer_scaling;
    int window_width;
    int window_height;

    // added in version 3 of options file (version 011 of Knights)
    std::string player_name;

    // added in version 4 of options file (version 016 of Knights)
    bool new_control_system;   // whether to use action bar
    bool action_bar_tool_tips;

    // added in version 5 of options file (version 020 of Knights)
    Coercri::RawKey global_chat_key, team_chat_key;
};

Options LoadOptions(std::istream &);
void SaveOptions(const Options &, std::ostream &);

#endif

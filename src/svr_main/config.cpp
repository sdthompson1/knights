/*
 * config.cpp
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

#include "config.hpp"
#include "my_ctype.hpp"
#include "trim.hpp"

#include <fstream>
#include <sstream>

namespace {

    std::string RemoveComments(const std::string &x)
    {
        size_t i = x.find('#');
        if (i == std::string::npos) return x;
        else return x.substr(0, i);
    }

    std::string LowerCase(const std::string &x)
    {
        std::string y = x;
        for (std::string::iterator it = y.begin(); it != y.end(); ++it) {
            *it = ToLower(*it);
        }
        return y;
    }

    template<class T>
    T StrToT(int line, const std::string &x)
    {
        std::istringstream str(x);
        T result = 0;
        str >> result;
        if (!str) throw ConfigError(line, "Integer expected");
        return result;
    }

    int StrToInt(int line, const std::string &x)
    {
        return StrToT<int>(line, x);
    }

    unsigned int StrToUnsignedInt(int line, const std::string &x)
    {
        return StrToT<unsigned int>(line, x);
    }
    
    bool StrToBool(int line, const std::string &x)
    {
        const std::string y = LowerCase(x);
        if (y == "t" || y == "true"  || y == "y" || y == "yes" || y == "1") return true;
        if (y == "f" || y == "false" || y == "n" || y == "no"  || y == "0") return false;
        throw ConfigError(line, "Boolean value (true or false) expected");
    }

    void CheckExists(const std::string &motd_file, int line_counter)
    {
        // test the file to see if it exists.
        std::ifstream str(motd_file.c_str());
        char c;
        str >> c;
        if (!str && !str.eof()) throw ConfigError(line_counter, std::string("Problem reading MOTD file: ") + motd_file);
    }
}

Config::Config(std::istream &str)
{
    // Default settings.
    port = 16399;
    max_games = 9999999;    // effectively unlimited.
    max_players = 100;  // reasonable default. can't set this too high else enet_host_create will fail.
    broadcast = true;

    // Now load the config file.
    // Throws ConfigError if there is a problem.
    int line_counter = 0;
    while (1) {
        std::string line;
        std::getline(str, line);
        ++line_counter;
        if (str.eof()) return;   // Done.
        if (!str) throw ConfigError(line_counter, "Read error");

        line = RemoveComments(line);
        line = Trim(line);
        if (line.empty()) continue;

        const size_t equals_pos = line.find('=');
        if (equals_pos == std::string::npos) throw ConfigError(line_counter, "Syntax error");

        std::string key = line.substr(0, equals_pos);
        std::string value = (equals_pos == line.length() - 1) ? "" : line.substr(equals_pos+1);

        key = Trim(key);
        value = Trim(value);

        const std::string lkey = LowerCase(key);
        
        if (lkey == "port") {
            port = StrToInt(line_counter, value);
        } else if (lkey == "description") {
            description = value;
        } else if (lkey == "motdfile") {
            motd_file = value;
            CheckExists(motd_file, line_counter);
        } else if (lkey == "oldmotdfile") {
            old_motd_file = value;
            CheckExists(old_motd_file, line_counter);
        } else if (lkey == "maxplayers") {
            max_players = StrToInt(line_counter, value);
            if (max_players < 2) throw ConfigError(line_counter, "MaxPlayers must be at least 2");
        } else if (lkey == "maxgames") {
            max_games = StrToInt(line_counter, value);
            if (max_games < 1) throw ConfigError(line_counter, "MaxGames must be at least 1");
        } else if (lkey == "usebroadcast") {
            broadcast = StrToBool(line_counter, value);
        } else if (lkey == "knightsdatadir") {
            knights_data_dir = value;
        } else if (lkey == "logfile") {
            log_file = value;
        } else {
            throw ConfigError(line_counter, "Unknown setting: " + key);
        }
    }
}

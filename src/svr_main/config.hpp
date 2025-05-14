/*
 * config.hpp
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

/*
 * Holds configuration options for the server program.
 *
 */

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iosfwd>
#include <string>

class ConfigError {
public:
    ConfigError(int line_, const std::string &msg_) : line(line_), msg(msg_) { }
    int getLine() const { return line; }
    const std::string &getMessage() const { return msg; }
private:
    int line;
    std::string msg;
};
    
class Config {
public:
    // Load options from a config file
    explicit Config(std::istream &str);

    // Get the settings
    
    int getPort() const { return port; }
    const std::string & getDescription() const { return description; }
    std::string getMOTDFile() const { return motd_file; }
    std::string getOldMOTDFile() const { return old_motd_file.empty() ? motd_file : old_motd_file; }
    
    int getMaxPlayers() const { return max_players; }
    int getMaxGames() const { return max_games; }
    
    bool getUseMetaserver() const { return metaserver; }
    bool getReplyToBroadcast() const { return broadcast; }

    const std::string & getPassword() const { return password; }  // empty if no password.

    const std::string & getKnightsDataDir() const { return knights_data_dir; }  // empty means use default (DATA_DIR or knights_data)
    const std::string & getLogFile() const { return log_file; }  // empty means print log to stdout.

private:
    int port;
    std::string description, motd_file, old_motd_file;
    int max_players;   // must be > 0.
    int max_games;     // must be >= 0. 0 means unlimited.
    bool metaserver, broadcast;
    std::string password;
    std::string knights_data_dir;
    std::string log_file;
};
    
#endif

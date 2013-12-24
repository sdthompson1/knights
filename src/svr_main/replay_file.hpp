/*
 * replay_file.hpp
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

#ifndef REPLAY_FILE_HPP
#define REPLAY_FILE_HPP

#include <fstream>
#include <string>

// Class representing a replay file.

class ReplayFile {
public:
    ReplayFile(const std::string &filename, int timestamp_size_);

    // Returns msg=="EOF" if end of file found, otherwise reads one msg from the file.
    void readMessage(std::string &msg,
                     int &int_arg,
                     std::string &extra_bytes,
                     unsigned int &msec);

private:
    std::ifstream str;
    int timestamp_size;
};

#endif

/*
 * knights_log.hpp
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
 * Interface for receiving log messages from server.
 *
 */

#ifndef KNIGHTS_LOG_HPP
#define KNIGHTS_LOG_HPP

#include <string>

class KnightsLog {
public:
    virtual ~KnightsLog() { }

    // NOTE: logMessage may be called concurrently from different threads.
    virtual void logMessage(const std::string &msg) = 0;

    // binary log msgs. used for debugging.
    // note: msg_code must be a 3 byte string.
    virtual void logBinary(const char * msg_code, int conn_num, int num_extra_bytes, const char * extra_bytes) = 0;
};

#endif

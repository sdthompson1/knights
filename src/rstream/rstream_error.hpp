/*
 * rstream_error.hpp
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

#ifndef RSTREAM_ERROR_HPP
#define RSTREAM_ERROR_HPP

#include "boost/filesystem.hpp"

#include <exception>
#include <string>

// Exception class for RStreams.

// NOTE: This is not consistently used everywhere. Sometimes std or
// boost exceptions are thrown on errors. This might be considered an
// implementation bug.

class RStreamError : public std::exception {
public:
    RStreamError(const boost::filesystem::path &r, const std::string &e);
    virtual ~RStreamError() throw() { }
    const boost::filesystem::path & getResource() const { return resource; }
    const std::string &getErrorMsg() const { return error_msg; }
    const char *what() const throw();

private:
    boost::filesystem::path resource;
    std::string error_msg, what_str;
};

#endif

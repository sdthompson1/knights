/*
 * rstream_error.cpp
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

#include "rstream_error.hpp"

RStreamError::RStreamError(const boost::filesystem::path &r, const std::string &e)
    : resource(r), error_msg(e)
{
    what_str = "error loading '" + getResource().generic_string() +
        "': " + getErrorMsg();
}

const char *RStreamError::what() const throw()
{
    return what_str.c_str();
}

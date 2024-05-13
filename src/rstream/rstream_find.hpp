/*
 * rstream_find.hpp
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

#ifndef RSTREAM_FIND_HPP
#define RSTREAM_FIND_HPP

#include "boost/filesystem.hpp"

//
// Given a path "input_path" and a directory "cwd", this function
// returns either "cwd / input_path" if this exists, or just
// "input_path" otherwise. (In the former case, any redundant ".."
// path components are removed.)
//
// It is intended for the case where you want to look for a file
// either in some "current directory", or in the base directory.
//

boost::filesystem::path RStreamFind(const boost::filesystem::path &input_path,
                                    const boost::filesystem::path &cwd);

#endif


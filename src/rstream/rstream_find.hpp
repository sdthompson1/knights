/*
 * rstream_find.hpp
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

#ifndef RSTREAM_FIND_HPP
#define RSTREAM_FIND_HPP

//
// Given a path "input_path" and a directory "cwd", this function
// returns either "cwd / input_path" if this exists, or just
// "input_path" otherwise.
//
// Exception: if the input_path begins with a slash or backslash, then
// "cwd" is ignored and "input_path" is returned unmodified.
//
// This is intended for cases where you want to look for a file either
// in some "current directory", or in the base directory.
//

std::string RStreamFind(const std::string &input_path,
                        const std::string &cwd);

#endif


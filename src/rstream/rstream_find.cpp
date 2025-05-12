/*
 * rstream_find.cpp
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

#include "rstream.hpp"
#include "rstream_find.hpp"

std::string RStreamFind(const std::string &input_path,
                        const std::string &cwd)
{
    if (!input_path.empty() && (input_path[0] == '/' || input_path[0] == '\\')) {
        // "Absolute" resource path; do not use cwd
        return input_path;

    } else {
        // Try the cwd first, then fall back to just input_path
        std::string proposed = cwd + "/" + input_path;
        if (RStream::Exists(proposed)) {
            return proposed;
        } else {
            return input_path;
        }
    }
}

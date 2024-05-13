/*
 * rstream_find.cpp
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

#include "rstream.hpp"
#include "rstream_find.hpp"

namespace {
    boost::filesystem::path RemoveDotDot(const boost::filesystem::path &p)
    {
        // This is like a cut-down canonical(), that does not chase symlinks.
        boost::filesystem::path result;
        for (boost::filesystem::path::iterator it = p.begin(); it != p.end(); ++it) {
            if (*it == ".") continue;
            if (*it == "..") {
                result.remove_filename();
                continue;
            }
            result /= *it;
        }
        return result;
    }
}

boost::filesystem::path RStreamFind(const boost::filesystem::path &input_path,
                                    const boost::filesystem::path &cwd)
{
    if (input_path.has_root_path()) {
        // This shouldn't really happen for rstream paths. Just hand it back to the caller.
        return input_path;

    } else {
        boost::filesystem::path proposed = RemoveDotDot(cwd / input_path);
        if (RStream::Exists(proposed)) {
            return proposed;
        } else {
            return input_path;
        }
    }
}

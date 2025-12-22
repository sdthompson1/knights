/*
 * file_cache.cpp
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

#include "misc.hpp"

#include "file_cache.hpp"
#include "file_info.hpp"
#include "rstream.hpp"
#include "rstream_error.hpp"

#include "boost/make_shared.hpp"

#include <cctype>

namespace {
    void ValidateStdFileName(const std::string &name)
    {
        // This check prevents having a standard file name of something like
        // "../foo/bar", which would allow standard files outside of the "std_files"
        // directory to be loaded!
        for (const char c : name) {
            if (!isalnum(c) && c != '_' && c != '.') {
                throw RStreamError(name, "Invalid file name (only alphanumeric, underscore, dot allowed)");
            }
        }
    }
}

boost::shared_ptr<std::istream> FileCache::openFile(const FileInfo &fi) const
{
    ValidateStdFileName(fi.getPath());
    return boost::make_shared<RStream>(std::string("client/std_files/") + fi.getPath());
}

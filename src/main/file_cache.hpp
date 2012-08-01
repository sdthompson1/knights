/*
 * file_cache.hpp
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

#ifndef FILE_CACHE_HPP
#define FILE_CACHE_HPP

#include "file_info.hpp"

#include "boost/shared_ptr.hpp"

#include <iosfwd>
#include <memory>

class FileCache {
public:
    // The following returns NULL if the file is not in cache, and
    // needs to be downloaded from server.
    boost::shared_ptr<std::istream> openFile(const FileInfo &file) const;

    // "Install" a server file into the cache

    // It's guaranteed that calling installFile() then calling
    // openFile() on the same FileInfo will succeed and return a
    // stream. (But if any other file is installed in the interim, the
    // first file might have been evicted.)
    void installFile(const FileInfo &file, const std::string &contents);

private:
    // At the moment the cache stores only one file at a time, and
    // stores it in RAM only.
    std::auto_ptr<FileInfo> stored_fi;
    std::string stored_contents;
};

#endif

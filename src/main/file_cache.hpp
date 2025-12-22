/*
 * file_cache.hpp
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

#ifndef FILE_CACHE_HPP
#define FILE_CACHE_HPP

#include "boost/shared_ptr.hpp"
#include <iosfwd>
#include <string>

class FileInfo;

class FileCache {
public:
    // Open a file from client/std_files/, or throw an RStreamError on
    // failure.
    // If this returns, the pointer is always non-null.
    boost::shared_ptr<std::istream> openFile(const FileInfo &fi) const;
};

#endif

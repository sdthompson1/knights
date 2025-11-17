/*
 * file_info.hpp
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

/*
 * Stores filename and other information needed to identify a server
 * file.
 *
 */

#ifndef FILE_INFO_HPP
#define FILE_INFO_HPP

#include "network/byte_buf.hpp" // coercri

#include <string>

class FileInfo {
public:
    // construction on server side
    explicit FileInfo(const char *f, const char *cwd);

    // serialization
    explicit FileInfo(Coercri::InputByteBuf &buf);
    void serialize(Coercri::OutputByteBuf &buf) const;

    // interface used by FileCache
    bool isStandardFile() const { return standard_file; }
    const std::string &getPath() const { return pathname; }

    // comparison operator(s)
    bool operator==(const FileInfo &rhs) const 
    {
        return pathname == rhs.pathname
            && standard_file == rhs.standard_file;
    }
    
private:
    // This is a resource pathname i.e. it will be passed to RStream constructor eventually.
    // Therefore, it is a std::string rather than a std::filesystem::path.
    std::string pathname;

    // false: file is on server, must be downloaded by client (or read from cache, if client does caching).
    // true: file is one of the "standard files" included with Knights; client doesn't need to download.
    bool standard_file;
};

#endif

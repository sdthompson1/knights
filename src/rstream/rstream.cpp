/*
 * rstream.cpp
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
#include "rstream_error.hpp"

namespace bfs = boost::filesystem;

namespace {

    enum CheckPathResult {
        CP_OK,          // file found, and confirmed it is within the resource dir tree.
        CP_NOT_FOUND,   // file not found.
        CP_WRONG_DIR,   // file found, but is not under the resource dir tree.
        CP_ERROR        // file found, but there was an error in boost::canonical (can this happen??)
    };

    // CheckPath
    
    // Checks whether 'file' exists and is under a subdirectory of 'dir'.

    // Used to implement the security check whereby we ensure that
    // resource files truly are being loaded from the resource
    // directory (e.g. don't want "/etc/passwd" to be a valid resource
    // name).

    // We use boost::filesystem::canonical to do the check. This has
    // the advantage that it checks for things like symlinks being
    // used to "break out" of the resource directory. (The downside is
    // that it is slightly inefficient since it "stats" each component
    // of the path individually.)

    // Preconditions: It is assumed that 'dir' is in canonical form,
    // although 'file' may not be.

    CheckPathResult CheckPath(const bfs::path &dir, const bfs::path &file)
    {
        boost::system::error_code ec;

        // canonical requires that the file exist... so first check this
        if (!exists(file)) {
            // The file doesn't exist
            return CP_NOT_FOUND;
        }

        bfs::path c_file = bfs::canonical(file, ec);
        if (ec) {
            // An unexpected / "other" error occurred
            return CP_ERROR;
        }

        bfs::path::iterator dir_it = dir.begin(),
            c_file_it = c_file.begin();

        // Iterate through components of the dir-path, and check they
        // equal the corresponding bits of file-path. (i.e. check that
        // dir-path is a prefix of file-path.)

        while (dir_it != dir.end() && *dir_it != ".") {

            // We are somewhere in the middle of the dir-path.

            // If we have reached the end of file-path, then that is an error
            // (case where file-path is the parent, or some ancestor, of dir-path).
            if (c_file_it == c_file.end()) return CP_WRONG_DIR;

            // We are in the middle of both dir-path and file-path
            // Check that the two components are equal
            // (If not, they are "sibling" or "cousin" directories)
            if (*dir_it != *c_file_it) return CP_WRONG_DIR;

            // Advance to next component of each path
            ++dir_it;
            ++c_file_it;
        }

        // We got all the way to the end of dir-path, without finding
        // anything that differed from file-path.

        // In other words we have proved that dir-path is a prefix of
        // file-path, as required.
        
        return CP_OK;
    }
}

bfs::path RStream::base_path;
bool RStream::initialized = false;

void RStream::Initialize(const boost::filesystem::path &base_path_)
{
    if (initialized) {
        throw RStreamError("N/A", "Resource Loader Initialized Twice");
    }

    // It is intended that base_path_ be an absolute path.
    // However, if it is relative, we canonicalize it w.r.t. the current directory.

    // NOTE: If the basepath does not exist, the following call to canonical() will throw.
    
    base_path = bfs::canonical(base_path_);
    
    initialized = true;
}

bool RStream::Exists(const boost::filesystem::path &resource_path)
{
    boost::filesystem::path path_to_open = base_path / resource_path;

    switch (CheckPath(base_path, path_to_open)) {
    case CP_NOT_FOUND:
        return false;
    default:
        // Anything else means the file exists. (It may be that we are not able to access it
        // for whatever reason, but we still report to the caller that it exists.)
        return true;
    }
}

RStream::RStream(const bfs::path & resource_path)
  : std::istream(&my_filebuf)
{
    if (!initialized) {
        throw RStreamError(resource_path, "Resource Loader Not Initialized");
    }

    if (resource_path.has_root_path()) {
        throw RStreamError(resource_path, "Path cannot be absolute");
    }

    boost::filesystem::path path_to_open = base_path / resource_path;   

    // check we are still inside the resource directory
    switch (CheckPath(base_path, path_to_open)) {
    case CP_NOT_FOUND:
        throw RStreamError(resource_path, "File not found");
    case CP_ERROR:
        throw RStreamError(resource_path, "Could not access file");
    case CP_WRONG_DIR:
        throw RStreamError(resource_path, "Path points outside of the base directory");
    }
    
    // Open the resource as a binary file
    bool success = my_filebuf.open(path_to_open,
                                   ios_base::in | ios_base::binary) != 0;
    my_filebuf.pubsetbuf(buffer, BUFSIZE);
    if (!success) throw RStreamError(resource_path, "could not open file");
}

RStream::~RStream()
{
    // make sure the filebuf gets closed on exit.
    my_filebuf.close();
}

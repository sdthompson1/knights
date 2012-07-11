/*
 * rstream.cpp
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

#include "rstream.hpp"

namespace bfs = boost::filesystem;

namespace {
    // CheckPath
    
    // Returns true if 'file' is under a subdirectory of 'dir',
    // otherwise returns false.

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

    // NOTE: This will throw a boost::filesystem::filesystem_error if
    // there is any problem calling canonical() on file.
    // This is not ideal (arguably it should throw an RStreamError)
    // but it is good enough for now.

    bool CheckPath(const bfs::path &dir, const bfs::path &file)
    {
        bfs::path c_file = canonical(file);
        
        bfs::path::iterator dir_it = dir.begin(),
            c_file_it = c_file.begin();

        // Iterate through components of the dir-path, and check they
        // equal the corresponding bits of file-path. (i.e. check that
        // dir-path is a prefix of file-path.)

        while (dir_it != dir.end() && *dir_it != ".") {

            // We are somewhere in the middle of the dir-path.

            // If we have reached the end of file-path, then that is an error
            // (case where file-path is the parent, or some ancestor, of dir-path).
            if (c_file_it == c_file.end()) return false;

            // We are in the middle of both dir-path and file-path
            // Check that the two components are equal
            // (If not, they are "sibling" or "cousin" directories)
            if (*dir_it != *c_file_it) return false;

            // Advance to next component of each path
            ++dir_it;
            ++c_file_it;
        }

        // We got all the way to the end of dir-path, without finding
        // anything that differed from file-path.

        // In other words we have proved that dir-path is a prefix of
        // file-path, as required.
        
        return true;
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
    
    base_path = canonical(base_path_);
    
    initialized = true;
}

RStream::RStream(const bfs::path & resource_path)
  : std::istream(&my_filebuf)
{
    if (!initialized) {
        throw RStreamError(resource_path, "Resource Loader Not Initialized");
    }
    
    boost::filesystem::path path_to_open = base_path / resource_path;

    // check we are still inside the resource directory
    if (!CheckPath(base_path, path_to_open)) {
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

/*
 * rstream.cpp
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
#include "rstream_error.hpp"

#include <vector>

#ifndef VIRTUAL_SERVER
std::filesystem::path RStream::base_path;
bool RStream::initialized = false;

void RStream::Initialize(const std::filesystem::path &base_path_)
{
    if (initialized) {
        throw RStreamError("N/A", "Resource Loader Initialized Twice");
    }

    // It is intended that base_path_ be an absolute path.
    // However, if it is relative, we canonicalize it w.r.t. the current directory.

    // NOTE: If the basepath does not exist, the following call to canonical() will throw.

    base_path = std::filesystem::canonical(base_path_);

    initialized = true;
}
#endif

std::string RStream::NormalizePath(const char *path)
{
    std::vector<std::string> components;  // List of path components found
    std::string comp;  // Current path component being built
    while (true) {
        if (*path == ':') {
            // Colons are not allowed
            throw RStreamError(path, "invalid path");
        } else if (*path == '/' || *path == '\\' || *path == 0) {
            // Path component separator character, or end of string
            if (comp == "" || comp == ".") {
                // Empty or "." path components are just dropped
            } else if (comp == "..") {
                // ".." means move back up a directory
                if (components.empty()) {
                    // Attempt to break out above the root directory!
                    throw RStreamError(path, "invalid path");
                }
                components.pop_back();
            } else {
                // This is a normal path component, add it to the list.
                components.push_back(comp);
            }
            if (*path == 0) break;   // Done!
            comp.clear();  // Not done, clear 'comp' for next component
        } else {
            // Normal character - part of the current path component
            comp += *path;
        }
        ++path;  // Move on to next character
    }

    // Path is valid; construct the normalized version
    std::string output;
    for (std::vector<std::string>::size_type i = 0; i < components.size(); ++i) {
        if (i != 0) {
            output += '/';
        }
        output += components[i];
    }
    return output;
}

std::string RStream::NormalizePath(const std::string &path)
{
    return NormalizePath(path.c_str());
}

bool RStream::Exists(const char *resource_path)
{
    std::string normalized = NormalizePath(resource_path);
    if (normalized.empty()) throw RStreamError(resource_path, "invalid path");

#ifdef VIRTUAL_SERVER
    // Check if the path exists by opening it (this should be good enough)
    std::ifstream str(("RES:" + normalized).c_str(), std::ios::binary);
    return bool(str);
#else
    // Check if the path exists using std::filesystem
    std::filesystem::path path_to_open = base_path / normalized;
    return std::filesystem::exists(path_to_open);
#endif
}

bool RStream::Exists(const std::string &resource_path)
{
    return Exists(resource_path.c_str());
}

RStream::RStream(const char* resource_path)
    : std::istream(&my_filebuf)
{
    construct(resource_path);
}

RStream::RStream(const std::string &resource_path)
    : std::istream(&my_filebuf)
{
    construct(resource_path.c_str());
}

void RStream::construct(const char *resource_path)
{
#ifndef VIRTUAL_SERVER
    if (!initialized) {
        throw RStreamError(resource_path, "Resource Loader Not Initialized");
    }
#endif

    std::string normalized = NormalizePath(resource_path);
    if (normalized.empty()) throw RStreamError(resource_path, "invalid path");

#ifdef VIRTUAL_SERVER
    bool success = my_filebuf.open(("RES:" + normalized).c_str(),
                                   ios_base::in | ios_base::binary) != 0;
#else
    bool success = my_filebuf.open(base_path / normalized,
                                   ios_base::in | ios_base::binary) != 0;
#endif

    my_filebuf.pubsetbuf(buffer, BUFSIZE);
    if (!success) throw RStreamError(resource_path, "could not open file");
}

RStream::~RStream()
{
    // make sure the filebuf gets closed on exit.
    my_filebuf.close();
}

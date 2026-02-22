/*
 * vfs.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2026.
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

#ifndef VFS_HPP
#define VFS_HPP

#include <fstream>
#include <string>

#ifndef VIRTUAL_SERVER
#include <filesystem>
#include <map>
#include <set>
class XXHash;
#endif

/*
 * Virtual Filesystem (VFS). Abstracts the locations of resource files
 * on the local filesystem.
 *
 * First, configure mount points using add() and the related methods,
 * which describe how local filesystem directories are mapped into the
 * VFS namespace. Then use open() to open files by their VFS path.
 *
 * VFS paths use the following conventions:
 *
 * '/' separates path components; '\' is accepted and treated the same.
 * ':' is an error (to avoid clashes with Windows drive names).
 * ".." navigates up one directory; "." and empty components are ignored.
 * Attempting to navigate above the root is an error.
 *
 * (Note: copying a VFS is fine; it just copies the strings representing
 * the mount points.)
 *
 */

class VFS {
public:

#ifndef VIRTUAL_SERVER
    // Add a new mount point. vfs_path must be a single path component
    // (no slashes or colons). If vfs_path is empty, the local directory
    // is mapped into the root of the VFS.
    // If a local file or directory clashes with a mount point, the
    // mount point takes priority.
    void add(std::filesystem::path local_path, std::string vfs_path);

    // Remove a mount point.
    void remove(const std::string &vfs_path);
#endif

    // Enable/disable mount points.
    void disableAll();
    void enable(const std::string &vfs_path);
#ifndef VIRTUAL_SERVER
    void enableAll();
    void disable(const std::string &vfs_path);
#endif

    // Open a file by VFS path. Returns an open, binary-mode ifstream.
    // Throws RStreamError if the path is invalid or the file cannot be opened.
    std::ifstream open(const char *vfs_path) const;
    std::ifstream open(const std::string &vfs_path) const;

    // Returns true if the given VFS path refers to an existing file.
    // Throws RStreamError if the path is invalid.
    bool exists(const char *vfs_path) const;
    bool exists(const std::string &vfs_path) const;

#ifndef VIRTUAL_SERVER
    // Recursively hash all files in a directory.
    // Adds file contents, names, and sizes to the given "hasher".
    // Throws RStreamError if the path is invalid or the directory doesn't exist.
    void hashDirectory(const std::string &vfs_path, XXHash &hasher) const;
#endif

    // Normalize a VFS path by resolving ".", "..", and empty components.
    // Throws RStreamError if the path is invalid.
    // Note: allows a completely empty result (i.e. the root); callers
    // that don't want this must check for it themselves.
    static std::string normalizePath(const char *vfs_path);
    static std::string normalizePath(const std::string &vfs_path);

private:
#ifndef VIRTUAL_SERVER
    // Map a *normalized* VFS path to a local filesystem path, or throw.
    std::filesystem::path map(const std::string &normalized) const;

    std::map<std::string, std::filesystem::path> mappings;
    std::set<std::string> enabled;
#endif
};

#endif  // VFS_HPP

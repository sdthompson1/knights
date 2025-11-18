/*
 * rstream.hpp
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

#ifndef RSTREAM_HPP
#define RSTREAM_HPP

#include <istream>
#include <fstream>

#ifndef VIRTUAL_SERVER
#include <filesystem>
class XXHash;
#endif

/*
 * Resource streams. Like ifstream except we load from "resources"
 * rather than files. The idea is to abstract away details of where on
 * the filesystem the resources are actually located.
 *
 * 
 * Usage:
 * 
 * First, set the base directory by calling:
 * 
 * RStream::Initialize(base_path);
 *
 * Then, create one or more RStreams, passing a filename to the ctor,
 * e.g.:
 * 
 * RStream rs("somefile.txt");
 *
 * Note that resource paths are, by definition, binary strings of
 * bytes, with the following interpretation:
 *
 * A forward slash ('/') separates path components. A backslash ('\') is
 * also usable and treated the same as a forward slash.
 *
 * The colon (':') character is an error (to avoid any potential
 * clashes with Windows drive names).
 *
 * Any other character/byte is considered part of a path component.
 * Theoretically it would be possible to include non-ASCII characters
 * (e.g. UTF-8 encodings) but this is not recommended as only ASCII
 * has been tested.
 *
 * The path components have the following meanings:
 *
 * A path component of ".." means "go up to the parent directory".
 * Paths are normalized by removing redundant ".." components if
 * applicable; e.g. "dir1/../dir2/file" is considered equivalent to
 * "dir2/file". If a path tries to go "above the root", it is an
 * error; e.g. "../file" or "dir/../../file" would both be rejected.
 *
 * A path component of "." means "the current directory" and is
 * effectively ignored. E.g. "./file" is the same as "file". Also
 * (perhaps weirdly) "file/." is considered the same as "file".
 *
 * Empty path components are also ignored (in the same way as "."
 * components). E.g. "//file/" is the same as "file".
 *
 * Path components other than ".", ".." or "" are treated as directory
 * names (if not the right-most component) or a file name (otherwise).
 * The path is then mapped to a file on the local file system,
 * starting from the base path, in the obvious way.
 *
 */

class RStream : public std::istream {
public:
    // Constructor: Opens a resource file, given the resource path (in
    // the format described above)
    explicit RStream(const char *resource_path);
    explicit RStream(const std::string &resource_path);

    // Destructor (closes the file)
    ~RStream();

#ifndef VIRTUAL_SERVER
    // Initialize function - must be called before creating any RStreams
    static void Initialize(const std::filesystem::path &base_path_);
#endif

    // Static function to tell whether a given resource exists.
    static bool Exists(const char *resource_path);
    static bool Exists(const std::string &resource_path);

    // Static function to normalize a path by removing "." and ".."
    // components etc.
    // Throws exception if the path is invalid.
    // Note: this does allow completely empty paths; if this is not
    // wanted, it must be checked for by the caller.
    static std::string NormalizePath(const char *resource_path);
    static std::string NormalizePath(const std::string &resource_path);

#ifndef VIRTUAL_SERVER
    // Recursively hash all files in a directory.
    // Adds file contents, names, and sizes to the given "hasher".
    // Throws RStreamError if path is invalid or directory doesn't exist.
    static void HashDirectory(const std::string &resource_path, XXHash &hasher);
#endif

private:
    void construct(const char *);

private:
    // at the moment we only implement by using a filebuf. could add a
    // custom stream buffer to do more sophisticated stuff. (but still
    // want rstream itself to inherit from istream.)
    std::filebuf my_filebuf;
    enum { BUFSIZE = 1024 };
    char buffer[BUFSIZE];

    // static members
#ifndef VIRTUAL_SERVER
    static std::filesystem::path base_path;
    static bool initialized;
#endif
};

#endif  // RSTREAM_HPP

/*
 * rstream.hpp
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

/*
 * Resource streams. Like ifstream except we load from "resources" rather than files.
 *
 * A resource is currently just a file (but loaded from a standard path, so we don't have to worry about
 * current working directory etc), but in theory it could be something more complicated, e.g.
 * some sort of virtual file system.
 *
 * Usage:
 * 
 * First, set the base directory by calling
 * 
 * RStream::Initialize(path);
 *
 * Then, create RStreams passing a resource name to the ctor:
 * 
 * RStream rs("fred.txt");
 * 
 * The system will automatically find fred.txt from the base
 * directory. rs can now be used as an ordinary istream.
 *
 * Resource names can be paths relative to the base directory, e.g.
 * 
 * RStream rs("somedir/somefile.png");
 *
 * However, it is checked that relative paths do not go "above" the
 * base directory, e.g. the following would fail:
 * 
 * RStream rs("../something");   // wrong
 * RStream rs("/etc/passwd");    // wrong
 *
 * This is to prevent user-supplied scripts (etc) from accessing parts
 * of the filesystem that they are not supposed to.
 *
 * Note: resource names are assumed to be ASCII strings. The base
 * directory, however, is a boost::filesystem::path.
 * 
 */

#ifndef RSTREAM_HPP
#define RSTREAM_HPP

#include <string>
#include <istream>
#include <fstream>
#include <exception>

#include "boost/filesystem.hpp"
#include "boost/filesystem/fstream.hpp"

class RStreamError : public std::exception {
public:
    RStreamError(const std::string &r, const std::string &e);
    virtual ~RStreamError() throw() { }
    const std::string & getResource() const { return resource; }
    const std::string &getErrorMsg() const { return error_msg; }
    const char *what() const throw();

private:
    std::string resource, error_msg, what_str;
};


class RStream : public std::istream {
public:
    explicit RStream(const std::string &resource_name);
    ~RStream();

    static void Initialize(const boost::filesystem::path &base_path_);

private:
    // at the moment we only implement by using a filebuf. could add a custom stream buffer
    // to do more sophisticated stuff. (but still want rstream itself to inherit from istream.)
    boost::filesystem::filebuf my_filebuf;
    enum { BUFSIZE = 1024 };
    char buffer[BUFSIZE];
    
    static boost::filesystem::path base_path;
    static bool initialized;
};

#endif  // RSTREAM_HPP

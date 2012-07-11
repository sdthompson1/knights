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

#ifndef RSTREAM_HPP
#define RSTREAM_HPP

#include <istream>
#include <fstream>

#include "boost/filesystem.hpp"
#include "boost/filesystem/fstream.hpp"


/*
 * Resource streams. Like ifstream except we load from "resources"
 * rather than files. The idea is to abstract away details of where on
 * the filesystem the resources are actually located.
 *
 * 
 * Usage:
 * 
 * First, set the base directory by calling
 * 
 * RStream::Initialize(base_path);
 *
 * Then, create one or more RStreams; pass a boost::filesystem::path
 * to the ctor, e.g.:
 * 
 * RStream rs("somefile.txt");
 *
 * (Here, we construct the path implicitly from a string)
 * 
 * The resource path is given relative to the base directory. However,
 * it is checked that resource paths do not go "above" the base
 * directory. e.g. "somedir/../file" is valid, but "../file" is not
 * valid, nor is "/etc/passwd".
 * 
 */

class RStream : public std::istream {
public:
    explicit RStream(const boost::filesystem::path &resource_path);
    ~RStream();

    // Initialize function
    static void Initialize(const boost::filesystem::path &base_path_);

    // Static function to tell whether a given resource exists.
    static bool Exists(const boost::filesystem::path &resource_path);

private:
    // at the moment we only implement by using a filebuf. could add a
    // custom stream buffer to do more sophisticated stuff. (but still
    // want rstream itself to inherit from istream.)
    boost::filesystem::filebuf my_filebuf;
    enum { BUFSIZE = 1024 };
    char buffer[BUFSIZE];

    // static members
    static boost::filesystem::path base_path;
    static bool initialized;
};

#endif  // RSTREAM_HPP

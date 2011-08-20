/*
 * rstream.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
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
 * To do:
 * More advanced RStreamError class?
 * Consideration of text vs binary files?
 * Add support for vfs / maybe a url like syntax.
 * 
 */

#include "rstream.hpp"

RStreamError::RStreamError(const string &r, const string &e)
    : resource(r), error_msg(e)
{
    what_str = "error loading '" + getResource() +
        "': " + getErrorMsg();
}

const char *RStreamError::what() const throw()
{
    return what_str.c_str();
}


string RStream::resource_path;
bool RStream::initialized = false;

void RStream::Initialize(const string &resource_path_)
{
    if (initialized) throw RStreamError("N/A", "Resource Loader Initialized Twice");
    resource_path = resource_path_;
    if (resource_path.length()>0 && *(resource_path.end()-1) != '/') {
        resource_path += '/';
    }
    initialized = true;
}

RStream::RStream(const string &resource_name)
    : istream(&my_filebuf)
{
    // Slight hack to detect whether this is an absolute path... (added 28-Feb-2010)
    std::string filename;
    if (resource_name.length() >= 1 && resource_name[0] == '/'    // unix
    || resource_name.length() >= 2 && resource_name[1] == ':') {  // windows
        filename = resource_name;
    } else {
        filename = resource_path + resource_name;
    }

    // Open the resource as a (unix mode) file for now.
    bool success = my_filebuf.open(filename.c_str(), ios_base::in | ios_base::binary);
    my_filebuf.pubsetbuf(buffer, BUFSIZE);
    if (!success) throw RStreamError(resource_name, "could not open file");
}

RStream::~RStream()
{
    // make sure the filebuf gets closed on exit.
    my_filebuf.close();
}

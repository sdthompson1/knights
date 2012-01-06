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
 * current working directory etc), but in the future could be some sort of virtual file system.
 *
 * API:
 * RStream rs("fred.txt");   (%%% perhaps a more url like syntax in future?)
 *  -- the system will automatically find fred.txt from the "standard" location(s)
 * Then manipulate rs as an ordinary istream.
 *
 * The standard location is currently "." but can optionally be reset by calling
 * RStream::Initialize(path); eg if(argc>1)RStream::Initialize(argv[1]); is useful for testing.
 *
 */

#ifndef RSTREAM_HPP
#define RSTREAM_HPP

#include <string>
#include <istream>
#include <fstream>
#include <exception>
using namespace std;

class RStreamError : public std::exception {
public:
	RStreamError(const string &r, const string &e);
	virtual ~RStreamError() throw() { }
	const string &getResource() const { return resource; }
	const string &getErrorMsg() const { return error_msg; }
	const char *what() const throw();

private:
	string resource, error_msg, what_str;
};


class RStream : public istream {
public:
	static void Initialize(const string &resource_path_);
	explicit RStream(const string &resource_name);
	~RStream();

private:
	// at the moment we only implement by using a filebuf. could add a custom stream buffer
	// to do more sophisticated stuff. (but still want rstream itself to inherit from istream.)
	filebuf my_filebuf;
	enum { BUFSIZE = 1024 };
	char buffer[BUFSIZE];
	static string resource_path;
	static bool initialized;
};

#endif  // RSTREAM_HPP

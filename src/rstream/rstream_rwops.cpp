/*
 * rstream_rwops.cpp
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

#include "rstream_rwops.hpp"

#include <fstream>

namespace {

int Rseek(SDL_RWops *context, int offset, int whence)
{
	std::ifstream *str = static_cast<std::ifstream *>(context->hidden.unknown.data1);
	switch (whence) {
	case SEEK_SET:
        str->seekg(offset, std::ios_base::beg);
		break;
	case SEEK_CUR:
        str->seekg(offset, std::ios_base::cur);
		break;
	case SEEK_END:
        str->seekg(offset, std::ios_base::end);
		break;
	default:
		return -1; // error
	}
	if (!*str) return -1;
	else return str->tellg();
}

int Rread(SDL_RWops *context, void *ptr, int size, int maxnum)
{
	std::ifstream *str = static_cast<std::ifstream *>(context->hidden.unknown.data1);
	str->read(static_cast<char*>(ptr), size*maxnum);
	if (!*str) return -1;
	else return str->gcount() / size;
}

int Rwrite(SDL_RWops *context, const void *ptr, int size, int maxnum)
{
	return -1; // read only!
}

int Rclose(SDL_RWops *context)
{
	std::ifstream *str = static_cast<std::ifstream *>(context->hidden.unknown.data1);
	delete str;
	delete context;
	return 0;
}

}  // namespace


SDL_RWops* RWFromVFS(const VFS &vfs, const std::string &vfs_path)
{
	// NB: not quite exception safe, that's the price of a C-like interface.
	SDL_RWops *rw = new SDL_RWops;
	std::ifstream *str = new std::ifstream(vfs.open(vfs_path));
	rw->seek = &Rseek;
	rw->read = &Rread;
	rw->write = &Rwrite;
	rw->close = &Rclose;
	rw->hidden.unknown.data1 = static_cast<void*>(str);
	return rw;
}

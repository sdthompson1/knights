/*
 * file_info.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2013.
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

#include "misc.hpp"

#include "file_info.hpp"
#include "rstream_find.hpp"

FileInfo::FileInfo(const char *f, const char *cwd)
{
    if (!f) throw std::invalid_argument("Incorrect argument to FileInfo ctor");

    if (f[0] == '+') {
        standard_file = true;
        pathname = (f+1);
    } else {
        standard_file = false;
        if (cwd) {
            pathname = RStreamFind(f, cwd);
        } else {
            pathname = f;
        }
    }
}       

FileInfo::FileInfo(Coercri::InputByteBuf &buf)
{
    standard_file = buf.readUbyte() != 0;
    pathname = buf.readString();
}

void FileInfo::serialize(Coercri::OutputByteBuf &buf) const
{
    buf.writeUbyte(standard_file ? 1 : 0);
    buf.writeString(pathname.generic_string());
}

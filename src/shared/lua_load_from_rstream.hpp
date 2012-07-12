/*
 * lua_load_from_rstream.hpp
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

#ifndef LUA_LOAD_FROM_RSTREAM_HPP
#define LUA_LOAD_FROM_RSTREAM_HPP

#include "boost/filesystem.hpp"

#include <string>

struct lua_State;

// Read a chunk from an Rstream, and execute it, with no args, and the given no.
//   of return values (can be LUA_MULTRET).
// On error, throws LuaError.
// Nothing is read from the stack on entry.
// On exit, the return values are pushed (as in lua_call).
//
// Pathing: The global variable _CWD will be read, this will be used as a path prefix.
// If the file does not exist under that directory (according to RStream::Exists)
// then it will be looked for in the resource root directory. If still not found, an
// exception will be thrown.
//
// While the chunk is executing _CWD will be set to the directory that the chunk was
// loaded from. It will be restored back again when the chunk exits.

void LuaExecRStream(lua_State *lua, const boost::filesystem::path &filename, int nresults);


// Reads a lua chunk from a string and pushes it onto the lua stack.
// (Deprecated, will be removed once we convert knights_data to lua.)
void LuaLoadFromString(lua_State *lua, const char *str);

#endif

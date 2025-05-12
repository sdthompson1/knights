/*
 * lua_load_from_rstream.hpp
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

#ifndef LUA_LOAD_FROM_RSTREAM_HPP
#define LUA_LOAD_FROM_RSTREAM_HPP

#include <string>

struct lua_State;

// Read a chunk from an Rstream, and execute it, with the given no of args,
//   and the given no of return values (can be LUA_MULTRET).
//
// On entry, pops the given no of args from the stack.
// On exit, pushes the given no of return values (or all return values if LUA_MULTRET).
// On error, raises a Lua error message if there is enclosing Lua code, otherwise
//  throws LuaError.
//
// While the chunk is executing _CWD will be set to the directory that the chunk was
// loaded from. It will be restored back again when the chunk exits.
//
// If look_in_cwd is set, then the file will be looked for in _CWD first, then the RStream root.
// If look_in_cwd is clear, the file will be looked for only in the RStream root.

void LuaExecRStream(lua_State *lua, const std::string &filename,
                    int nargs, int nresults,
                    bool look_in_cwd,
                    bool use_dofile_namespace_proposal);

#endif

/*
 * lua_vfs.hpp
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

// Miscellaneous helpers for working with the Lua/VFS interface

#ifndef LUA_VFS_HPP
#define LUA_VFS_HPP

#include "include_lua.hpp"

#include <string>

class VFS;

// Store a VFS in the Lua state.
void SetLuaVFS(lua_State *lua, const VFS &vfs);

// Get the stored VFS from the Lua state. The result is valid until the next SetLuaVFS call.
const VFS & GetLuaVFS(lua_State *lua);

// Return _CWD, or an empty string if _CWD is not set.
std::string GetCWD(lua_State *lua);

// Set _CWD to a new value
void SetCWD(lua_State *lua, const std::string &path);

// Given a filename which might be relative to _CWD, return a fully
// normalized RStream path to the file, or throw an exception if the
// file doesn't exist.
std::string LuaResolveFile(lua_State *lua,
                           const std::string &path);

#endif

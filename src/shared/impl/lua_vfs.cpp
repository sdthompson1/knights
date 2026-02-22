/*
 * lua_vfs.cpp
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

#include "misc.hpp"

#include "lua_vfs.hpp"
#include "rstream_error.hpp"
#include "vfs.hpp"

// Address of this variable is used as a unique Lua registry key.
static const char VFS_REGISTRY_KEY = 0;

static int vfs_gc(lua_State *lua)
{
    VFS *vfs = static_cast<VFS *>(lua_touserdata(lua, 1));
    vfs->~VFS();
    return 0;
}

void SetLuaVFS(lua_State *lua, const VFS &vfs)
{
    // Allocate a full userdata and copy-construct the VFS into it.
    void *mem = lua_newuserdata(lua, sizeof(VFS));
    new (mem) VFS(vfs);

    // Attach a metatable with __gc so the destructor runs when collected.
    if (luaL_newmetatable(lua, "KnightsVFS")) {
        lua_pushcfunction(lua, vfs_gc);
        lua_setfield(lua, -2, "__gc");
    }
    lua_setmetatable(lua, -2);

    // registry[&VFS_REGISTRY_KEY] = userdata
    lua_pushlightuserdata(lua, (void *)&VFS_REGISTRY_KEY);
    lua_insert(lua, -2);
    lua_rawset(lua, LUA_REGISTRYINDEX);
}

const VFS & GetLuaVFS(lua_State *lua)
{
    lua_pushlightuserdata(lua, (void *)&VFS_REGISTRY_KEY);
    lua_rawget(lua, LUA_REGISTRYINDEX);
    VFS *vfs = static_cast<VFS *>(lua_touserdata(lua, -1));
    lua_pop(lua, 1);
    return *vfs;
}

std::string GetCWD(lua_State *lua)
{
    std::string result;
    lua_getglobal(lua, "_CWD");
    const char *p = lua_tostring(lua, -1);
    if (p) {
        result = p;
    }
    lua_pop(lua, 1);
    return result;
}

void SetCWD(lua_State *lua, const std::string &path)
{
    lua_pushstring(lua, path.c_str());
    lua_setglobal(lua, "_CWD");
}

std::string LuaResolveFile(lua_State *lua,
                           const std::string &path)
{
    std::string cwd = GetCWD(lua);
    const VFS & vfs = GetLuaVFS(lua);

    if (path.empty()) {
        throw RStreamError(path, "empty filename");
    }

    // If the path doesn't start with a slash then try it relative to _CWD first
    bool is_absolute = (path[0] == '/' || path[0] == '\\');
    if (!is_absolute) {
        std::string proposed = cwd + "/" + path;
        if (vfs.exists(proposed)) {
            return vfs.normalizePath(proposed);
        }
    }

    // Otherwise, just normalize the path as-is
    std::string normalized = vfs.normalizePath(path);
    if (vfs.exists(normalized)) {
        return normalized;
    } else {
        throw RStreamError(path, "file not found");
    }
}

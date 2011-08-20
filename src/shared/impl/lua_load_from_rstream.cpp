/*
 * lua_load_from_rstream.cpp
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

#include "misc.hpp"

#include "lua_load_from_rstream.hpp"
#include "my_exceptions.hpp"
#include "rstream.hpp"

#include "lua.hpp"

namespace {
    struct ReadContext {
        const std::string * filename;
        RStream * str;
        char buf[512];
    };
    
    const char * LuaReader(lua_State *lua, void *context, size_t *size)
    {
        ReadContext * rc = static_cast<ReadContext*>(context);
        rc->str->read(&rc->buf[0], sizeof(rc->buf));

        // if we reach eof, eofbit & failbit will be set, and gcount() will return 0 which is what we want
        // on any other error, badbit will be set, which we test for below:
        if (rc->str->bad()) {
            throw LuaError(std::string("Error reading from Lua file '") + *rc->filename + "'");
        }
        
        *size = rc->str->gcount();
        return &rc->buf[0];
    }

    void HandleLuaError(lua_State *lua)
    {
        // get the error msg & throw exception
        const std::string err_msg = lua_tostring(lua, -1);
        lua_pop(lua, 1);
        throw LuaError(err_msg);
    }
}

// Load lua code in rstream named 'filename'
void LuaLoadFromRStream(lua_State *lua, const std::string &filename)
{
    RStream str(filename);
    if (!str) throw LuaError(std::string("Error opening Lua file '") + filename + "'");
    
    // filter out binary chunks, because of potential security problems
    // (http://www.lua.org/bugs.html#5.1.4-1)
    const int c = str.get();
    if (str.bad()) throw LuaError(std::string("Error reading from Lua file '") + filename + "'");
    if (c == 27) throw LuaError(std::string("Lua file '") + filename + "' is in binary format, which is not supported.");
    if (!str.eof()) str.putback(c);
    
    // now load it
    ReadContext rc;
    rc.filename = &filename;
    rc.str = &str;
    const std::string chunkname = "@" + filename;
    const int result = lua_load(lua, &LuaReader, &rc, chunkname.c_str());  // pushes 1 result value
    if (result != 0) HandleLuaError(lua);
}

// Load lua code from a string
void LuaLoadFromString(lua_State *lua, const char *str)
{
    if (!str || str[0] == 0) throw LuaError("Empty lua string");
    if (str[0] == 27) throw LuaError("Lua string is in binary format, which is not supported");

    const int result = luaL_loadstring(lua, str);
    if (result != 0) HandleLuaError(lua);
}

void LuaExec(lua_State *lua)
{
    const int result = lua_pcall(lua, 0, 0, 0);
    if (result != 0) {
        // get the error msg & throw exception
        const std::string err_msg = lua_tostring(lua, -1);
        lua_pop(lua, 1);
        throw LuaError(err_msg);
    }
}

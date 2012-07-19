/*
 * lua_load_from_rstream.cpp
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

#include "misc.hpp"

#include "lua_exec.hpp"
#include "lua_load_from_rstream.hpp"
#include "my_exceptions.hpp"
#include "rstream.hpp"

#include "lua.hpp"

namespace {
    struct ReadContext {
        std::string filename;
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
            throw LuaError(std::string("Error reading from Lua file '") + rc->filename + "'");
        }
        
        *size = rc->str->gcount();
        return &rc->buf[0];
    }

    boost::filesystem::path RemoveDotDot(const boost::filesystem::path &p)
    {
        // This is like a cut-down canonical(), that does not chase symlinks.
        boost::filesystem::path result;
        for (boost::filesystem::path::iterator it = p.begin(); it != p.end(); ++it) {
            if (*it == ".") continue;
            if (*it == "..") {
                result.remove_filename();
                continue;
            }
            result /= *it;
        }
        return result;
    }

    boost::filesystem::path GetCWD(lua_State *lua)
    {
        // Returns CWD, or empty path if CWD not set.
        boost::filesystem::path result;
        lua_getglobal(lua, "_CWD");
        const char *p = lua_isstring(lua, -1) ? lua_tostring(lua, -1) : "";
        if (p) {
            result = p;
        }
        lua_pop(lua, 1);
        return result;   
    }

    void SetCWD(lua_State *lua, const boost::filesystem::path &path)
    {
        lua_pushstring(lua, path.generic_string().c_str());
        lua_setglobal(lua, "_CWD");
    }
}

// Load and execute lua code in rstream named 'filename'
void LuaExecRStream(lua_State *lua, const boost::filesystem::path &filename,
                    int nargs, int nresults,
                    bool look_in_cwd)
{
    // Catch exceptions and convert them to lua errors if required.
    try {

        boost::filesystem::path to_load;
        boost::filesystem::path old_cwd = GetCWD(lua);
        
        bool ready = false;

        if (look_in_cwd && !filename.has_root_path()) {
            // look for it in CWD
            to_load = RemoveDotDot(old_cwd / filename);
            ready = RStream::Exists(to_load);
        }
        if (!ready) {
            // look for it in the root
            to_load = filename;
        }

        // open stream
        // (may throw)
        RStream str(to_load);

        if (!str) {
            // Throw a C++ error.
            // (Will be converted to a Lua error by the below catch block, if necessary.)
            std::string err_msg = std::string("Error opening Lua file '") + filename.generic_string() + "'";
            throw LuaError(err_msg);
        }

        // set _CWD to the new cwd
        SetCWD(lua, to_load.parent_path());
        
        // now load it (pushes lua function onto the stack)
        // note: we accept only text chunks, for security reasons
        ReadContext rc;
        rc.filename = to_load.generic_string();
        rc.str = &str;
        const std::string chunkname = "@" + rc.filename;
        const int result = lua_load(lua, &LuaReader, &rc, chunkname.c_str(), "t");  // pushes 1 lua function
        if (result != 0) {
            // Pick up the Lua message and convert it to a C++ exception.
            const std::string err_msg = lua_isstring(lua, -1) 
                ? lua_tostring(lua, -1) : "<No err msg>";
            lua_pop(lua, 1);
            throw LuaError(err_msg);
        }

        // stack is currently [<stuff> arg1 .. argn func]
        // change this to [<stuff> func arg1 .. argn], as required for LuaExec
        lua_insert(lua, -nargs-1);
        
        // execute the script
        // stack will be changed to [<stuff> result1 .. resultn]
        LuaExec(lua, nargs, nresults);

        // now restore _CWD and return.
        SetCWD(lua, old_cwd);
    
    } catch (std::exception &e) {

        // See if we need to convert this to a Lua error
        lua_Debug dummy;
        if (lua_getstack(lua, 0, &dummy)) {
            // convert to lua exception
            lua_pushstring(lua, e.what());
            lua_error(lua);
        } else {
            // re-throw as C++ exception
            throw;
        }
    }
}

// Load lua code from a string
// (Deprecated)
void LuaLoadFromString(lua_State *lua, const char *str)
{
    if (!str || str[0] == 0) throw LuaError("Empty lua string");
    if (str[0] == 27) throw LuaError("Lua string is in binary format, which is not supported");

    const int result = luaL_loadstring(lua, str);
    if (result != 0) {
        // throw a C++ exception on errors.
        const std::string err_msg = lua_isstring(lua, -1)
            ? lua_tostring(lua, -1) : "<No err msg>";
        lua_pop(lua, 1);
        throw LuaError(err_msg);
    }
}

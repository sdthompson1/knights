/*
 * lua_sandbox.cpp
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

#include "lua.hpp"
#include "lua_sandbox.hpp"
#include "my_exceptions.hpp"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace {
    struct LuaDeleter {
        void operator()(lua_State *lua)
        {
            lua_close(lua);
        }
    };
    
#ifndef NDEBUG
    void CheckSorted(const char ** whitelist)
    {
        const char ** p = whitelist;
        std::string prev = *p;
        ++p;
        while (*p) {
            const std::string here = *p;
            ASSERT(here > prev);
            prev = here;
            p++;
        }
    }
#endif
        
    // Whitelists of functions that can be included in the sandbox
    // environment. These must be SORTED.
    
    // NOTE: If you add a new whitelist, don't forget to update MakeLuaSandbox()
    // to actually apply the new whitelist!
    
    const char * global_whitelist[] = {
        "_G",         // not a problem: we don't care if they have access to global environment
        "_VERSION",   // safe
        "assert",     // safe
        // NOT: collectgarbage
        "coroutine",  // no problem; theoretically they could modify this table, thus affecting
                      // code outside the sandbox, but:
                      //  a) this would be considered rude; and
                      //  b) they won't be able to make the code outside the sandbox DO anything nasty,
                      //       because we haven't exposed any "nasty" functions to Lua anyway.
        // NOT: debug
        // NOT: dofile
        "error",      // safe
        "getfenv",    // not a problem: they can access whatever environments they want,
                      // it won't do them any good, because we control the global environment
        "getmetatable",  // they might use this to e.g. redefine string metatable, but we don't care
        // NOT: io
        "ipairs",     // safe
        // NOT: load
        // NOT: loadfile
        // NOT: loadstring (might be ok, not sure, but disable for now)
        "math",       // no problem (see whitelist below)
        // NOT: module
        // NOT: newproxy (undocumented base library function)
        "next",       // safe
        // NOT: os
        // NOT: package
        "pairs",      // safe
        "pcall",      // safe
        "print",      // safe (printing to stdio is harmless)
        "rawequal",   // bypasses metatables but we don't care
        "rawget",     // bypasses metatables but we don't care
        "rawset",     // bypasses metatables but we don't care
        // NOT: require
        "select",     // safe
        "setfenv",    // can modify environments of other functions, but we don't care
        "setmetatable",   // they might use this to e.g. redefine string metatable, but we don't care
        "string",     // no problem (see whitelist below)
        "table",      // no problem (see whitelist below)
        "tonumber",   // safe
        "tostring",   // safe
        "type",       // safe
        "unpack",     // safe
        "xpcall",     // safe
        0
    };

    const char * coroutine_whitelist[] = {
        "create",  // safe
        "resume",  // safe
        "running", // safe
        "status",  // safe
        "wrap",    // safe
        "yield",   // safe
        0
    };
    const char * string_whitelist[] = {
        "byte",  // safe
        "char",  // safe
        // NOT: dump (if only because we don't support binary loading of chunks)
        "find",  // safe
        "format",  // safe
        "gmatch",  // safe
        "gsub",    // safe
        "len",     // safe
        "lower",   // safe
        "match",   // safe
        "rep",     // safe
        "reverse", // safe
        "sub",     // safe
        "upper",   // safe
        0
    };
    const char * table_whitelist[] = {
        "insert",  // safe
        "maxn",    // safe
        "remove",  // safe
        "sort",    // safe
        0
    };
    const char * math_whitelist[] = {
        "abs",   // safe
        "acos",  // safe
        "asin",  // safe
        "atan",  // safe
        "atan2", // safe
        "ceil",  // safe
        "cos",   // safe
        "cosh",  // safe
        "deg",   // safe
        "exp",   // safe
        "floor", // safe
        "fmod",  // safe
        "frexp", // safe
        "huge",  // safe
        "ldexp", // safe
        "log",   // safe
        "log10", // safe
        "max",   // safe
        "min",   // safe
        "modf",  // safe
        "pi",    // safe
        "pow",   // safe
        "rad",   // safe
        // NOT: random, because Knights has its own RNG (for game recording/playback purposes).
        // NOT: randomseed, for same reason
        "sin",   // safe
        "sinh",  // safe
        "sqrt",  // safe
        "tan",   // safe
        "tanh",  // safe
        0
    };

    // Applies a whitelist to the table on top of stack.
    // Pops the table afterwards.
    void ApplyWhitelist(lua_State *lua, const char ** whitelist )
    {
#ifndef NDEBUG
        CheckSorted(whitelist);
#endif
        
        // make a list of key strings in the table.
        // note: standard lua libs should only insert string keys.
        // we throw an exception if this is not the case

        std::vector<std::string> tbl_keys;
        
        lua_pushnil(lua);  // table at index -2, key at -1
        while (lua_next(lua, -2) != 0) {
            // key now at index -2, value at index -1
            if (lua_type(lua, -2) != LUA_TSTRING) throw LuaError("unexpected key in library table");
            const char *k = lua_tostring(lua, -2);
            tbl_keys.push_back(k);
            lua_pop(lua, 1);  // pop value (leave key)
        }
        // table is now at index -1
        
        // sort the collected table keys
        std::sort(tbl_keys.begin(), tbl_keys.end());

        // copy the whitelist to a string vector
        std::vector<std::string> whitelist_vec;
        whitelist_vec.reserve(30);
        const char ** p = whitelist;
        while (*p) {
            whitelist_vec.push_back(*p);
            ++p;
        }
        
        // now take difference from the whitelist table
        std::vector<std::string> keys_to_delete;
        keys_to_delete.reserve(30);
        std::set_difference(tbl_keys.begin(), tbl_keys.end(), whitelist_vec.begin(), whitelist_vec.end(),
                            std::back_inserter(keys_to_delete));

        // now delete them one by one
        for (std::vector<std::string>::const_iterator it = keys_to_delete.begin();
        it != keys_to_delete.end(); ++it) {
            lua_pushnil(lua);  // table at -2, nil at -1
            lua_setfield(lua, -2, it->c_str());
        }

        lua_pop(lua, 1);  // pop the table.
    }

    // The lua libraries that we want to open
    const luaL_Reg kts_lua_libs[] = {
          {"", luaopen_base},
          {LUA_TABLIBNAME, luaopen_table},
          {LUA_STRLIBNAME, luaopen_string},
          {LUA_MATHLIBNAME, luaopen_math},
          {0, 0}
    };

}

boost::shared_ptr<lua_State> MakeLuaSandbox()
{
    boost::shared_ptr<lua_State> lua(luaL_newstate(), LuaDeleter());
    
    // Copied from linit.c (but with modified library list):
    const luaL_Reg *lib = kts_lua_libs;
    for (; lib->func; lib++) {
        lua_pushcfunction(lua.get(), lib->func);
        lua_pushstring(lua.get(), lib->name);
        lua_call(lua.get(), 1, 0);
    }

    // Apply our whitelists.
    lua_pushvalue(lua.get(), LUA_GLOBALSINDEX);
    ApplyWhitelist(lua.get(), global_whitelist);

    lua_getglobal(lua.get(), "coroutine");
    ApplyWhitelist(lua.get(), coroutine_whitelist);

    lua_getglobal(lua.get(), "math");
    ApplyWhitelist(lua.get(), math_whitelist);

    lua_getglobal(lua.get(), "string");
    ApplyWhitelist(lua.get(), string_whitelist);
    
    lua_getglobal(lua.get(), "table");
    ApplyWhitelist(lua.get(), table_whitelist);

    return lua;
}

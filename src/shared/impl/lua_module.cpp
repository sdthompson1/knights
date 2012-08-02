/*
 * lua_module.cpp
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

#include "lua_func_wrapper.hpp"
#include "lua_load_from_rstream.hpp"
#include "lua_module.hpp"
#include "rstream.hpp"

#include "lua.hpp"

namespace {

    int DoFile(lua_State *lua)
    {
        // This is essentially a Lua wrapper around LuaExecFromRStream.

        // Expect 1 string arg (file name to load)
        // Return a variable number of results (the chunk return values).

        const char * filename = luaL_checkstring(lua, 1);

        // empty the stack
        lua_settop(lua, 0);

        // execute the file (convert the lua string directly to a path.)
        LuaExecRStream(lua, filename, 0, LUA_MULTRET, 
                       true);   // look in both cwd & root dir

        // return the actual no of results.
        return lua_gettop(lua);
    }

    std::string ModNameToFilename(const char *modname)
    {
        // module "a.b.c" converts to "server/a/b/c/init.lua".
        
        std::string result = "server/";
        for (const char *p = modname; *p != 0; ++p) {
            if (*p == '.') result += '/';
            else result += *p;
        }
        result += "/init.lua";
        return result;
    }        

    int Require(lua_State *lua)
    {
        // This is a cut-down version of the standard lua require function, ll_require.

        const char *name = luaL_checkstring(lua, 1);

        lua_settop(lua, 1);   // [name]
        lua_getfield(lua, LUA_REGISTRYINDEX, "_LOADED");   // [name _LOADED]
        lua_getfield(lua, 2, name);   // [name _LOADED _LOADED[name]]

        if (lua_toboolean(lua, -1)) {
            // package is already loaded
            return 1;
        }

        // else must load package

        lua_pop(lua, 1);  // [name _LOADED]

        const std::string filename = ModNameToFilename(name);
        
        // we must pass module name and file name as arguments.
        lua_pushstring(lua, name);                 // [name _LOADED name]
        lua_pushstring(lua, filename.c_str());     // [name _LOADED name filename]

        // Run the module.
        LuaExecRStream(lua, filename, 2, 1, 
            false);  // look in 'server' dir only (not cwd)

        // [name _LOADED result]

        if (!lua_isnil(lua, -1)) {
            lua_setfield(lua, 2, name);   // [name _LOADED] and set _LOADED[name] = result
        }

        lua_getfield(lua, 2, name);   // [name _LOADED _LOADED[name]]

        if (lua_isnil(lua, -1)) {
            // module did not set a value in _LOADED[name]
            lua_pushboolean(lua, 1);   // [name _LOADED nil true]
            lua_pushvalue(lua, -1);    // [name _LOADED nil true true]
            lua_setfield(lua, 2, name);  // [name _LOADED nil true] and set _LOADED[name]=true
        }

        return 1;   // Return the contents of _LOADED[name].
    }

    //
    // "module" function
    // Note this is considerably changed from the Lua 5.1 version.
    //

    int PrivIndex(lua_State *lua)
    {
        // args: _, k
        // upvalues: M, Parent

        // this function does not throw C++ exceptions
        // (although it might throw Lua errors).
        // therefore, it is safe to use with lua_pushcclosure
        // w/o the PushCClosure wrapper.

        ASSERT(lua_gettop(lua) == 2);
        
        // local x = M[k]
        lua_pushvalue(lua, 2);  // [_ k k]
        lua_gettable(lua, lua_upvalueindex(1));   // [_ k x]

        // if x == nil then x = Parent[k] end
        if (lua_isnil(lua, -1)) {
            lua_pop(lua, 1);                         // [_ k]
            lua_gettable(lua, lua_upvalueindex(2));  // [_ val]
        }
        
        // return x
        return 1;
    }
    
    int Module(lua_State *lua)
    {
        /* This does the rough equivalent of the following lua code:

              local Parent = _ENV

              local M = {}
              package.loaded[...] = M
           
              local Priv = {}
              local PrivMeta = {}
              function PrivMeta.__index(_,k)
                 local x = M[k]
                 if x ~= nil then
                    return x
                 else
                    return Parent[k]
                 end
              end
              PrivMeta.__newindex = M
              setmetatable(Priv, PrivMeta)

           And it also sets _ENV in the parent frame to Priv.
        */

        // fetch "..." argument
        const char *name = luaL_checkstring(lua, 1);

        // local Parent = _ENV
        lua_Debug ar;
        if (lua_getstack(lua, 1, &ar) == 0 ||
          lua_getinfo(lua, "f", &ar) == 0 ||       // [caller]
          lua_iscfunction(lua, -1)) {
            luaL_error(lua, "'module' called from C");
        }
        const char *upval = lua_getupvalue(lua, -1, 1);  // [caller _ENV]
        if (!name || std::strcmp(upval, "_ENV") != 0) {
            luaL_error(lua, "'module': bad upvalue");
        }

        // local M = {}
        lua_newtable(lua);     // [f Parent M]

        // package.loaded[...] = M
        lua_getfield(lua, LUA_REGISTRYINDEX, "_LOADED");   // [f Parent M _LOADED]
        lua_pushvalue(lua, -2);   // [f Parent M _LOADED M] 
        lua_setfield(lua, -2, name);  // [f Parent M _LOADED]
        lua_pop(lua, 1);  // [f Parent M]

        // local Priv = {}
        // local PrivMeta = {}
        lua_newtable(lua);
        lua_createtable(lua, 0, 2);  // [f Parent M Priv PrivMeta]

        // function PrivMeta.__index
        // (Don't use PushCFunction, we need maximum efficiency here)
        lua_pushvalue(lua, -3);               // [f Parent M Priv PrivMeta M]
        lua_pushvalue(lua, -5);               // [f Parent M Priv PrivMeta M Parent]
        lua_pushcclosure(lua, &PrivIndex, 2); // [f Parent M Priv PrivMeta __index]
        lua_setfield(lua, -2, "__index");     // [f Parent M Priv PrivMeta]

        // PrivMeta.__newindex = M
        lua_pushvalue(lua, -3);               // [f Parent M Priv PrivMeta M]
        lua_setfield(lua, -2, "__newindex");  // [f Parent M Priv PrivMeta]

        // setmetatable(Priv, PrivMeta)
        lua_setmetatable(lua, -2);            // [f Parent M Priv]

        // Now set _ENV in the parent (f) to Priv
        lua_setupvalue(lua, -4, 1);           // [f Parent M]

        return 0;
    }
}

void AddModuleFuncs(lua_State *lua)
{
    // Global "dofile" function
    PushCFunction(lua, &DoFile);
    lua_setglobal(lua, "dofile");

    // Global "require" function
    PushCFunction(lua, &Require);
    lua_setglobal(lua, "require");

    // Global "module" function
    PushCFunction(lua, &Module);
    lua_setglobal(lua, "module");

    // "package" table
    lua_createtable(lua, 0, 1);  // [{}]
    luaL_getsubtable(lua, LUA_REGISTRYINDEX, "_LOADED");   // [{} {}]  (second tbl is also in registry as _LOADED)
    lua_setfield(lua, -2, "loaded");    // [{loaded={}}]
    lua_setglobal(lua, "package");      // []
}

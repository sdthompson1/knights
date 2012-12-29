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

#include <cstring>

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
        /* This is equivalent to the following Lua code:

             function PrivMeta.__index(_, k)
                local x = M[k]     -- M is the first (and only) upvalue
                if x ~= nil then
                   return x
                else
                   return _G[k]    -- _G is the "real" global table (LUA_RIDX_GLOBALS)
                end
             end
        */

        // NOTE: This function does not throw C++ exceptions (although
        // it might throw Lua errors). Therefore, it is safe to use
        // with lua_pushcclosure w/o the PushCClosure wrapper.

        ASSERT(lua_gettop(lua) == 2);

        // local x = M[k]
        lua_pushvalue(lua, 2);      // [_ k k]
        lua_gettable(lua, lua_upvalueindex(1));   // [_ k x]

        // if x ~= nil
        if (!lua_isnil(lua, -1)) {

            // return x
            return 1;

        } else {
            // return _G[k]
            lua_rawgeti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);    // [_ k x G]
            lua_pushvalue(lua, 2);   // [_ k x G k]
            lua_gettable(lua, -2);    // [_ k x G[k]]
            return 1;
        }
    }

    int Module(lua_State *lua)
    {
        /* This does the rough equivalent of the following lua code:

              local M = {}
              package.loaded[...] = M

              local Priv = {}
              local PrivMeta = {
                 __index = <See above>,
                 __newindex = M
              }
              setmetatable(Priv, PrivMeta)

           And it also sets _ENV in the parent frame to Priv.

           NOTE: The reason to use a proxy table "Priv", instead of
           setting _ENV to M directly, is to ensure that global
           symbols (e.g. "print") do not appear in the module table.
           [e.g. local M = require("somemodule"); assert(M.print == nil); ]
        */

        // fetch "..." argument
        const char *name = luaL_checkstring(lua, 1);
        if (!name) {
            luaL_error(lua, "'module': first argument must be module name");
        }

        // find the parent call frame
        lua_Debug ar;
        if (lua_getstack(lua, 1, &ar) == 0 ||
          lua_getinfo(lua, "f", &ar) == 0 ||       // [caller]
          lua_iscfunction(lua, -1)) {
            luaL_error(lua, "'module' called from C");
        }

        // local M = {}
        lua_newtable(lua);     // [f M]

        // package.loaded[...] = M
        lua_getfield(lua, LUA_REGISTRYINDEX, "_LOADED");   // [f M _LOADED]
        lua_pushvalue(lua, -2);       // [f M _LOADED M] 
        lua_setfield(lua, -2, name);  // [f M _LOADED]
        lua_pop(lua, 1);              // [f M]

        // local Priv = {}
        // local PrivMeta = {}
        lua_newtable(lua);
        lua_createtable(lua, 0, 2);  // [f M Priv PrivMeta]

        // function PrivMeta.__index
        // (Don't use PushCFunction, we need maximum efficiency here)
        lua_pushvalue(lua, -3);               // [f M Priv PrivMeta M]
        lua_pushcclosure(lua, &PrivIndex, 1); // [f M Priv PrivMeta __index]
        lua_setfield(lua, -2, "__index");     // [f M Priv PrivMeta]

        // PrivMeta.__newindex = M
        lua_pushvalue(lua, -3);                  // [f M Priv PrivMeta M]
        lua_setfield(lua, -2, "__newindex");     // [f M Priv PrivMeta]

        // setmetatable(Priv, PrivMeta)
        lua_setmetatable(lua, -2);            // [f M Priv]

        // Now set _ENV in the parent (f) to Priv
        lua_setupvalue(lua, -3, 1);           // [f M]

        return 0;
    }

    
    //
    // Strict Checking of Globals (#203)
    //

    int StrictIndex(lua_State *lua)
    {
        /* Equivalent Lua code:

              local v = OldEnv[k]
              if v ~= nil then
                 return v
              elseif Declared[k] then
                 return nil
              else
                 error("Variable '" .. k .. "' is not declared")
              end

           Note: OldEnv = upvalue #1
                 Declared = upvalue #2
        */

        // [t k]
        
        // local v = OldEnv[k]
        lua_pushvalue(lua, 2);    // [t k k]
        lua_gettable(lua, lua_upvalueindex(1));   // [t k v]

        // if v ~= nil then
        if (!lua_isnil(lua, -1)) {

            // return v
            return 1;

        } else {
            // if Declared[k] then
            lua_pushvalue(lua, 2);    // [t k v k]
            lua_rawget(lua, lua_upvalueindex(2));  // [t k v declared]
            if (!lua_isnil(lua, -1)) {

                // return nil
                lua_pushnil(lua);    // [t k v declared nil]
                return 1;

            } else {
                // error
                return luaL_error(lua,
                                  "Variable '%s' is not declared",
                                  lua_tostring(lua, 2));
            }
        }
    }

    int StrictNewIndex(lua_State *lua)
    {
        /* Equivalent Lua code:
           
              if not Declared[k] then
                 if not Called_From_Top_Level() then
                    error("Assignment to undeclared variable '" .. k .. "'")
                 end
                 Declared[k] = true
              end
              OldEnv[k] = v
        */

        // [t k v]
        
        // if not Declared[k] then
        lua_pushvalue(lua, 2);   // [t k v k]
        lua_rawget(lua, lua_upvalueindex(2));    // [t k v declared]
        if (lua_isnil(lua, -1)) {

            // if not Called_From_Top_Level() then
            lua_Debug ar;
            const int getstack_ok = lua_getstack(lua, 1, &ar);
            if (!getstack_ok) {
                return luaL_error(lua, "STRICT: lua_getstack failed");
            }
            const int getinfo_ok = lua_getinfo(lua, "S", &ar);
            if (!getinfo_ok) {
                return luaL_error(lua, "STRICT: lua_getinfo failed");
            }
            const bool called_from_top_level = (ar.what[0] == 'm');
            if (!called_from_top_level) {

                // error
                return luaL_error(lua,
                                  "Assignment to undeclared variable '%s'",
                                  lua_tostring(lua, 2));
            }

            // Declared[k] = true
            lua_pushvalue(lua, 2);        // [t k v declared k]
            lua_pushboolean(lua, true);   // [t k v declared k true]
            lua_rawset(lua, lua_upvalueindex(2));    // [t k v declared]
        }

        // OldEnv[k] = v
        lua_pop(lua, 1);    // [t k v]
        lua_settable(lua, lua_upvalueindex(1));   // [t]
        return 0;
    }

    int UseStrict(lua_State *lua)
    {
        /* Equivalent Lua code:

              local OldEnv = _ENV
              local Declared = {}
              local Proxy = {}
              local Meta = {
                 __index = <See above>,
                 __newindex = <See above>
              }
              setmetatable(Proxy, Meta)

           Then: set _ENV in parent frame to Proxy
        */

        // local Parent = _ENV
        lua_Debug ar;
        if (lua_getstack(lua, 1, &ar) == 0 ||
          lua_getinfo(lua, "f", &ar) == 0 ||       // [caller]
          lua_iscfunction(lua, -1)) {
            luaL_error(lua, "use_strict: called from C");
        }
        const char *upval = lua_getupvalue(lua, -1, 1);  // [caller OldEnv]
        if (!upval || std::strcmp(upval, "_ENV") != 0) {
            luaL_error(lua, "use_strict: bad upvalue");
        }

        // local Declared = {}
        // local Proxy = {}
        // local Meta = {}
        lua_newtable(lua);              // [f OldEnv Declared]
        lua_createtable(lua, 0, 0);     // [f OldEnv Declared Proxy]
        lua_createtable(lua, 0, 2);     // [f OldEnv Declared Proxy Meta]

        // set Meta.__index
        lua_pushvalue(lua, -4);         // [f OldEnv Declared Proxy Meta OldEnv]
        lua_pushvalue(lua, -4);         // [f OldEnv Declared Proxy Meta OldEnv Declared]
        lua_pushcclosure(lua, &StrictIndex, 2);  // [f OldEnv Declared Proxy Meta __index]
        lua_setfield(lua, -2, "__index");  // [f OldEnv Declared Proxy Meta]

        // set Meta.__newindex
        lua_pushvalue(lua, -4);         // [f OldEnv Declared Proxy Meta OldEnv]
        lua_pushvalue(lua, -4);         // [f OldEnv Declared Proxy Meta OldEnv Declared]
        lua_pushcclosure(lua, &StrictNewIndex, 2);  // [f OldEnv Declared Proxy Meta __newindex]
        lua_setfield(lua, -2, "__newindex");  // [f OldEnv Declared Proxy Meta]

        // setmetatable(Proxy, Meta)
        lua_setmetatable(lua, -2);      // [f OldEnv Declared Proxy]

        // set _ENV of parent to Proxy
        lua_setupvalue(lua, -4, 1);     // [f OldEnv Declared]

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

    // Global "use_strict" function
    PushCFunction(lua, &UseStrict);
    lua_setglobal(lua, "use_strict");

    // "package" table
    lua_createtable(lua, 0, 1);  // [{}]
    luaL_getsubtable(lua, LUA_REGISTRYINDEX, "_LOADED");   // [{} {}]  (second tbl is also in registry as _LOADED)
    lua_setfield(lua, -2, "loaded");    // [{loaded={}}]
    lua_setglobal(lua, "package");      // []
}

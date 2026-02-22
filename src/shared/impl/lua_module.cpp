/*
 * lua_module.cpp
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

#include "lua_func_wrapper.hpp"
#include "lua_load_from_rstream.hpp"
#include "lua_module.hpp"

#include "include_lua.hpp"

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
                       true,    // look in both cwd & root dir
                       true);   // use dofile namespace proposal

        // return the actual no of results.
        return lua_gettop(lua);
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

    // Global "use_strict" function
    PushCFunction(lua, &UseStrict);
    lua_setglobal(lua, "use_strict");
}

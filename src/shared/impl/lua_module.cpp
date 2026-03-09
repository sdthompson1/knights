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

#include <algorithm>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

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
    //
    // mod.* API
    //

    static bool ParseVersion(const std::string &s, std::vector<int> &out, std::string &err)
    {
        if (s.empty()) { err = "version string is empty"; return false; }
        out.clear();
        std::istringstream ss(s);
        std::string token;
        while (std::getline(ss, token, '.')) {
            if (token.empty()) { err = "empty component in version string"; return false; }
            for (char c : token) {
                if (c < '0' || c > '9') { err = "non-digit in version component"; return false; }
            }
            out.push_back(std::stoi(token));
        }
        return true;
    }

    static bool VersionGe(const std::string &a_str, const std::string &b_str)
    {
        std::vector<int> a, b;
        std::string err;
        if (!ParseVersion(a_str, a, err)) return false;
        if (!ParseVersion(b_str, b, err)) return false;
        const size_t n = std::max(a.size(), b.size());
        for (size_t i = 0; i < n; ++i) {
            int ai = (i < a.size()) ? a[i] : 0;
            int bi = (i < b.size()) ? b[i] : 0;
            if (ai < bi) return false;
            if (ai > bi) return true;
        }
        return true;
    }

    int Mod_RegisterMod(lua_State *L)
    {
        luaL_checktype(L, 1, LUA_TTABLE);

        // Get name (required string)
        lua_getfield(L, 1, "name");
        if (!lua_isstring(L, -1))
            return luaL_error(L, "mod.RegisterMod: 'name' must be a string");
        const std::string name = lua_tostring(L, -1);
        lua_pop(L, 1);

        // Check _MODULE_PREFIX — must be set (i.e. we're inside the loading loop)
        lua_getglobal(L, "_MODULE_PREFIX");
        if (lua_isnil(L, -1))
            return luaL_error(L, "mod.RegisterMod: called outside module loading sequence");
        const std::string prefix = lua_tostring(L, -1);
        lua_pop(L, 1);

        // Check not already registered
        lua_getfield(L, LUA_REGISTRYINDEX, "_MOD_REGISTRY");  // [settings, registry]
        lua_getfield(L, -1, name.c_str());
        if (!lua_isnil(L, -1))
            return luaL_error(L, "mod.RegisterMod: mod '%s' is already registered", name.c_str());
        lua_pop(L, 1);  // [settings, registry]

        // Validate version if present
        lua_getfield(L, 1, "version");
        std::string version_str;
        if (lua_isstring(L, -1)) {
            version_str = lua_tostring(L, -1);
            std::vector<int> parsed;
            std::string err;
            if (!ParseVersion(version_str, parsed, err))
                return luaL_error(L, "mod.RegisterMod: invalid version '%s': %s",
                                  version_str.c_str(), err.c_str());
        } else if (!lua_isnil(L, -1)) {
            return luaL_error(L, "mod.RegisterMod: 'version' must be a string if present");
        }
        lua_pop(L, 1);  // [settings, registry]

        // Create ns = {} with __index = _G
        lua_newtable(L);  // [settings, registry, ns]
        lua_createtable(L, 0, 1);
        lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
        lua_setfield(L, -2, "__index");
        lua_setmetatable(L, -2);  // [settings, registry, ns]

        const int ns_abs = lua_gettop(L);

        // Store entry in _MOD_REGISTRY[name] = {version, prefix, ns}
        lua_newtable(L);  // [settings, registry, ns, entry]
        lua_pushstring(L, version_str.c_str());
        lua_setfield(L, -2, "version");
        lua_pushstring(L, prefix.c_str());
        lua_setfield(L, -2, "prefix");
        lua_pushvalue(L, ns_abs);
        lua_setfield(L, -2, "ns");
        // registry is at ns_abs - 1
        lua_setfield(L, ns_abs - 1, name.c_str());  // pops entry
        // [settings, registry, ns]

        // Replace caller's _ENV with ns (unless keep_env is true)
        lua_getfield(L, 1, "keep_env");
        const bool keep_env = lua_toboolean(L, -1);
        lua_pop(L, 1);  // [settings, registry, ns]

        if (!keep_env) {
            lua_Debug ar;
            if (!lua_getstack(L, 1, &ar) || !lua_getinfo(L, "f", &ar))
                return luaL_error(L, "mod.RegisterMod: cannot get caller info");
            // [settings, registry, ns, caller]
            if (lua_iscfunction(L, -1)) {
                lua_pop(L, 1);
                return luaL_error(L, "mod.RegisterMod: cannot be called from C");
            }
            const char *upname = lua_getupvalue(L, -1, 1);
            // [settings, registry, ns, caller, upval]
            if (!upname || std::strcmp(upname, "_ENV") != 0)
                return luaL_error(L, "mod.RegisterMod: caller's first upvalue is not '_ENV'");
            lua_pop(L, 1);  // discard old upval; [settings, registry, ns, caller]
            lua_pushvalue(L, ns_abs);  // [settings, registry, ns, caller, ns]
            lua_setupvalue(L, -2, 1); // pops ns, sets as upvalue 1; [settings, registry, ns, caller]
            lua_pop(L, 1);  // [settings, registry, ns]
        }

        // Return ns
        return 1;
    }

    int Mod_IsModRegistered(lua_State *L)
    {
        if (!lua_isstring(L, 1))
            return luaL_error(L, "mod.IsModRegistered: arg 1 must be a string");
        lua_getfield(L, LUA_REGISTRYINDEX, "_MOD_REGISTRY");
        lua_getfield(L, -1, lua_tostring(L, 1));
        lua_pushboolean(L, !lua_isnil(L, -1));
        return 1;
    }

    int Mod_GetRegisteredMod(lua_State *L)
    {
        if (!lua_isstring(L, 1))
            return luaL_error(L, "mod.GetRegisteredMod: arg 1 must be a string");
        const std::string name = lua_tostring(L, 1);

        // Read and validate min_version before touching the stack
        std::string min_ver;
        const bool has_min_ver = !lua_isnoneornil(L, 2);
        if (has_min_ver) {
            if (!lua_isstring(L, 2))
                return luaL_error(L, "mod.GetRegisteredMod: min_version must be a string");
            min_ver = lua_tostring(L, 2);
            std::vector<int> parsed;
            std::string err;
            if (!ParseVersion(min_ver, parsed, err))
                return luaL_error(L, "mod.GetRegisteredMod: invalid min_version '%s': %s",
                                  min_ver.c_str(), err.c_str());
        }

        lua_getfield(L, LUA_REGISTRYINDEX, "_MOD_REGISTRY");
        lua_getfield(L, -1, name.c_str());
        if (lua_isnil(L, -1)) {
            lua_pushnil(L);
            return 1;
        }

        if (has_min_ver) {
            lua_getfield(L, -1, "version");
            const std::string mod_ver = lua_isstring(L, -1) ? lua_tostring(L, -1) : "";
            lua_pop(L, 1);
            if (!VersionGe(mod_ver, min_ver)) {
                lua_pushnil(L);
                return 1;
            }
        }

        lua_getfield(L, -1, "ns");
        return 1;
    }

    int Mod_GetModVersion(lua_State *L)
    {
        if (!lua_isstring(L, 1))
            return luaL_error(L, "mod.GetModVersion: arg 1 must be a string");
        lua_getfield(L, LUA_REGISTRYINDEX, "_MOD_REGISTRY");
        lua_getfield(L, -1, lua_tostring(L, 1));
        if (lua_isnil(L, -1)) {
            lua_pushnil(L);
            return 1;
        }
        lua_getfield(L, -1, "version");
        return 1;
    }

    int Mod_GetModPrefix(lua_State *L)
    {
        if (!lua_isstring(L, 1))
            return luaL_error(L, "mod.GetModPrefix: arg 1 must be a string");
        lua_getfield(L, LUA_REGISTRYINDEX, "_MOD_REGISTRY");
        lua_getfield(L, -1, lua_tostring(L, 1));
        if (lua_isnil(L, -1)) {
            lua_pushnil(L);
            return 1;
        }
        lua_getfield(L, -1, "prefix");
        return 1;
    }

    int Require(lua_State *L)
    {
        if (!lua_isstring(L, 1))
            return luaL_error(L, "require: arg 1 must be a string");
        const std::string name = lua_tostring(L, 1);
        lua_getfield(L, LUA_REGISTRYINDEX, "_MOD_REGISTRY");
        lua_getfield(L, -1, name.c_str());
        if (lua_isnil(L, -1))
            return luaL_error(L, "require: mod '%s' is not registered", name.c_str());
        lua_getfield(L, -1, "ns");
        return 1;
    }

    int Mod_GetAllMods(lua_State *L)
    {
        lua_getfield(L, LUA_REGISTRYINDEX, "_MOD_REGISTRY");  // [registry]
        lua_newtable(L);  // [registry, result]
        lua_pushnil(L);
        while (lua_next(L, -3)) {
            // [registry, result, key, entry]
            lua_pushvalue(L, -2);  // key
            lua_getfield(L, -2, "version");  // version string
            lua_settable(L, -5);  // result[key] = version
            lua_pop(L, 1);  // pop entry
        }
        return 1;
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

    // Create _MOD_REGISTRY in the Lua registry
    lua_newtable(lua);
    lua_setfield(lua, LUA_REGISTRYINDEX, "_MOD_REGISTRY");

    // Create and register the "mod" global table
    lua_newtable(lua);  // [mod]

    lua_pushcfunction(lua, &Mod_RegisterMod);
    lua_setfield(lua, -2, "RegisterMod");

    lua_pushcfunction(lua, &Mod_IsModRegistered);
    lua_setfield(lua, -2, "IsModRegistered");

    lua_pushcfunction(lua, &Mod_GetRegisteredMod);
    lua_setfield(lua, -2, "GetRegisteredMod");

    lua_pushcfunction(lua, &Mod_GetModVersion);
    lua_setfield(lua, -2, "GetModVersion");

    lua_pushcfunction(lua, &Mod_GetModPrefix);
    lua_setfield(lua, -2, "GetModPrefix");

    lua_pushcfunction(lua, &Mod_GetAllMods);
    lua_setfield(lua, -2, "GetAllMods");

    lua_setglobal(lua, "mod");

    // Global "require" function
    PushCFunction(lua, &Require);
    lua_setglobal(lua, "require");
}

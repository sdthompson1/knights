/*
 * lua_userdata.hpp
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
#include "lua_userdata.hpp"
#include "map_support.hpp"

namespace {
    
    enum LuaPtrType {
        PTR_COLLECTED,   // Set when the userdata has been garbage collected.
        PTR_RAW,
        PTR_SHARED,
        PTR_WEAK
    };

    // Note: Userdata policy.
    //  -- Full userdatas are always a memory block of type UserData, or one of 
    //     its subclasses.
    //  -- Light userdatas can be more or less anything depending on context. However, if a 
    //     light userdata is used as a registry key, we insist that it be a valid 
    //     address in the C++ address space (this is to avoid clashes between different
    //     "users" of the registry).
    struct UserData {
        unsigned short int tag;
        unsigned short int ptr_type;
    };

    struct RawUserData : UserData {
        void *ptr;
    };

    struct SharedUserData : UserData {
        boost::shared_ptr<void> ptr;
    };

    struct WeakUserData : UserData {
        boost::weak_ptr<void> ptr;
    };

    // The __gc method for a Knights userdata object
    int GCMethod(lua_State *lua)
    {
        UserData *ud = static_cast<UserData*>(lua_touserdata(lua, 1));
        if (!ud) {
            return luaL_error(lua, "__gc: Not a valid userdata object");
        }

        switch (ud->ptr_type) {
        case PTR_COLLECTED:
            return luaL_error(lua, "__gc: Object already collected");
        case PTR_SHARED:
            {
                SharedUserData *ud2 = static_cast<SharedUserData*>(ud);
                ud2->ptr.boost::shared_ptr<void>::~shared_ptr();
            }
            break;
        case PTR_WEAK:
            {
                WeakUserData *ud2 = static_cast<WeakUserData*>(ud);
                ud2->ptr.boost::weak_ptr<void>::~weak_ptr();
            }
            break;
        }

        ud->ptr_type = PTR_COLLECTED;
        return 0;
    }

    // Pushes the metatable for Knights userdata onto the stack.
    // (The table is created the first time this routine is called.)
    // Note the registry key KTS_UDMETA is used for this.
    void SetUserdataMetatable(lua_State *lua, int index)
    {
        const int created = luaL_newmetatable(lua, "KTS_UDMETA"); // Pushes the metatable.
        if (created) {
            // Set the __gc method in the metatable.
            lua_pushstring(lua, "__gc");
            PushCFunction(lua, &GCMethod);
            lua_settable(lua, -3);
        }
        // Associate the metatable with the given userdata object.
        if (index < 0) --index; // because the metatable has been added to stack!
        lua_setmetatable(lua, index);
    }
}
    
void NewLuaPtr_Impl(lua_State *lua, void *ptr, LuaTag tag)
{
    if (ptr) {
        RawUserData * mem = static_cast<RawUserData*>(lua_newuserdata(lua, sizeof(RawUserData)));
        mem->tag = tag;
        mem->ptr_type = PTR_RAW;
        mem->ptr = ptr;
        // Note: no need to set a metatable because no garbage collection is needed.
    } else {
        lua_pushnil(lua);
    }
}

void NewLuaSharedPtr_Impl(lua_State *lua, boost::shared_ptr<void> ptr, LuaTag tag)
{
    if (ptr) {
        SharedUserData * mem = static_cast<SharedUserData*>(lua_newuserdata(lua, sizeof(SharedUserData)));
        mem->tag = tag;
        mem->ptr_type = PTR_SHARED;
        new (&mem->ptr) boost::shared_ptr<void>(ptr);
        SetUserdataMetatable(lua, -1);
    } else {
        lua_pushnil(lua);
    }
}

void NewLuaWeakPtr_Impl(lua_State *lua, boost::weak_ptr<void> ptr, LuaTag tag)
{
    if (ptr.lock()) {
        WeakUserData * mem = static_cast<WeakUserData*>(lua_newuserdata(lua, sizeof(WeakUserData)));
        mem->tag = tag;
        mem->ptr_type = PTR_WEAK;
        new (&mem->ptr) boost::weak_ptr<void>(ptr);
        SetUserdataMetatable(lua, -1);
    } else {
        lua_pushnil(lua);
    }
}

void * ReadLuaPtr_Impl(lua_State *lua, int index, LuaTag expected_tag)
{
    UserData * mem = static_cast<UserData*>(lua_touserdata(lua, index));
    if (!mem || mem->tag != expected_tag) {
        return 0;
    } else if (mem->ptr_type == PTR_RAW) {
        RawUserData *ud = static_cast<RawUserData*>(mem);
        return ud->ptr;
    } else if (mem->ptr_type == PTR_SHARED) {
        SharedUserData *ud = static_cast<SharedUserData*>(mem);
        return ud->ptr.get();
    } else {
        // This probably indicates a bug in Knights
        luaL_error(lua, "Can't convert weak pointer to raw pointer");
        return 0;
    }
}

boost::shared_ptr<void> ReadLuaSharedPtr_Impl(lua_State *lua, int index, LuaTag expected_tag)
{
    UserData *mem = static_cast<UserData*>(lua_touserdata(lua, index));
    if (!mem || mem->tag != expected_tag) {
        return boost::shared_ptr<void>();
    } else if (mem->ptr_type == PTR_SHARED) {
        SharedUserData *ud = static_cast<SharedUserData*>(mem);
        return ud->ptr;
    } else if (mem->ptr_type == PTR_WEAK) {
        WeakUserData *ud = static_cast<WeakUserData*>(mem);
        return ud->ptr.lock();
    } else {
        // This probably indicates a bug in Knights
        luaL_error(lua, "Can't convert raw pointer to shared pointer");
        return boost::shared_ptr<void>();
    }
}

boost::weak_ptr<void> ReadLuaWeakPtr_Impl(lua_State *lua, int index, LuaTag expected_tag)
{
    UserData *mem = static_cast<UserData*>(lua_touserdata(lua, index));
    if (!mem || mem->tag != expected_tag) {
        return boost::weak_ptr<void>();
    } else if (mem->ptr_type == PTR_WEAK) {
        WeakUserData *ud = static_cast<WeakUserData*>(mem);
        return ud->ptr;
    } else {
        // This probably indicates a bug in Knights
        luaL_error(lua, "Can't convert shared or raw pointer to weak pointer");
        return boost::weak_ptr<void>();
    }
}

void PushMapCoord(lua_State *lua, const MapCoord &mc)
{
    if (mc.isNull()) {
        lua_pushnil(lua);
    } else {
        lua_createtable(lua, 0, 2);  // [tbl]
        lua_pushinteger(lua, mc.getX());  // [tbl x]
        lua_setfield(lua, -2, "x");  // [tbl]
        lua_pushinteger(lua, mc.getY());  // [tbl y]
        lua_setfield(lua, -2, "y");  // [tbl]
    }
}

void PushMapDirection(lua_State *lua, MapDirection dir)
{
    switch (dir) {
    case D_NORTH: lua_pushstring(lua, "north"); break;
    case D_EAST: lua_pushstring(lua, "east"); break;
    case D_SOUTH: lua_pushstring(lua, "south"); break;
    case D_WEST: lua_pushstring(lua, "west"); break;
    default: lua_pushnil(lua); break;
    }
}

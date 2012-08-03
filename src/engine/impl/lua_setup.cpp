/*
 * lua_setup.cpp
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

#include "control.hpp"
#include "create_monster_type.hpp"
#include "create_tile.hpp"
#include "graphic.hpp"
#include "knights_config_impl.hpp"
#include "load_segments.hpp"
#include "lua_check.hpp"
#include "lua_exec.hpp"
#include "lua_func_wrapper.hpp"
#include "lua_ref.hpp"
#include "lua_setup.hpp"
#include "lua_userdata.hpp"
#include "my_exceptions.hpp"
#include "segment.hpp"
#include "tile.hpp"

#include "lua.hpp"

#include <string>

namespace {

    //
    // Lua Config Functions.
    //

    int MakeAnim(lua_State *lua)
    {
        KnightsConfigImpl * kc = GetKC(lua, "Anims");
        Anim * anim = kc->addLuaAnim(lua, 1);
        NewLuaPtr<Anim>(lua, anim);
        return 1;
    }
    
    // Upvalue: KnightsConfigImpl*
    // Input: Table representing a new Control
    // Return value: Control* (full userdata, raw ptr)
    int MakeControl(lua_State *lua)
    {
        KnightsConfigImpl * kc = GetKC(lua, "Controls");

        LuaFunc action(lua, 1, "action");
        LuaFunc possible(lua, 1, "possible");
        bool can_do_while_moving = LuaGetBool(lua, 1, "can_do_while_moving", false);
        bool can_do_while_stunned = LuaGetBool(lua, 1, "can_do_while_stunned", false);
        
        int action_bar_slot = LuaGetInt(lua, 1, "action_bar_slot", -1);
        int tap_priority = LuaGetInt(lua, 1, "tap_priority", 0);
        int action_bar_priority = LuaGetInt(lua, 1, "action_bar_priority", tap_priority);
        bool continuous = LuaGetBool(lua, 1, "continuous", false);
        MapDirection menu_direction = LuaGetMapDirection(lua, 1, "menu_direction", D_NORTH);
        const Graphic * menu_icon = LuaGetPtr<Graphic>(lua, 1, "menu_icon");
        unsigned int menu_special = static_cast<unsigned int>(LuaGetInt(lua, 1, "menu_special", 0));
        std::string name = LuaGetString(lua, 1, "name");
        bool suicide_key = LuaGetBool(lua, 1, "suicide_key", false);

        auto_ptr<Control> ctrl(new Control(lua, 1,
                                           menu_icon, menu_direction, tap_priority, action_bar_slot,
                                           action_bar_priority, suicide_key, continuous, menu_special,
                                           name,
                                           action, possible, can_do_while_moving, can_do_while_stunned));

        Control *c = kc->addLuaControl(ctrl); // transfers ownership (also sets control's ID)
        NewLuaPtr<Control>(lua, c);
        return 1;
    }

    // Upvalue: KnightsConfigImpl*
    // Input: n args representing a new graphic (stack posns 1 to lua_gettop)
    // Output: userdata representing the graphic
    int MakeGraphic(lua_State *lua)
    {
        KnightsConfigImpl *kc = GetKC(lua, "Graphics");
        std::auto_ptr<Graphic> gfx(CreateGraphicFromLua(lua));
        NewLuaPtr<Graphic>(lua, gfx.get());
        kc->addLuaGraphic(gfx);
        return 1;
    }
    
    // Upvalue: KnightsConfigImpl* (light userdata)
    // Input arguments: Table representing a new ItemType
    // Return values: ItemType* (full userdata, raw ptr)
    int MakeItemType(lua_State *lua)
    {
        KnightsConfigImpl * kc = GetKC(lua, "ItemTypes");
        auto_ptr<ItemType> it(new ItemType(lua, 1));
        ItemType * lua_item_type = kc->addLuaItemType(it); // transfers ownership.
        NewLuaPtr<ItemType>(lua, lua_item_type);
        return 1;
    }

    // Upvalue: KnightsConfigImpl*
    // Input: table representing an Overlay
    // Output: userdata representing an Overlay
    int MakeOverlay(lua_State *lua)
    {
        KnightsConfigImpl *kc = GetKC(lua, "Overlays");

        // catch case where they call as Overlay(a,b,c,d) instead of 
        // Overlay{a,b,c,d}...
        if (lua_gettop(lua) != 1) {
            luaL_error(lua, "kts.Overlay expects exactly one argument (a table)");
        }

        auto_ptr<Overlay> result(new Overlay(lua, 1));
        
        // expects a table of four entries to be in stack position 1 (top of stack).
        for (int i = 0; i < 4; ++i) {
            lua_pushinteger(lua, i+1);
            lua_gettable(lua, 1);
            Graphic * graphic = ReadLuaPtr<Graphic>(lua, -1);
            lua_pop(lua, 1);
            result->setRawGraphic(MapDirection(i), graphic);
        }

        Overlay *r = kc->addLuaOverlay(result);  // transfers ownership
        NewLuaPtr<Overlay>(lua, r);
        return 1;
    }

    // Upvalue: KnightsConfigImpl*
    // Input: table representing a monstertype
    // Output: userdata representing a monstertype
    int MakeMonsterType(lua_State *lua)
    {
        KnightsConfigImpl *kc = GetKC(lua, "MonsterTypes");
        MonsterType *m = CreateMonsterType(lua, kc);
        NewLuaPtr<MonsterType>(lua, m);
        return 1;
    }

    // Upvalue: KnightsConfigImpl*
    // Input: table of tiles, and segments-filename (relative to _CWD), and category string (temporary)
    // Output: table (sequence) of segment-userdatas.
    int MakeSegments(lua_State *lua)
    {
        KnightsConfigImpl *kc = GetKC(lua, "Segments");        
        const char *filename = luaL_checkstring(lua, 2);
        const char *category = luaL_checkstring(lua, 3);

        boost::filesystem::path cwd;
        lua_getglobal(lua, "_CWD");  // [.. cwd]
        const char *p = lua_tostring(lua, -1);
        if (p) {
            cwd = p;
        }
        lua_pop(lua, 1); // [..]

        lua_pushvalue(lua, 1);  // [.. tiletable]

        LoadSegments(lua, kc, filename, cwd, category);  // [.. segments]
        
        return 1;
    }        

    // Upvalue: KnightsConfigImpl*
    // Input: filename
    // Output: userdata representing the sound
    int MakeSound(lua_State *lua)
    {
        KnightsConfigImpl *kc = GetKC(lua, "Sounds");
        const char *filename = luaL_checkstring(lua, 1);

        lua_getglobal(lua, "_CWD"); // [_CWD]
        const char *cwd = lua_tostring(lua, -1);
        Sound * sound = kc->addLuaSound(FileInfo(filename, cwd));
        lua_pop(lua, 1);  // []

        NewLuaPtr<Sound>(lua, sound);  // [snd]
        return 1;
    }

    // Upvalue: KnightsConfigImpl*
    // Input: table representing a tile
    // Output: userdata representing the tile
    int MakeTile(lua_State *lua)
    {
        // note: no need to store Tiles in the KnightsConfigImpl.
        // They are shared_ptrs and will be automatically released when the lua state is closed down.
        KnightsConfigImpl *kc = GetKC(lua, "tiles");
        boost::shared_ptr<Tile> tile = CreateTile(lua, kc);
        NewLuaSharedPtr<Tile>(lua, tile);
        return 1;
    }

    // Upvalue: KnightsConfigImpl*
    // Input: 60 arguments for the overlay offsets
    // Output: nothing
    int SetOverlayOffsets(lua_State *lua)
    {
        KnightsConfigImpl *kc = GetKC(lua, "overlay offsets");
        kc->setOverlayOffsets(lua);
        return 0;
    }

    // Upvalue: none
    // Input: zero or more Tiles
    // Output: nothing
    int SetRotate(lua_State *lua)
    {
        boost::shared_ptr<Tile> first = ReadLuaSharedPtr<Tile>(lua, 1);
        boost::shared_ptr<Tile> prev = first;
        for (int i = 2; i <= lua_gettop(lua); ++i) {
            boost::shared_ptr<Tile> here = ReadLuaSharedPtr<Tile>(lua, i);
            if (prev && here) prev->setRotate(here);
            prev = here;
        }
        if (prev && first) prev->setRotate(first);
        return 0;
    }

    // Upvalue: none
    // Input: two Tiles
    // Output: nothing
    int SetReflect(lua_State *lua)
    {
        boost::shared_ptr<Tile> t1 = ReadLuaSharedPtr<Tile>(lua, 1);
        boost::shared_ptr<Tile> t2 = ReadLuaSharedPtr<Tile>(lua, 2);
        if (t1 && t2) {
            t1->setReflect(t2);
            t2->setReflect(t1);
        }
        return 0;
    }
}

void AddLuaConfigFunctions(lua_State *lua, KnightsConfigImpl *kc)
{
    // all functions go in "kts" table.
    lua_rawgeti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);   // [env]
    luaL_getsubtable(lua, -1, "kts");                        // [env kts]

    
    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeAnim, 1);
    lua_setfield(lua, -2, "Anim");

    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeControl, 1);
    lua_setfield(lua, -2, "Control");

    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeGraphic, 1);
    lua_setfield(lua, -2, "Graphic");
    
    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeItemType, 1);
    lua_setfield(lua, -2, "ItemType");

    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeOverlay, 1);
    lua_setfield(lua, -2, "Overlay");

    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeMonsterType, 1);
    lua_setfield(lua, -2, "MonsterType");
    
    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeSound, 1);
    lua_setfield(lua, -2, "Sound");

    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeTile, 1);
    lua_setfield(lua, -2, "Tile");


    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeSegments, 1);
    lua_setfield(lua, -2, "LoadSegments");
    

    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &SetOverlayOffsets, 1);
    lua_setfield(lua, -2, "SetOverlayOffsets");


    PushCFunction(lua, &SetRotate);
    lua_setfield(lua, -2, "SetRotate");

    PushCFunction(lua, &SetReflect);
    lua_setfield(lua, -2, "SetReflect");
    

    lua_pop(lua, 2); // []
}


// Helper functions for accessing values from a lua table
// (assumed to be at top of stack).
    
bool LuaGetBool(lua_State *lua, int idx, const char * key, bool dflt)
{
    lua_getfield(lua, idx, key);
    bool result;
    if (lua_isnil(lua, -1)) {
        result = dflt;
    } else {
        result = lua_toboolean(lua, -1) != 0;
    }
    lua_pop(lua, 1);
    return result;
}

int LuaGetInt(lua_State *lua, int idx, const char * key, int dflt)
{
    lua_getfield(lua, idx, key);
    int result;
    if (lua_isnil(lua, -1)) {
        result = dflt;
    } else {
        result = lua_tointeger(lua, -1);
    }
    lua_pop(lua, 1);
    return result;
}

float LuaGetFloat(lua_State *lua, int idx, const char *key, float dflt)
{
    lua_getfield(lua, idx, key);
    float result;
    if (lua_isnil(lua, -1)) {
        result = dflt;
    } else {
        result = static_cast<float>(lua_tonumber(lua, -1));
    }
    lua_pop(lua, 1);
    return result;
}

float LuaGetProbability(lua_State *lua, int idx, const char *key, float dflt)
{
    float p = LuaGetFloat(lua, idx, key, dflt);
    if (p < 0.0f || p > 1.0f) {
        luaL_error(lua, "'%s' must be between 0 and 1", key);
    }
    return p;
}

std::string LuaGetString(lua_State *lua, int idx, const char *key, const char * dflt)
{
    lua_getfield(lua, idx, key);
    std::string result;
    if (lua_isnil(lua, -1)) {
        result = dflt;
    } else if (!lua_isstring(lua, -1)) {
        luaL_error(lua, "'%s' must be a string", key);
    } else {
        result = lua_tostring(lua, -1);
    }
    lua_pop(lua, 1);
    return result;
}

namespace {
    ItemSize ToItemSize(const std::string &s, ItemSize dflt)
    {
        if (s == "held") return IS_BIG;
        else if (s == "backpack") return IS_SMALL;
        else if (s == "magic") return IS_MAGIC;
        else if (s == "nopickup") return IS_NOPICKUP;
        else return dflt;
    }        
}

ItemSize LuaGetItemSize(lua_State *lua, int idx, const char *key, ItemSize dflt)
{
    std::string s = LuaGetString(lua, idx, key);
    return ToItemSize(s, dflt);
}

ItemSize LuaPopItemSize(lua_State *lua, ItemSize dflt)
{
    ItemSize result;
    if (lua_isstring(lua, -1)) {
        result = ToItemSize(lua_tostring(lua, -1), dflt);
    } else {
        result = dflt;
    }
    lua_pop(lua, 1);
    return result;
}

MapDirection LuaGetMapDirection(lua_State *lua, int idx, const char *key, MapDirection dflt)
{
    lua_getfield(lua, idx, key);
    MapDirection result = dflt;
    if (lua_isstring(lua, -1)) {
        result = GetMapDirection(lua, -1);
    }
    lua_pop(lua, 1);
    return result;
}

RandomInt LuaGetRandomInt(lua_State *lua, int idx, const char *key)
{
    // Returns a RandomInt (defaults to zero).
    lua_getfield(lua, idx, key);  // [.. function/number]
    return LuaPopRandomInt(lua, key);  // [..]
}
    
RandomInt LuaPopRandomInt(lua_State *lua, const char *key)
{
    if (lua_isnil(lua, -1)) {
        lua_pop(lua, 1);
        return RandomInt();
    } else if (!lua_isfunction(lua, -1) && !lua_isnumber(lua, -1)) {
        luaL_error(lua, "'%s': expected function or number, got %s", key,
                   lua_typename(lua, lua_type(lua, -1)));
        return RandomInt(); // prevent compiler warning
    } else {
        return RandomInt(lua);  // pops function or number
    }
}

KnightsConfigImpl * GetKC(lua_State *lua, const char * msg)
{
    KnightsConfigImpl * kc = static_cast<KnightsConfigImpl*>(lua_touserdata(lua, lua_upvalueindex(2)));
    if (!kc->doingConfig()) {
        luaL_error(lua, (std::string("Cannot create new ") + msg + " during the game").c_str());
    }
    return kc;
}

void LuaGetTileList(lua_State *lua, int tbl_idx, const char *key, std::vector<boost::shared_ptr<Tile> > &tiles)
{
    tiles.clear();
    lua_getfield(lua, tbl_idx, key);  // [... tbl]
    LuaPopTileList(lua, tiles);
}

void LuaPopTileList(lua_State *lua, std::vector<boost::shared_ptr<Tile> > &tiles)
{
    if (!lua_isnil(lua, -1)) {
        lua_len(lua, -1); // [... tbl len]
        const int sz = lua_tointeger(lua, -1);
        lua_pop(lua, 1);   // [... tbl]
        tiles.clear();
        tiles.reserve(sz);
        for (int i = 1; i <= sz; ++i) {
            lua_pushinteger(lua, i);   // [... tbl i]
            lua_gettable(lua, -2);     // [... tbl tile]
            tiles.push_back(ReadLuaSharedPtr<Tile>(lua, -1));
            lua_pop(lua, 1);  // [... tbl]
        }
    }

    lua_pop(lua, 1);  // [...]
}

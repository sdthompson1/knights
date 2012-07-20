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

#include "action.hpp"
#include "control.hpp"
#include "create_tile.hpp"
#include "graphic.hpp"
#include "knights_config_impl.hpp"
#include "lua_check.hpp"
#include "lua_exec.hpp"
#include "lua_func_wrapper.hpp"
#include "lua_ref.hpp"
#include "lua_setup.hpp"
#include "lua_userdata.hpp"
#include "tile.hpp"

#include "lua.hpp"

#include <string>

namespace {

    // Class that calls Lua to generate a random integer.
    class LuaRandomInt : public KConfig::RandomInt {
    public:
        explicit LuaRandomInt(lua_State *lua);  // pops a lua function.
        virtual int get() const;  // overridden from base class

    private:
        LuaRef lua_func_ref;
    };

    LuaRandomInt::LuaRandomInt(lua_State *lua)
    {
        if (!LuaIsCallable(lua, -1) && !lua_isnumber(lua, -1)) {
            throw LuaError("Value is not a random int");
        }
        lua_func_ref.reset(lua); // pops function/number.
    }

    int LuaRandomInt::get() const
    {
        lua_State *lua = lua_func_ref.getLuaState();
        ASSERT(lua);  // should have been set in the ctor

        lua_func_ref.push(lua);  // pushes function (or number)

        if (LuaIsCallable(lua, -1)) {
            LuaExec(lua, 0, 1);  // pops function and pushes number
        }
        // else: there should be a number on top of stack already.
        const int result = lua_tointeger(lua, -1);
        lua_pop(lua, 1);  // pop the result
        return result;
    }
    

    // Macro used by some LuaGetXXX functions
    
#define LUA_GET(lua, idx, key, dflt, T, GET)     \
    lua_getfield(lua, idx, key);                 \
    T result;                            \
    if (lua_isnil(lua, -1)) {            \
        result = dflt;                   \
    } else {                             \
        result = GET(lua, -1);           \
    }                                    \
    lua_pop(lua, 1);                     \
    return result                       

    
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

        const Action * action = LuaGetAction(lua, 1, "action", kc);
        int action_bar_slot = LuaGetInt(lua, 1, "action_bar_slot", -1);
        int tap_priority = LuaGetInt(lua, 1, "tap_priority", 0);
        int action_bar_priority = LuaGetInt(lua, 1, "action_bar_priority", tap_priority);
        bool continuous = LuaGetBool(lua, 1, "continuous", false);
        MapDirection menu_direction = LuaGetMapDirection(lua, 1, "menu_direction", D_NORTH);
        const Graphic * menu_icon = LuaGetPtr<Graphic>(lua, 1, "menu_icon");
        unsigned int menu_special = static_cast<unsigned int>(LuaGetInt(lua, 1, "menu_special", 0));
        std::string name = LuaGetString(lua, 1, "name");
        bool suicide_key = LuaGetBool(lua, 1, "suicide_key", false);

        auto_ptr<Control> ctrl(new Control(lua, -1,
                                           menu_icon, menu_direction, tap_priority, action_bar_slot,
                                           action_bar_priority, suicide_key, continuous, menu_special,
                                           name, action));

        Control *c = kc->addLuaControl(ctrl); // transfers ownership (also sets control's ID)
        NewLuaPtr<Control>(lua, c);
        return 1;
    }


    // Upvalue: KnightsConfigImpl*
    // Input: Table representing a dungeon layout (Name+Function)
    // Return value: RandomDungeonLayout* (as full userdata)
    int MakeDungeonLayout(lua_State *lua)
    {
        KnightsConfigImpl *kc = GetKC(lua, "DungeonLayouts");
        RandomDungeonLayout *dlay = kc->addLuaDungeonLayout(lua);
        NewLuaPtr<RandomDungeonLayout>(lua, dlay);
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
    
    // Upvalue: KnightsConfigImpl*
    // Input: one function or callable object
    // Output: userdata representing the ItemGenerator
    int MakeItemGenerator(lua_State *lua)
    {
        KnightsConfigImpl *kc = GetKC(lua, "ItemGenerators");
        ItemGenerator *ig = kc->addLuaItemGenerator(lua);  // reads from arg position 1
        NewLuaPtr<ItemGenerator>(lua, ig);  // pushes it to lua stack
        return 1;
    }
    
    // Upvalue: KnightsConfigImpl* (light userdata)
    // Input arguments: Table representing a new ItemType
    // Return values: ItemType* (full userdata, raw ptr)
    int MakeItemType(lua_State *lua)
    {
        KnightsConfigImpl * kc = GetKC(lua, "ItemTypes");
        auto_ptr<ItemType> it(new ItemType(lua, 1, kc));
        const ItemType * lua_item_type = kc->addLuaItemType(it); // transfers ownership.
        NewLuaPtr<const ItemType>(lua, lua_item_type);
        return 1;
    }

    // Upvalue: KnightsConfigImpl*
    // Input: table representing an Overlay
    // Output: userdata representing an Overlay
    int MakeOverlay(lua_State *lua)
    {
        KnightsConfigImpl *kc = GetKC(lua, "Overlays");

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
    // Input: filename
    // Output: userdata representing the sound
    int MakeSound(lua_State *lua)
    {
        KnightsConfigImpl *kc = GetKC(lua, "Sounds");
        const char *filename = luaL_checkstring(lua, 1);
        Sound * sound = kc->addLuaSound(filename);
        NewLuaPtr<Sound>(lua, sound);
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
    
    // Upvalue: KnightsConfigImpl*
    // Input: one string (kconfig variable name)
    // Output: userdata representing a control
    int MakeKconfigControl(lua_State *lua)
    {
        KnightsConfigImpl *kc = GetKC(lua, "Controls");
        const char * name = luaL_checkstring(lua, 1);
        kc->kconfigControl(name);
        return 1;
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
    PushCClosure(lua, &MakeDungeonLayout, 1);
    lua_setfield(lua, -2, "DungeonLayout");
    
    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeGraphic, 1);
    lua_setfield(lua, -2, "Graphic");

    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeItemGenerator, 1);
    lua_setfield(lua, -2, "ItemGenerator");
    
    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeItemType, 1);
    lua_setfield(lua, -2, "ItemType");

    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeOverlay, 1);
    lua_setfield(lua, -2, "Overlay");

    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeSound, 1);
    lua_setfield(lua, -2, "Sound");

    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeTile, 1);
    lua_setfield(lua, -2, "Tile");
    

    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &SetOverlayOffsets, 1);
    lua_setfield(lua, -2, "SetOverlayOffsets");


    PushCFunction(lua, &SetRotate);
    lua_setfield(lua, -2, "SetRotate");

    PushCFunction(lua, &SetReflect);
    lua_setfield(lua, -2, "SetReflect");
    

    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeKconfigControl, 1);
    lua_setfield(lua, -2, "kconfig_control");

    
    lua_pop(lua, 2); // []
}


// Helper functions for accessing values from a lua table
// (assumed to be at top of stack).
    
bool LuaGetBool(lua_State *lua, int idx, const char * key, bool dflt)
{
    LUA_GET(lua, idx, key, dflt, bool, lua_toboolean);
}

int LuaGetInt(lua_State *lua, int idx, const char * key, int dflt)
{
    LUA_GET(lua, idx, key, dflt, int, lua_tointeger);
}

float LuaGetFloat(lua_State *lua, int idx, const char *key, float dflt)
{
    LUA_GET(lua, idx, key, dflt, float, lua_tonumber);
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

ItemSize LuaGetItemSize(lua_State *lua, int idx, const char *key, ItemSize dflt)
{
    std::string s = LuaGetString(lua, idx, key);
    if (s == "held") return IS_BIG;
    else if (s == "backpack") return IS_SMALL;
    else if (s == "magic") return IS_MAGIC;
    else if (s == "nopickup") return IS_NOPICKUP;
    else return dflt;
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

const KConfig::RandomInt * LuaGetRandomInt(lua_State *lua, int idx, const char *key, KnightsConfigImpl *kc)
{
    // Pops Lua nil, function or number; returns a RandomInt* (can be null).

    lua_getfield(lua, idx, key);  // [.. function/number]

    LuaRandomInt *result = 0;

    if (lua_isnil(lua, -1)) {
        lua_pop(lua, 1);
    } else if (!lua_isfunction(lua, -1) && !lua_isnumber(lua, -1)) {
        luaL_error(lua, "'%s': expected function or number, got %s", key,
                   lua_typename(lua, lua_type(lua, -1)));
    } else {
        result = new LuaRandomInt(lua);   // pops function/number
        kc->getRandomIntContainer().add(result);  // transfers ownership
    }

    return result;
}

KnightsConfigImpl * GetKC(lua_State *lua, const char * msg)
{
    KnightsConfigImpl * kc = static_cast<KnightsConfigImpl*>(lua_touserdata(lua, lua_upvalueindex(2)));
    if (!kc->doingConfig()) {
        luaL_error(lua, (std::string("Cannot create new ") + msg + " during the game").c_str());
    }
    return kc;
}

Action * LuaGetAction(lua_State *lua, int idx, const char *key, KnightsConfigImpl *kc)
{
    // This builds a new LuaAction object from the function at given
    // key in table at top of stack, and returns it. (The object will
    // be added to KnightsConfigImpl for deletion when
    // ~KnightsConfigImpl is called.)

    lua_getfield(lua, idx, key); // function is now at top of stack

    Action *ac = 0;
    if (lua_isnil(lua, -1)) {
        lua_pop(lua, 1);
    } else if (!LuaIsCallable(lua, -1)) {
        luaL_error(lua, "'%s': expected function or callable object, got %s", key,
                   lua_typename(lua, lua_type(lua, -1)));
    } else {
        auto_ptr<Action> action(new LuaAction(lua));  // pops stack
        ac = kc->addLuaAction(action);  // transfers ownership
    }
    return ac;
}

Action * LuaGetAction(lua_State *lua, int idx, KnightsConfigImpl *kc)
{
    // Same but reads from given stack idx instead
    if (lua_isnil(lua, idx)) {
        return 0;
    } else if (!LuaIsCallable(lua, idx)) {
        luaL_error(lua, "expected function or callable object");
        return 0;  // prevent compiler warning
    } else {
        lua_pushvalue(lua, idx);
        auto_ptr<Action> action(new LuaAction(lua));  // pops stack
        return kc->addLuaAction(action);  // transfers ownership
    }
}

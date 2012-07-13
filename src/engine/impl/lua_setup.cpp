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

#include "graphic.hpp"
#include "knights_config_impl.hpp"
#include "lua_setup.hpp"
#include "lua_userdata.hpp"

#include "lua.hpp"

#include <string>

namespace {

    // Helper functions for accessing values from a lua table
    // (assumed to be at top of stack).
    
#define LUA_GET(lua, key, dflt, T, GET)  \
    lua_pushstring(lua, key);            \
    lua_gettable(lua, -2);               \
    T result;                            \
    if (lua_isnil(lua, -1)) {            \
        result = dflt;                   \
    } else {                             \
        result = GET(lua, -1);           \
    }                                    \
    lua_pop(lua, 1);                     \
    return result                       
    
    bool LuaGetBool(lua_State *lua, const char * key, bool dflt = false)
    {
        LUA_GET(lua, key, dflt, bool, lua_toboolean);
    }

    int LuaGetInt(lua_State *lua, const char * key, int dflt = 0)
    {
        LUA_GET(lua, key, dflt, int, lua_tointeger);
    }

    float LuaGetFloat(lua_State *lua, const char *key, float dflt = 0.0f)
    {
        LUA_GET(lua, key, dflt, float, lua_tonumber);
    }

    std::string LuaGetString(lua_State *lua, const char *key, const char * dflt = "")
    {
        LUA_GET(lua, key, dflt, std::string, lua_tostring);
    }

    template<class T>
    Graphic * LuaGetPtr(lua_State *lua, KnightsConfigImpl *kc, const char *key)
    {
        lua_pushstring(lua, key);
        lua_gettable(lua, -2);
        T * result = ReadLuaPtr<T>(lua, -1);
        lua_pop(lua, 1);
        return result;
    }

    ItemSize LuaGetItemSize(lua_State *lua, const char *key, ItemSize dflt = IS_NOPICKUP)
    {
        std::string s = LuaGetString(lua, key);
        if (s == "held") return IS_BIG;
        else if (s == "backpack") return IS_SMALL;
        else if (s == "magic") return IS_MAGIC;
        else if (s == "nopickup") return IS_NOPICKUP;
        else return dflt;
    }


    KnightsConfigImpl * GetKC(lua_State *lua, const char * msg)
    {
        KnightsConfigImpl * kc = static_cast<KnightsConfigImpl*>(lua_touserdata(lua, lua_upvalueindex(1)));
        if (!kc->doingConfig()) {
            luaL_error(lua, (std::string("Cannot create new ") + msg + " during the game").c_str());
        }
        return kc;
    }
        

    //
    // Lua Config Functions.
    //
    

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
    

    /*
    // Upvalue: KnightsConfigImpl* (light userdata)
    // Input arguments: Table representing a new ItemType
    // Return values: ItemType* (light userdata)
    int MakeItemType(lua_State *lua)
    {
        KnightsConfigImpl * kc = GetKC(lua, "ItemTypes");
        
        const bool allow_strength = LuaGetBool(lua, "allow_strength", true);

        const ItemType * ammo = LuaGetItemType(lua, kc, "ammo");

        Graphic * backpack_graphic = LuaGetGraphic(lua, kc, "backpack_graphic");
        Graphic * backpack_overdraw = LuaGetGraphic(lua, kc, "backpack_overdraw");
        int backpack_slot = LuaGetInt(lua, "backpack_slot");

        bool can_throw = LuaGetBool(lua, "can_throw");

        Control * control = LuaGetControl(lua, kc, "control");

        bool fragile = LuaGetBool(lua, "fragile");

        Graphic * graphic = LuaGetGraphic(lua, kc, "graphic");

        int key = LuaGetInt(lua, "key");

        int max_stack = LuaGetInt(lua, "max_stack", 1);

        const Action * melee_action = LuaGetAction(lua, kc, "melee_action");
        int melee_backswing_time = LuaGetInt(lua, "melee_backswing_time");
        const RandomInt * melee_damage = LuaGetRandomInt(lua, kc, "melee_damage");
        int melee_downswing_time = LuaGetInt(lua, "melee_downswing_time");
        const RandomInt * melee_stun_time = LuaGetRandomInt(lua, kc, "melee_stun_time");
        const RandomInt * melee_tile_damage = LuaGetRandomInt(lua, kc, "melee_tile_damage");

        int missile_access_chance = LuaGetInt(lua, "missile_access_chance");
        const Anim * missile_anim = LuaGetAnim(lua, kc, "missile_anim");
        int missile_backswing_time = LuaGetInt(lua, "missile_backswing_time");
        const RandomInt * missile_damage = LuaGetRandomInt(lua, kc, "missile_damage");
        int missile_downswing_time = LuaGetInt(lua, "missile_downswing_time");
        int missile_hit_multiplier = LuaGetInt(lua, "missile_hit_multiplier", 1);
        int missile_range = LuaGetInt(lua, "missile_range");
        int missile_speed = LuaGetInt(lua, "missile_speed");
        const RandomInt * missile_stun_time = LuaGetRandomInt(lua, kc, "missile_stun_time");

        std::string name = LuaGetString(lua, "name");

        Action *on_drop = LuaGetAction(lua, kc, "on_drop");
        Action *on_hit = LuaGetAction(lua, kc, "on_hit");
        Action *on_pick_up = LuaGetAction(lua, kc, "on_pick_up");
        Action *on_walk_over = LuaGetAction(lua, kc, "on_walk_over");

        bool open_traps = LuaGetBool(lua, "open_traps");

        Overlay * overlay = LuaGetOverlay(lua, kc, "overlay");

        int parry_chance = LuaGetInt(lua, "parry_chance");

        const Action * reload_action = LuaGetAction(lua, kc, "reload_action");
        int reload_action_time = LuaGetInt(lua, "reload_action_time", 250);
        int reload_time = LuaGetInt(lua, "reload_time");

        Graphic * stack_graphic = LuaGetGraphic(lua, kc, "stack_graphic");
        if (!stack_graphic) stack_graphic = graphic;
        
        int tutorial_key = LuaGetInt(lua, "tutorial");

        ItemSize item_size = LuaGetItemSize(lua, "type", IS_NOPICKUP);

        auto_ptr<ItemType> it(new ItemType);
        it->construct(graphic, stack_graphic, backpack_graphic, backpack_overdraw, overlay, item_size, max_stack,
                      backpack_slot, fragile,
                      melee_backswing_time, melee_downswing_time, melee_damage,
                      melee_stun_time, melee_tile_damage, melee_action, parry_chance, 
                      can_throw, missile_range, missile_speed, missile_access_chance,
                      missile_hit_multiplier, missile_backswing_time, missile_downswing_time,
                      missile_damage, missile_stun_time, missile_anim,
                      reload_time, ammo, reload_action, reload_action_time,
                      key, open_traps, control, on_pick_up, on_drop, on_walk_over, on_hit,
                      allow_strength, tutorial_key, name);
        
        void * lua_item_type = kc->addLuaItemType(it);

        lua_pushlightuserdata(lua, lua_item_type);
        return 1;
    }
    */
    
    // Upvalue: KnightsConfigImpl*
    // Input: one string (kconfig variable name)
    // Output: userdata representing the itemtype
    int MakeKconfigItemType(lua_State *lua)
    {
        KnightsConfigImpl *kc = GetKC(lua, "ItemTypes");
        const char * name = luaL_checkstring(lua, 1);
        kc->kconfigItemType(name);
        return 1;
    }

    // Upvalue: KnightsConfigImpl*
    // Input: one string (kconfig variable name)
    // Output: userdata representing the tiletype
    int MakeKconfigTile(lua_State *lua)
    {
        KnightsConfigImpl *kc = GetKC(lua, "TileTypes");
        const char * name = luaL_checkstring(lua, 1);
        kc->kconfigTile(name);
        return 1;
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
    luaL_getsubtable(lua, -1, "kts");                         // [env kts]
    
    lua_pushlightuserdata(lua, kc);
    lua_pushcclosure(lua, &MakeGraphic, 1);
    lua_setfield(lua, -2, "Graphic");

    lua_pushlightuserdata(lua, kc);
    lua_pushcclosure(lua, &MakeSound, 1);
    lua_setfield(lua, -2, "Sound");

    /*
    lua_pushlightuserdata(lua, kc);
    lua_pushcclosure(lua, &MakeItemType, 1);
    lua_setfield(lua, -2, "ItemType");
    */
    
    lua_pushlightuserdata(lua, kc);
    lua_pushcclosure(lua, &MakeKconfigItemType, 1);
    lua_setfield(lua, -2, "kconfig_itemtype");

    lua_pushlightuserdata(lua, kc);
    lua_pushcclosure(lua, &MakeKconfigTile, 1);
    lua_setfield(lua, -2, "kconfig_tile");

    lua_pushlightuserdata(lua, kc);
    lua_pushcclosure(lua, &MakeKconfigControl, 1);
    lua_setfield(lua, -2, "kconfig_control");

    lua_pop(lua, 2); // []
}

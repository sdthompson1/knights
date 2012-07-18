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
#include "graphic.hpp"
#include "knights_config_impl.hpp"
#include "lua_exec.hpp"
#include "lua_func_wrapper.hpp"
#include "lua_setup.hpp"
#include "lua_userdata.hpp"

#include "lua.hpp"

#include <string>

namespace {

    // Class that calls Lua to generate a random integer.
    class LuaRandomInt : public KConfig::RandomInt {
    public:
        // Note: We store a raw pointer to the lua state, so clients
        // must destroy the LuaRandomInt BEFORE they destroy the LuaState.
        
        explicit LuaRandomInt(lua_State *lua_);  // pops a lua function.
        ~LuaRandomInt();
        virtual int get() const;  // overridden from base class

    private:
        lua_State *lua;
    };

    LuaRandomInt::LuaRandomInt(lua_State *lua_)
        : lua(lua_)
    {
        if (!lua_isfunction(lua, -1) && !lua_isnumber(lua, -1)) {
            throw LuaError("Value is not a random int");
        }
        lua_rawsetp(lua, LUA_REGISTRYINDEX, this); // pops function/number.
    }

    LuaRandomInt::~LuaRandomInt()
    {
        lua_pushnil(lua);
        lua_rawsetp(lua, LUA_REGISTRYINDEX, this);  // clear registry entry.
    }

    int LuaRandomInt::get() const
    {
        lua_rawgetp(lua, LUA_REGISTRYINDEX, this);
        if (lua_isfunction(lua, -1)) {
            LuaExec(lua, 0, 1);  // pops function and pushes number
        }
        // else: there should be a number on top of stack already.
        const int result = lua_tointeger(lua, -1);
        lua_pop(lua, 1);  // pop the result
        return result;
    }
    
    
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
    T * LuaGetPtr(lua_State *lua, KnightsConfigImpl *kc, const char *key)
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

    MapDirection LuaGetMapDirection(lua_State *lua, const char *key, MapDirection dflt = D_NORTH)
    {
        lua_pushstring(lua, key);
        lua_gettable(lua, -2);
        MapDirection result = dflt;
        if (lua_isstring(lua, -1)) {
            result = GetMapDirection(lua, -1);
        }
        lua_pop(lua, 1);
        return result;
    }

    const KConfig::RandomInt * LuaGetRandomInt(lua_State *lua, KnightsConfigImpl *kc, const char *key)
    {
        // Pops Lua nil, function or number; returns a RandomInt* (can be null).

        lua_pushstring(lua, key);    // [.. tbl key]
        lua_gettable(lua, -2);       // [.. tbl function/number]

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

    Action * LuaGetAction(lua_State *lua, KnightsConfigImpl *kc, const char *key)
    {
        // This builds a new LuaAction object from the function on the top of the stack,
        // and returns it. (The object will be added to KnightsConfigImpl for deletion
        // when ~KnightsConfigImpl is called.)
        
        lua_pushstring(lua, key);
        lua_gettable(lua, -2);   // function is now at top of stack

        Action *ac = 0;
        if (lua_isnil(lua, -1)) {
            lua_pop(lua, 1);
        } else if (!lua_isfunction(lua, -1)) {
            luaL_error(lua, "'%s': expected function, got %s", key,
                       lua_typename(lua, lua_type(lua, -1)));
        } else {
            auto_ptr<Action> action(new LuaAction(kc->getLuaState()));  // pops stack
            ac = kc->addLuaAction(action);  // transfers ownership
        }
        return ac;
    }
        

    //
    // Lua Config Functions.
    //

    // Upvalue: KnightsConfigImpl*
    // Input: Table representing a new Control
    // Return value: Control* (full userdata, raw ptr)
    int MakeControl(lua_State *lua)
    {
        KnightsConfigImpl * kc = GetKC(lua, "Controls");

        const Action * action = LuaGetAction(lua, kc, "action");
        int action_bar_slot = LuaGetInt(lua, "action_bar_slot", -1);
        int tap_priority = LuaGetInt(lua, "tap_priority", 0);
        int action_bar_priority = LuaGetInt(lua, "action_bar_priority", tap_priority);
        bool continuous = LuaGetBool(lua, "continuous", false);
        MapDirection menu_direction = LuaGetMapDirection(lua, "menu_direction", D_NORTH);
        const Graphic * menu_icon = LuaGetPtr<Graphic>(lua, kc, "menu_icon");
        unsigned int menu_special = static_cast<unsigned int>(LuaGetInt(lua, "menu_special", 0));
        std::string name = LuaGetString(lua, "name");
        bool suicide_key = LuaGetBool(lua, "suicide_key", false);

        auto_ptr<Control> ctrl(new Control(lua, -1,
                                           menu_icon, menu_direction, tap_priority, action_bar_slot,
                                           action_bar_priority, suicide_key, continuous, menu_special,
                                           name, action));

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
        
        const bool allow_strength = LuaGetBool(lua, "allow_strength", true);

        const ItemType * ammo = LuaGetPtr<const ItemType>(lua, kc, "ammo");

        Graphic * backpack_graphic = LuaGetPtr<Graphic>(lua, kc, "backpack_graphic");
        Graphic * backpack_overdraw = LuaGetPtr<Graphic>(lua, kc, "backpack_overdraw");
        int backpack_slot = LuaGetInt(lua, "backpack_slot");

        bool can_throw = LuaGetBool(lua, "can_throw");

        Control * control = LuaGetPtr<Control>(lua, kc, "control");

        bool fragile = LuaGetBool(lua, "fragile");

        Graphic * graphic = LuaGetPtr<Graphic>(lua, kc, "graphic");

        int key = LuaGetInt(lua, "key");

        int max_stack = LuaGetInt(lua, "max_stack", 1);

        const Action * melee_action = LuaGetAction(lua, kc, "melee_action");
        int melee_backswing_time = LuaGetInt(lua, "melee_backswing_time");
        const RandomInt * melee_damage = LuaGetRandomInt(lua, kc, "melee_damage");
        int melee_downswing_time = LuaGetInt(lua, "melee_downswing_time");
        const RandomInt * melee_stun_time = LuaGetRandomInt(lua, kc, "melee_stun_time");
        const RandomInt * melee_tile_damage = LuaGetRandomInt(lua, kc, "melee_tile_damage");

        float missile_access_chance = LuaGetFloat(lua, "missile_access_chance");
        const Anim * missile_anim = LuaGetPtr<Anim>(lua, kc, "missile_anim");
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

        Overlay * overlay = LuaGetPtr<Overlay>(lua, kc, "overlay");

        float parry_chance = LuaGetFloat(lua, "parry_chance");

        const Action * reload_action = LuaGetAction(lua, kc, "reload_action");
        int reload_action_time = LuaGetInt(lua, "reload_action_time", 250);
        int reload_time = LuaGetInt(lua, "reload_time");

        Graphic * stack_graphic = LuaGetPtr<Graphic>(lua, kc, "stack_graphic");
        if (!stack_graphic) stack_graphic = graphic;
        
        int tutorial_key = LuaGetInt(lua, "tutorial");

        ItemSize item_size = LuaGetItemSize(lua, "type", IS_NOPICKUP);

        auto_ptr<ItemType> it(new ItemType(lua, -1));
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
    // Input: one string (kconfig variable name)
    // Output: userdata representing the anim
    int MakeKConfigAnim(lua_State *lua)
    {
        KnightsConfigImpl *kc = GetKC(lua, "Anims");
        const char *name = luaL_checkstring(lua, 1);
        kc->kconfigAnim(name);
        return 1;
    }    
    
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

    // Upvalue: KnightsConfigImpl*
    // Input: one string (kconfig variable name)
    // Output: userdata representing an anim
    int MakeKconfigAnim(lua_State *lua)
    {
        KnightsConfigImpl *kc = GetKC(lua, "Anims");
        const char * name = luaL_checkstring(lua, 1);
        kc->kconfigAnim(name);
        return 1;
    }
}

void AddLuaConfigFunctions(lua_State *lua, KnightsConfigImpl *kc)
{
    // all functions go in "kts" table.
    lua_rawgeti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);   // [env]
    luaL_getsubtable(lua, -1, "kts");                        // [env kts]

    
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
    PushCClosure(lua, &MakeSound, 1);
    lua_setfield(lua, -2, "Sound");

    
    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeKconfigItemType, 1);
    lua_setfield(lua, -2, "kconfig_itemtype");

    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeKconfigTile, 1);
    lua_setfield(lua, -2, "kconfig_tile");

    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeKconfigControl, 1);
    lua_setfield(lua, -2, "kconfig_control");

    lua_pushlightuserdata(lua, kc);
    PushCClosure(lua, &MakeKconfigAnim, 1);
    lua_setfield(lua, -2, "kconfig_anim");

    
    lua_pop(lua, 2); // []
}

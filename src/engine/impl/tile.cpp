/*
 * tile.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2025.
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

#include "action_data.hpp"
#include "creature.hpp"
#include "dungeon_map.hpp"
#include "item.hpp"
#include "item_type.hpp"
#include "knights_callbacks.hpp"
#include "knights_config_impl.hpp"  // for getTileCategory
#include "lua_exec.hpp"
#include "lua_setup.hpp"
#include "lua_userdata.hpp"
#include "mediator.hpp"
#include "my_ctype.hpp"
#include "my_exceptions.hpp"
#include "sweep.hpp"
#include "tile.hpp"

#include "include_lua.hpp"

#include <cstring>

namespace {
    MapAccess PopAccess(lua_State *lua)
    {
        MapAccess result = A_BLOCKED;  // default
        
        if (!lua_isnil(lua, -1)) {
            const char *p = lua_tostring(lua, -1);
            
            // convert to uppercase
            std::string s(p ? p : "");
            for (std::string::iterator it = s.begin(); it != s.end(); ++it) *it = ToUpper(*it);

            if (s == "APPROACH" || s == "PARTIAL") result = A_APPROACH;
            else if (s == "BLOCKED") result = A_BLOCKED;
            else if (s == "CLEAR") result = A_CLEAR;
            else {
                luaL_error(lua,
                           "'%s' is not a valid access type; must be 'approach', 'partial', 'blocked' or 'clear'",
                           p);
            }
        }

        lua_pop(lua, 1);
        return result;
    }
}
            
            
//
// public functions
//

// constructs a Tile from a lua table, assumed to be at top of stack.
// note this ignores "type" and just constructs a plain Tile.
Tile::Tile(lua_State *lua)
    : hit_points(0)
{
    ASSERT(lua);   // [t]

    lua_pushvalue(lua, -1);  // [t t]
    table_ref.reset(lua);    // [t]

    // set default access
    for (int i = 0; i <= H_MISSILES; ++i) access[i] = A_CLEAR;

    // read the "access" field
    lua_getfield(lua, -1, "access");   // [t access-table]
    if (!lua_isnil(lua, -1)) {
        readAccess(lua);
    }
    lua_pop(lua, 1);  // [t]
    
    // read other fields
    connectivity_check = LuaGetInt(lua, -1, "connectivity_check"); // default 0

    lua_getfield(lua, -1, "control");   // [t control]
    if (IsLuaPtr<Control>(lua, -1)) {
        control = ReadLuaPtr<Control>(lua, -1);
        lua_pop(lua, 1);
    } else {
        // set it in the lua registry
        control = 0;
        control_func_ref.reset(lua);   // pops lua stack
    }

    depth = LuaGetInt(lua, -1, "depth");  // default 0
    graphic = LuaGetPtr<Graphic>(lua, -1, "graphic");  // default 0
    initial_hit_points = LuaGetRandomInt(lua, -1, "hit_points");  // default 0

    lua_getfield(lua, -1, "items");  // [t items]
    popItems(lua);                   // [t]

    on_activate.reset(lua, -1, "on_activate");
    on_approach.reset(lua, -1, "on_approach");
    on_destroy.reset(lua, -1, "on_destroy");
    on_hit.reset(lua, -1, "on_hit");
    on_walk_over.reset(lua, -1, "on_walk_over");
    on_withdraw.reset(lua, -1, "on_withdraw");

    lua_getfield(lua, -1, "stairs_down");
    readStairsDown(lua);
    lua_pop(lua, 1);

    tutorial_key = LuaGetInt(lua, -1, "tutorial");  // default 0

    lua_getfield(lua, -1, "map_as");
    popMapAs(lua);
}

void Tile::popMapAs(lua_State *lua)
{
    const char * map_as = lua_tostring(lua, -1);
    use_col = false;
    if (map_as) {
        if (std::strcmp(map_as, "wall") == 0) {
            use_col = true;
            col = COL_WALL;
        } else if (std::strcmp(map_as, "floor") == 0) {
            use_col = true;
            col = COL_FLOOR;
        }
    }
    lua_pop(lua, 1);
}

void Tile::popItems(lua_State *lua)
{
    if (lua_isnumber(lua, -1)) {
        if (lua_tointeger(lua, -1) == 0) {
            item_category = -2;   // blocked
        } else {
            item_category = -1;   // allowed
        }
    } else if (lua_isnil(lua, -1)) {
        // default (allow items for A_CLEAR, else block)
        if (access[H_WALKING] == A_CLEAR) item_category = -1;  // allowed
        else item_category = -2;  // blocked
    } else if (lua_isstring(lua, -1)
               && std::strcmp( lua_tostring(lua, -1), "destroy") == 0) {
        item_category = -3;  // destroy
    } else {
        item_category = GetTileCategory(lua, -1);
    }
    lua_pop(lua, 1);  // [t]

    if (item_category > -2) {
        items_mode = ALLOWED;
    } else if (item_category == -3) {
        items_mode = DESTROY;
    } else {
        items_mode = BLOCKED;
    }
}

void Tile::readAccess(lua_State *lua)
{
    lua_getfield(lua, -1, "flying");   // [at access]
    access[H_FLYING] = PopAccess(lua); // [at]

    lua_getfield(lua, -1, "missiles");
    access[H_MISSILES] = PopAccess(lua);
    
    lua_getfield(lua, -1, "walking");
    access[H_WALKING] = PopAccess(lua);
}    

void Tile::readStairsDown(lua_State *lua)
{
    if (lua_isnil(lua, -1)) {
        is_stair = stair_top = false;
    } else {
        const char *s = lua_tostring(lua, -1);
        if (s && std::strcmp(s, "special") == 0) {
            is_stair = false;
            stair_top = true;
        } else {
            is_stair = true;
            stair_top = false;
            stair_direction = GetMapDirection(lua, -1);
        }
    }
}

Tile::Tile(const LuaFunc &walk_over, const LuaFunc &activate)
    : hit_points(0), on_walk_over(walk_over), on_activate(activate)
{
    for (int i = 0; i <= H_MISSILES; ++i) access[i] = A_CLEAR;
    connectivity_check = 0;
    control = 0;
    depth = 0;
    graphic = 0;
    item_category = -1;  // items allowed
    items_mode = ALLOWED;
    is_stair = stair_top = false;
    tutorial_key = 0;
    use_col = false;
}

void Tile::newIndex(lua_State *lua)
{
    // [ud key val]
    if (!lua_isstring(lua, 2)) {
        return;
    }

    const std::string k = lua_tostring(lua, 2);

    if (k == "access") {
        if (lua_isnil(lua, 3)) {
            luaL_error(lua, "Cannot set 'access' to nil");
        } else {
            lua_pushvalue(lua, 3);
            readAccess(lua);
            lua_pop(lua, 1);
        }

    } else if (k == "connectivity_check") {
        connectivity_check = lua_tointeger(lua, 3);

    } else if (k == "control") {
        if (IsLuaPtr<Control>(lua, 3)) {
            control = ReadLuaPtr<Control>(lua, 3);
        } else {
            control = 0;
            lua_pushvalue(lua, 3);
            control_func_ref.reset(lua); // pops
        }

    } else if (k == "depth") {
        depth = lua_tointeger(lua, 3);

    } else if (k == "graphic") {
        graphic = ReadLuaPtr<Graphic>(lua, 3);

    } else if (k == "hit_points") {
        lua_pushvalue(lua, 3);
        initial_hit_points = LuaPopRandomInt(lua, "hit_points");

    } else if (k == "items") {
        lua_pushvalue(lua, 3);
        popItems(lua);

    } else if (k == "map_as") {
        lua_pushvalue(lua, 3);
        popMapAs(lua);
        
    } else if (k == "on_activate") {
        lua_pushvalue(lua, 3);
        on_activate = LuaFunc(lua); // pops

    } else if (k == "on_approach") {
        lua_pushvalue(lua, 3);
        on_approach = LuaFunc(lua);

    } else if (k == "on_destroy") {
        lua_pushvalue(lua, 3);
        on_destroy = LuaFunc(lua);

    } else if (k == "on_hit") {
        lua_pushvalue(lua, 3);
        on_hit = LuaFunc(lua);

    } else if (k == "on_walk_over") {
        lua_pushvalue(lua, 3);
        on_walk_over = LuaFunc(lua);

    } else if (k == "on_withdraw") {
        lua_pushvalue(lua, 3);
        on_withdraw = LuaFunc(lua);

    } else if (k == "stairs_down") {
        lua_pushvalue(lua, 3);
        readStairsDown(lua);
        lua_pop(lua, 1);

    } else if (k == "tutorial") {
        tutorial_key = lua_tointeger(lua, 3);

    }
}

shared_ptr<Tile> Tile::clone(bool force_copy)
{
    shared_ptr<Tile> new_tile = doClone(force_copy);
    new_tile->setHitPoints();

    // set 'original_tile'
    new_tile->original_tile = shared_from_this();  // creates weak_ptr to this.
    
    return new_tile;
}

shared_ptr<Tile> Tile::cloneWithNewGraphic(const Graphic *new_graphic)
{
    shared_ptr<Tile> new_tile = clone(true);  // we must force a copy because this is the only way we can sensibly change the graphic.
    new_tile->graphic = new_graphic;
    return new_tile;
}

shared_ptr<Tile> Tile::getOriginalTile() const
{
    return original_tile.lock();
}

void Tile::setHitPoints()
{
    // note: hit_points=0 (the default) means 'tile is indestructible'
    hit_points = initial_hit_points.get();
}

void Tile::damage(DungeonMap &dmap, const MapCoord &mc, int amount, shared_ptr<Creature> actor)
{
    // NB: hit_points <= 0 means that the tile is indestructible.
    if (hit_points > 0) {
        hit_points -= amount;
        if (hit_points <= 0) {
            // Tile has been destroyed
            shared_ptr<Tile> self(shared_from_this());
            dmap.rmTile(mc, self, actor->getOriginator());
            self->onDestroy(dmap, mc, actor, actor->getOriginator());
        }
    }
}

void Tile::onDestroy(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor, const Originator &originator)
{
    // Run on_destroy event
    if (on_destroy.hasValue()) {
        ActionData ad;
        ad.setActor(actor);
        ad.setOriginator(originator);
        ad.setTile(&dmap, mc, shared_from_this());
        ad.setGenericPos(&dmap, mc);
        on_destroy.execute(ad);
    }
    
    // Destroy any fragile items present
    shared_ptr<Item> item = dmap.getItem(mc);
    if (item && item->getType().isFragile()) dmap.rmItem(mc);
}

bool Tile::targettable() const
{
    // Current definition of targettable is that it is not A_CLEAR at H_WALKING,
    // *OR* that it has an on_hit action.
    // This rules out "floor" tiles (except the special pentagram which has on_hit),
    // but pretty much everything else is included.
    return (getAccess(H_WALKING) != A_CLEAR || on_hit.hasValue());
}

void Tile::onActivate(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor,
                      const Originator &originator, ActivateType act_type)
{
    // NB act_type isn't used here, but it is used in subclasses (in particular Lockable).
    if (on_activate.hasValue()) {
        ActionData ad;
        ad.setActor(actor);
        ad.setOriginator(originator);
        ad.setTile(&dmap, mc, shared_from_this());
        ad.setGenericPos(&dmap, mc);
        on_activate.execute(ad);
    }
}

void Tile::onWalkOver(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor, const Originator &originator)
{
    if (on_walk_over.hasValue() && actor && actor->getHeight() == H_WALKING) {
        ActionData ad;
        ad.setActor(actor);
        ad.setOriginator(originator);
        ad.setTile(&dmap, mc, shared_from_this());
        ad.setGenericPos(&dmap, mc);
        on_walk_over.execute(ad);
    }
}

void Tile::onApproach(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor, const Originator &originator)
{
    if (on_approach.hasValue()) {
        ActionData ad;
        ad.setActor(actor);
        ad.setOriginator(originator);
        ad.setTile(&dmap, mc, shared_from_this());
        ad.setGenericPos(&dmap, mc);
        on_approach.execute(ad);
    }
}

void Tile::onWithdraw(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor, const Originator &originator)
{
    if (on_withdraw.hasValue()) {
        ActionData ad;
        ad.setActor(actor);
        ad.setOriginator(originator);
        ad.setTile(&dmap, mc, shared_from_this());
        ad.setGenericPos(&dmap, mc);
        on_withdraw.execute(ad);
    }
}

void Tile::onHit(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor, const Originator &originator)
{
    if (on_hit.hasValue()) {
        ActionData ad;
        ad.setActor(actor);
        ad.setOriginator(originator);
        ad.setTile(&dmap, mc, shared_from_this());
        ad.setGenericPos(&dmap, mc);
        on_hit.execute(ad);
    }
}


//
// custom control
//

const Control * Tile::getControl(const MapCoord &pos) const
{
    if (control) {
        return control;
    } else {
        lua_State *lua = control_func_ref.getLuaState();
        if (!lua) {
            return 0;
        }

        control_func_ref.push(lua);   // [func]

        if (lua_isnil(lua, -1)) {
            lua_pop(lua, 1);  // []
            return 0;
        } else {
        
            PushMapCoord(lua, pos);   // [func pos]

            try {
                LuaExec(lua, 1, 1);  // resets stack to [] on exception
            } catch (const LuaError &e) {
                Mediator::instance().getCallbacks().gameMsgRaw(-1, UTF8String::fromUTF8Safe(e.what()), true);
                return 0;
            }

            // on successful execution stack is [return_val]
            if (!IsLuaPtr<Control>(lua, -1)) {
                Mediator::instance().getCallbacks().gameMsgRaw(-1,
                    UTF8String::fromUTF8("Tile 'control' function did not return a control!"), true);
                return 0;
            }

            const Control * ctrl = ReadLuaPtr<Control>(lua, -1);
            lua_pop(lua, 1);   // []
            return ctrl;
        }
    }
}

//
// reflect/rotate
//

shared_ptr<Tile> Tile::getReflect()
{
    shared_ptr<Tile> t = reflect.lock();
    if (t) return t;
    else return shared_from_this();
}

shared_ptr<Tile> Tile::getRotate()
{
    shared_ptr<Tile> t = rotate.lock();
    if (t) return t;
    else return shared_from_this();
}


//
// protected functions
//

shared_ptr<Tile> Tile::doClone(bool force_copy)
{
    ASSERT(typeid(*this)==typeid(Tile)); //doClone must be overridden in subclasses.

    const bool has_hit_points = 
        !initial_hit_points.isFixed()
        || initial_hit_points.get() != 0;

    // basic tiles are shared rather than cloned
    // however, if a tile has hit points, it must be copied. (and hit points must be rerolled, also.)
    if (has_hit_points || force_copy) {
        return shared_ptr<Tile>(new Tile(*this));
    } else {
        return shared_from_this();
    }
}

void Tile::setGraphic(DungeonMap *dmap, const MapCoord &mc, const Graphic *g,
                      shared_ptr<const ColourChange> ccnew)
{
    graphic = g;
    cc = ccnew;
    if (dmap) {
        // Graphic-change needs to be propagated to clients
        Mediator::instance().onChangeTile(*dmap, mc, *this);
    }
}

void Tile::setItemsAllowed(DungeonMap *dmap, const MapCoord &mc, bool allow, bool destroy) 
{
    setItemsAllowedNoSweep(allow, destroy);
    if (dmap && items_mode != ALLOWED) {
        // We don't need to tell Mediator about items-allowed changes.
        // However we do need to call SweepItems if items-allowed is now BLOCKED, or destroy
        // item if mode is DESTROY.
        if (items_mode == DESTROY) {
            dmap->rmItem(mc);
        } else {
            SweepItems(*dmap, mc);
        }
    }
}

void Tile::setItemsAllowedNoSweep(bool allow, bool destroy)
{
    if (destroy) items_mode = DESTROY;
    else if (allow) items_mode = ALLOWED;
    else items_mode = BLOCKED;
}

void Tile::setAccess(DungeonMap *dmap, const MapCoord &mc, MapHeight height, MapAccess acc, const Originator &originator)
{
    access[height] = acc;
    if (dmap) {
        // We don't need to tell Mediator about access changes, but we do need 
        // to call SweepCreatures.
        // (See also: Tile::setAccessNoSweep in the header)
        SweepCreatures(*dmap, mc, true, height, originator);
    }
}

void Tile::setAccess(DungeonMap *dmap, const MapCoord &mc, MapAccess acc, const Originator &originator)
{
    for (int i=0; i<=H_MISSILES; ++i) {
        access[i] = acc;
    }
    SweepCreatures(*dmap, mc, false, H_MISSILES, originator);
}

MiniMapColour Tile::getColour() const
{
    // Current method of determining mini map colour is as follows: 
    // 
    // * colour is COL_WALL if 
    //     access level != A_CLEAR at walking and flying heights, 
    //     AND access_level == A_BLOCKED at missile height (added for #119),
    //     AND tile is indestructible
    // * else, colour is COL_FLOOR
    //
    // This can be overridden by subclasses if required.
    //
    // This can also be overridden by Lua "map_as" property.

    if (use_col) return col;
    if (destructible()) return COL_FLOOR;
    if (getAccess(H_WALKING) == A_CLEAR) return COL_FLOOR;
    if (getAccess(H_FLYING) == A_CLEAR) return COL_FLOOR;
    if (getAccess(H_MISSILES) != A_BLOCKED) return COL_FLOOR;
    return COL_WALL;
}

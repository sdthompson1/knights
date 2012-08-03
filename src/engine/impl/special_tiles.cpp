/*
 * special_tiles.cpp
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

#include "action_data.hpp"
#include "colour_change.hpp"
#include "dungeon_map.hpp"
#include "dungeon_view.hpp"
#include "item.hpp"
#include "item_type.hpp"
#include "knight.hpp"
#include "lua_setup.hpp"
#include "mediator.hpp"
#include "player.hpp"
#include "rng.hpp"
#include "special_tiles.hpp"

#include "lua.hpp"


//
// Door
//

Door::Door(lua_State *lua) : Lockable(lua)
{
    open_graphic = LuaGetPtr<Graphic>(lua, -1, "open_graphic");   // default null
    closed_graphic = getGraphic();
    
    // Save the access state when the tile is closed
    for (int i = 0; i <= H_MISSILES; ++i) {
        closed_access[i] = getAccess(MapHeight(i));
    }

    // If the door is open initially then set all accesses to A_CLEAR,
    // and set the graphic to the open graphic
    if (!isClosed()) {
        for (int i = 0; i <= H_MISSILES; ++i) {
            setAccessNoSweep(MapHeight(i), A_CLEAR);
        }
        setGraphicNoNotify(open_graphic);
    }
}

shared_ptr<Tile> Door::doClone(bool)
{
    return shared_ptr<Tile>(new Door(*this));
}

void Door::openImpl(DungeonMap &dmap, const MapCoord &mc, const Originator &originator)
{
    setGraphic(&dmap, mc, open_graphic, shared_ptr<const ColourChange>());
    setItemsAllowed(&dmap, mc, true, false);
    setAccess(&dmap, mc, A_CLEAR, originator);
}

void Door::closeImpl(DungeonMap &dmap, const MapCoord &mc, const Originator &originator)
{
    setGraphic(&dmap, mc, closed_graphic, shared_ptr<const ColourChange>());
    setItemsAllowed(&dmap, mc, false, false);
    for (int h=0; h<=H_MISSILES; ++h) {
        setAccess(&dmap, mc, MapHeight(h), closed_access[MapHeight(h)], originator);
    }
}

void Door::damage(DungeonMap &dmap, const MapCoord &mc, int amt, shared_ptr<Creature> actor)
{
    // Filter out damage if the door is open!
    if (isClosed()) {
        Lockable::damage(dmap, mc, amt, actor);
    }
}

void Door::onHit(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor, const Originator &originator)
{
    // Filter out on_hit if the door is open!
    if (isClosed()) {
        Lockable::onHit(dmap, mc, actor, originator);
    }
}

bool Door::targettable() const
{
    // Open doors are not targettable.
    if (isClosed()) return Lockable::targettable();
    else return false;
}


//
// Chest
//

Chest::TrapInfo Chest::popTrapInfo(lua_State *lua)
{
    // [traps]

    Chest::TrapInfo result;
    
    lua_pushinteger(lua, 1);   // [traps 1]
    lua_gettable(lua, -2);     // [traps itemtype]
    result.itype = ReadLuaPtr<ItemType>(lua, -1);
    lua_pop(lua, 1);    // [traps]

    lua_pushinteger(lua, 2);  // [traps 2]
    lua_gettable(lua, -2);    // [traps action]
    result.action = LuaFunc(lua);  // [traps]
    lua_pop(lua, 1);   // []

    return result;
}

Chest::Chest(lua_State *lua) : Lockable(lua)
{
    open_graphic = LuaGetPtr<Graphic>(lua, -1, "open_graphic");  // default null
    closed_graphic = getGraphic();    
    facing = LuaGetMapDirection(lua, -1, "facing");  // default = north
    trap_chance = LuaGetProbability(lua, -1, "trap_chance");  // default = 0

    lua_getfield(lua, -1, "traps");  // [t traps]
    lua_len(lua, -1);   // [t traps len]
    const int sz = lua_tointeger(lua, -1);
    lua_pop(lua, 1);    // [t traps]
    for (int i = 0; i < sz; ++i) {
        lua_pushinteger(lua, i+1);  // [t traps idx]
        lua_gettable(lua, -2);      // [t traps trap]
        traps.push_back(popTrapInfo(lua));  // [t traps]
    }
    lua_pop(lua, 1);  // [t]

    if (isClosed()) {
        setItemsAllowedNoSweep(false, false);
    } else {
        setItemsAllowedNoSweep(true, false);
        setGraphicNoNotify(open_graphic);
    }
}

shared_ptr<Tile> Chest::doClone(bool)
{
    shared_ptr<Tile> new_tile(new Chest(*this));
    return new_tile;
}

void Chest::openImpl(DungeonMap &dmap, const MapCoord &mc, const Originator &)
{
    setGraphic(&dmap, mc, open_graphic, shared_ptr<const ColourChange>());
    setItemsAllowed(&dmap, mc, true, false);
    dmap.addItem(mc, stored_item);
    stored_item = shared_ptr<Item>();
}

void Chest::closeImpl(DungeonMap &dmap, const MapCoord &mc, const Originator &)
{
    setGraphic(&dmap, mc, closed_graphic, shared_ptr<const ColourChange>());
    stored_item = dmap.getItem(mc);
    if (stored_item) dmap.rmItem(mc);
    setItemsAllowed(&dmap, mc, false, false);
}

bool Chest::cannotActivateFrom(MapDirection &dir) const
{
    dir = Opposite(facing);
    return true;
}

bool Chest::canPlaceItem() const
{
    return true;
}

shared_ptr<Item> Chest::getPlacedItem() const
{
    return stored_item;
}

void Chest::placeItem(shared_ptr<Item> i) 
{
    stored_item = i;
}

void Chest::onDestroy(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor, const Originator &originator)
{
    Tile::onDestroy(dmap, mc, actor, originator);
    if (stored_item && !stored_item->getType().isFragile()) {
        // NB don't set actor, because the creature isn't actually dropping the item himself.
        // Also: no need to set allow_nonlocal - because the chest tile should always be free
        DropItem(stored_item, dmap, mc, false, false, D_NORTH, shared_ptr<Creature>());
    }
}

bool Chest::generateTrap(DungeonMap &dmap, const MapCoord &mc)
{
    if (g_rng.getBool(trap_chance)) {
        const TrapInfo &ti(traps[g_rng.getInt(0, traps.size())]);

        // We create a "simulated creature" and get him to place the trap using the action given in the TrapInfo.
        // This is really really ugly, but I want to get the pretrapped chests working fairly quickly :)
        // (What's really needed is a whole new way of specifying traps in the config file.)
        shared_ptr<Creature> dummy(new Creature(1, H_FLYING, 0, 0, 1));
        dummy->setFacing(Opposite(facing));  // this makes sure the bolt traps fire the right way
        dummy->addToMap(&dmap, DisplaceCoord(mc, facing));  // position him in front of the chest
        ActionData ad;
        ad.setActor(dummy);
        ad.setItem(0, MapCoord(), ti.itype);  // this should ensure the correct item gets dropped if the trap is disarmed
        ad.setTile(&dmap, mc, shared_from_this());
        ad.setGenericPos(&dmap, mc);
        ad.setFlag(true);  // enable hack on the control_actions side...
        ti.action.execute(ad);
        dummy->rmFromMap();

        return true;
    } else {
        return false;
    }
}


//
// Home
//

Home::Home(lua_State *lua) : Tile(lua)
{
    facing = LuaGetMapDirection(lua, -1, "facing");  // default = north
    special_exit = LuaGetBool(lua, -1, "special_exit");  // #20. default = false

    unsigned int col = static_cast<unsigned int>(LuaGetInt(lua, -1, "unsecured_colour"));   // default black
    shared_ptr<ColourChange> cc_unsec(new ColourChange);
    cc_unsec->add(Colour(255,0,0),
                  Colour((col & 0xff0000) >> 16,
                         (col & 0x00ff00) >> 8,
                         col & 0xff));
    setCCNoNotify(cc_unsec);
}

shared_ptr<Tile> Home::doClone(bool)
{
    // Homes are always copied, not shared.
    return shared_ptr<Tile>(new Home(*this));
}

void Home::onApproach(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> creature, const Originator &originator)
{
    // If creature is a kt, then do healing.
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(creature);
    if (kt) {
        const Player &pl (*kt->getPlayer());
        if (kt->getPos() == pl.getHomeLocation() && kt->getFacing() == pl.getHomeFacing()) {
            kt->startHomeHealing();
        }
    }
    Tile::onApproach(dmap, mc, creature, originator);
}

void Home::onWithdraw(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> creature, const Originator &originator)
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(creature);
    if (kt) {
        const Player &pl(*kt->getPlayer());
        if (kt->getPos() == pl.getHomeLocation() && kt->getFacing() == pl.getHomeFacing()) {
            kt->stopHomeHealing();
        }
        kt->getPlayer()->getDungeonView().cancelContinuousMessages();
    }
    Tile::onWithdraw(dmap, mc, creature, originator);
}

void Home::secure(DungeonMap &dmap, const MapCoord &mc, shared_ptr<const ColourChange> newcc)
{
    setGraphic(&dmap, mc, getGraphic(), newcc);
}


//
// Barrel
//

shared_ptr<Tile> Barrel::doClone(bool)
{
    // set items_allowed to false (see also Chest above)
    setItemsAllowedNoSweep(false, false);
    
    // Barrels need to be copied (because they may contain a stored item).
    shared_ptr<Tile> new_tile(new Barrel(*this));
    return new_tile;
}

bool Barrel::canPlaceItem() const
{
    return true;
}

shared_ptr<Item> Barrel::getPlacedItem() const
{
    return stored_item;
}

void Barrel::placeItem(shared_ptr<Item> i)
{
    stored_item = i;
}

void Barrel::onDestroy(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> actor, const Originator &originator)
{
    Tile::onDestroy(dmap, mc, actor, originator);
    if (stored_item && !stored_item->getType().isFragile()) {
        // NB don't set actor, because the creature isn't actually dropping the item himself.
        // Also: should always be able to drop on the barrel tile itself, so don't set "allow_nonlocal"
        DropItem(stored_item, dmap, mc, false, false, D_NORTH, shared_ptr<Creature>());
    }
}

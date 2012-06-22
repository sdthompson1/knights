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

#include "action.hpp"
#include "dungeon_map.hpp"
#include "dungeon_view.hpp"
#include "item.hpp"
#include "item_type.hpp"
#include "knight.hpp"
#include "mediator.hpp"
#include "player.hpp"
#include "rng.hpp"
#include "special_tiles.hpp"


//
// Door
//

Door::Door()
    : open_graphic(0), closed_graphic(0)
{
    for (int i=0; i<=H_MISSILES; ++i) closed_access[i] = A_BLOCKED;
}

void Door::doorConstruct(const Graphic *og, const Graphic *cg, const MapAccess acc[])
{
    open_graphic = og; closed_graphic = cg;
    for (int i=0; i<=H_MISSILES; ++i) closed_access[i] = acc[i];
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

shared_ptr<Tile> Chest::doClone(bool)
{
    // Make sure that the tile has items_allowed == false. This is a slight hack since there
    // is no way at present to configure a tile with items_allowed == false but
    // item_category >= 0...

    // (Note we can't do this during the ctor because Tile::construct gets called after that,
    // and it would set the items_allowed back to true in this case.)

    setItemsAllowed(0, MapCoord(), false, false);
    
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
    if (g_rng.getBool(float(trap_chance)/100.0f)) {
        const TrapInfo &ti(traps[g_rng.getInt(0, traps.size())]);

        // We create a "simulated creature" and get him to place the trap using the action given in the TrapInfo.
        // This is really really ugly, but I want to get the pretrapped chests working fairly quickly :)
        // (What's really needed is a whole new way of specifying traps in the config file.)
        shared_ptr<Creature> dummy(new Creature(1, H_FLYING, 0, 0, 1));
        dummy->setFacing(Opposite(facing));  // this makes sure the bolt traps fire the right way
        dummy->addToMap(&dmap, DisplaceCoord(mc, facing));  // position him in front of the chest
        ActionData ad;
        ad.setActor(dummy, false);
        ad.setItem(0, MapCoord(), ti.itype);  // this should ensure the correct item gets dropped if the trap is disarmed
        ad.setFlag(true);  // enable hack on the control_actions side...
        ti.action->execute(ad);
        dummy->rmFromMap();

        return true;
    } else {
        return false;
    }
}


//
// Home
//

void Home::homeConstruct(MapDirection fcg, shared_ptr<const ColourChange> cc_unsec)
{
    facing = fcg;
    setGraphic(0, MapCoord(), getGraphic(), cc_unsec); // "default" colour change for home.
}

shared_ptr<Tile> Home::doClone(bool)
{
    // Homes are always copied, not shared.
    return shared_ptr<Tile>(new Home(*this));
}

void Home::onApproach(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Creature> creature, const Originator &originator)
{
    // If creature is a kt, then do healing and check quest conditions.
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(creature);
    if (kt) {
        const Player &pl (*kt->getPlayer());
        if (kt->getPos() == pl.getHomeLocation() && kt->getFacing() == pl.getHomeFacing()) {
            kt->startHomeHealing();
        }
        if (kt->getPos() == pl.getExitLocation() && kt->getFacing() == pl.getExitFacing()) {
            vector<string> msgs;
            bool result = kt->getPlayer()->checkQuests(msgs);
            if (result) {
                // Player has won the game
                Mediator::instance().updateQuestIcons(pl, WIN_FROM_COMPLETE_QUEST);
                Mediator::instance().winGame(pl);
            } else {
                for (int i=0; i<msgs.size(); ++i) {
                    kt->getPlayer()->getDungeonView().addContinuousMessage(msgs[i]);
                }
            }
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
    // set items_allowed to false (see also Chest::doClone above)
    setItemsAllowed(0, MapCoord(), false, false);
    
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

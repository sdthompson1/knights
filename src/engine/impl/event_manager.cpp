/*
 * event_manager.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
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
#include "creature.hpp"
#include "dungeon_map.hpp"
#include "event_manager.hpp"
#include "item.hpp"
#include "tile.hpp"

void EventManager::onAddCreature(Creature &cr)
{
    // Do on_walk_over
    if (cr.getMap() && !cr.isMoving() && !cr.isApproaching()) {
        walkOverEvent(cr, cr.getOriginator());
    }
}

void EventManager::onRmCreature(Creature &cr)
{
    if (!cr.getMap()) return;
    if (!cr.isMoving() && cr.isApproaching()) {
        withdrawEvent(cr);
    }
}

void EventManager::postRepositionCreature(Creature &cr)
{
    // Do on_walk_over
    if (cr.getMap() && !cr.isMoving() && !cr.isApproaching()
    && cr.queryWalkOverFlag()) {
        walkOverEvent(cr, cr.getOriginator());
    }
}

void EventManager::onChangeEntityMotion(Creature &cr)
{
    if (!cr.getMap()) return;
    if (cr.getMotionType() == MT_WITHDRAW) {
        // Do on_withdraw event if necessary
        withdrawEvent(cr);
    } else if (!cr.isMoving() && cr.isApproaching()) {
        // Do on_approach event if necessary
        MapCoord mc = DisplaceCoord(cr.getPos(), cr.getFacing());
        vector<shared_ptr<Tile> > tiles;
        cr.getMap()->getTiles(mc, tiles);
        shared_ptr<Creature> crs(static_pointer_cast<Creature>(cr.shared_from_this()));
        for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end();
        ++it) {
            (*it)->onApproach(*cr.getMap(), cr.getPos(), crs, Originator(OT_None()));
            if (!cr.getMap()) return;
        }
    }
}

void EventManager::onAddTile(DungeonMap &dmap, const MapCoord &mc, Tile &tile, const Originator &originator)
{
    // NB we don't bother running on_approach from here.
    // Do on_walk_over event if necessary (just for this tile though).
    vector<shared_ptr<Entity> > ents;
    dmap.getEntities(mc, ents);
    for (vector<shared_ptr<Entity> >::iterator it = ents.begin(); it != ents.end(); ++it) {
        if (!(*it)->isMoving()) {
            shared_ptr<Creature> cr = dynamic_pointer_cast<Creature>(*it);
            if (cr) {
                tile.onWalkOver(dmap, mc, cr, originator);
            }
        }
    }
}

void EventManager::onRmTile(DungeonMap &dmap, const MapCoord &mc, Tile &tile, const Originator &originator)
{
    // Run on_withdraw if necessary (just for this tile though)
    vector<shared_ptr<Entity> > ents;
    for (int d=0; d<4; ++d) {
        MapCoord mc2 = DisplaceCoord(mc, MapDirection(d));        
        dmap.getEntities(mc2, ents);
        for (vector<shared_ptr<Entity> >::iterator it = ents.begin(); it != ents.end();
        ++it) {
            if (!(*it)->isMoving() && (*it)->isApproaching() 
            && (*it)->getFacing() == Opposite(MapDirection(d))) {
                shared_ptr<Creature> cr = dynamic_pointer_cast<Creature>(*it);
                if (cr) {
                    tile.onWithdraw(dmap, mc, cr, originator);  // (not mc2; we want the tile's position, not the creature's.)
                }
            }
        }
    }
}


void EventManager::walkOverEvent(Creature &cr, const Originator &originator)
{
    // Do on_walk_over
    if (cr.getMap()) {
        vector<shared_ptr<Tile> > tiles;
        cr.getMap()->getTiles(cr.getPos(), tiles);
        shared_ptr<Creature> crs(static_pointer_cast<Creature>(cr.shared_from_this()));
        for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end();
        ++it) {
            (*it)->onWalkOver(*cr.getMap(), cr.getPos(), crs, originator);
            if (!cr.getMap()) return;  // don't process any further events
        }

        shared_ptr<Item> item = cr.getMap()->getItem(cr.getPos());
        if (item) {
            item->getType().onWalkOver(*cr.getMap(), cr.getPos(), crs, item->getOwner());
        }
    }

    cr.resetWalkOverFlag();
}

void EventManager::withdrawEvent(Creature &cr)
{
    if (cr.getMap()) {
        vector<shared_ptr<Tile> > tiles;
        MapCoord mc = DisplaceCoord(cr.getPos(), cr.getFacing());
        cr.getMap()->getTiles(mc, tiles);
        shared_ptr<Creature> crs(static_pointer_cast<Creature>(cr.shared_from_this()));
        for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end();
        ++it) {
            (*it)->onWithdraw(*cr.getMap(), mc, crs, Originator(OT_None()));
            if (!cr.getMap()) return;
        }
    }
}

void EventManager::runHook(const string &name, shared_ptr<Creature> cr) const
{
    ActionData ad;
    ad.setActor(cr, false);
    doHook(name, ad);
}

void EventManager::runHook(const string &name, DungeonMap *dmap, const MapCoord &mc) const
{
    ActionData ad;
    ad.setTile(dmap, mc, shared_ptr<Tile>());
    doHook(name, ad);
}

void EventManager::doHook(const string &name, const ActionData &ad) const 
{
    map<string, const Action *>::const_iterator it = hooks.find(name);
    if (it != hooks.end() && it->second) {
        it->second->execute(ad);
    }
}

/*
 * dungeon_map.cpp
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

#include "copy_if.hpp"
#include "creature.hpp"
#include "dungeon_map.hpp"
#include "entity.hpp"
#include "item.hpp"
#include "knights_callbacks.hpp"
#include "map_helper.hpp"
#include "mediator.hpp"
#include "rng.hpp"
#include "room_map.hpp"
#include "sweep.hpp"
#include "task_manager.hpp"
#include "tile.hpp"


/////////////////////////
//  Map support stuff  //
/////////////////////////

MapDirection Opposite(MapDirection m)
{
    return MapDirection((int(m) + 2) % 4);
}

MapDirection Clockwise(MapDirection m)
{
    return MapDirection((int(m) + 1) % 4);
}

MapDirection Anticlockwise(MapDirection m)
{
    return MapDirection((int(m) + 3) % 4);
}

MapCoord DisplaceCoord(const MapCoord &base, MapDirection dir, int amt)
{
    switch (dir) {
    case D_EAST:
        return MapCoord(base.getX() + amt, base.getY());
    case D_NORTH:
        return MapCoord(base.getX(), base.getY() - amt);
    case D_SOUTH:
        return MapCoord(base.getX(), base.getY() + amt);
    case D_WEST:
        return MapCoord(base.getX() - amt, base.getY());
    default:
        return base;
    }
}

size_t hash_value(const MapCoord &mc)
{
    return (mc.getX() << 16) + (mc.getY());
}


/////////////////
//  MapHelper  //
/////////////////

// NB for maphelper routines, caller must do own error checking, eg do
// not try to add an entity to an invalid mapcoord, or to a square
// without the appropriate access level.

// also note that these are low level routines which do not call mediator etc.

void MapHelper::addEntity(shared_ptr<Entity> e)
{
    if (!e) return;
    DungeonMap *const dmap = e->getMap();
    if (!dmap) return;
    ASSERT(dmap->valid(e->getPos()));

    // add to "entities"
    dmap->entities.insert(make_pair(e->getPos(), e));
    
    // Moving entities must be stored twice in "entities", once in the "behind" square
    // and once in the "ahead" square:
    if (e->getMotionType() == MT_MOVE) {
        const MapCoord mc2 = DisplaceCoord(e->getPos(), e->getFacing());
        if (dmap->valid(mc2)) {
            dmap->entities.insert(make_pair(mc2, e));
        }
    }
}

void MapHelper::rmEntity(shared_ptr<Entity> e)
{
    if (!e) return;
    DungeonMap *const dmap = e->getMap();
    if (!dmap) return;
    ASSERT(dmap->valid(e->getPos()));

    // remove from "current" pos
    typedef DungeonMap::EntityContainer::iterator Itor;
    pair<Itor,Itor> p = dmap->entities.equal_range(e->getPos());
    for (Itor it = p.first; it != p.second; ++it) {
        if (it->second == e) {
            dmap->entities.erase(it);
            break;
        }
    }

    // remove from "forward" pos (if moving)
    if (e->isMoving() && !e->isApproaching()) {
        const MapCoord mc2 = DisplaceCoord(e->getPos(), e->getFacing());
        if (dmap->valid(mc2)) {
            p = dmap->entities.equal_range(mc2);
            for (Itor it = p.first; it != p.second; ++it) {
                if (it->second == e) {
                    dmap->entities.erase(it);
                    break;
                }
            }
        }
    }
}




//////////////////
//  DungeonMap  //
//////////////////

DungeonMap::DungeonMap()
    : map_width(0), map_height(0), room_map(0)
{ }

DungeonMap::~DungeonMap()
{
    delete room_map;
}

void DungeonMap::create(int w, int h)
{
    if (w < 1 || h < 1) return;

    entities.clear();
    items.clear();
    tiles.clear();

    map_width = w;
    map_height = h;
    tiles.resize(w*h);

    setRoomMap(0);

    displaced_items.clear();
}

void DungeonMap::setRoomMap(RoomMap *r)
{
    if (room_map) delete room_map;
    room_map = r;
}

void DungeonMap::getEntities(const MapCoord &mc, vector<shared_ptr<Entity> > &results) const
{
    results.clear();
    pair<EntityContainer::iterator, EntityContainer::iterator> p = entities.equal_range(mc);
    transform(p.first, p.second, back_inserter(results), GetSecond<shared_ptr<Entity> >());
}

void DungeonMap::getAllEntities(const MapCoord &mc, vector<shared_ptr<Entity> > &results)
    const
{
    getEntities(mc, results);
    doEntity(DisplaceCoord(mc, D_WEST), D_EAST, results);
    doEntity(DisplaceCoord(mc, D_EAST), D_WEST, results);
    doEntity(DisplaceCoord(mc, D_NORTH), D_SOUTH, results);
    doEntity(DisplaceCoord(mc, D_SOUTH), D_NORTH, results);
}

namespace {
    struct ApproachTest {
        ApproachTest(MapDirection d_) : d(d_) { }
        bool operator()(const shared_ptr<Entity> &e) const {
            return e && e->getFacing()==d && e->isApproaching();
        }
        MapDirection d;
    };
}

void DungeonMap::doEntity(const MapCoord &mc, MapDirection facing, 
                          vector<shared_ptr<Entity> > &results) const
{
    if (valid(mc)) {
        vector<shared_ptr<Entity> > ents;
        getEntities(mc, ents);
        copy_if(ents.begin(), ents.end(), back_inserter(results), ApproachTest(facing));
    }
}

shared_ptr<Creature> DungeonMap::getTargetCreatureHelper(const Entity &attacker,
                                                         const MapCoord &mc) const
{
    const int halfway_point = 500;
    
    vector<shared_ptr<Entity> > ents;
    shared_ptr<Creature> result;
    MapHeight result_height;
    
    // We don't have a separate index for "targettable creatures"; instead we rely
    // on dynamic casting from the main entities index (getEntities)...
    getEntities(mc, ents);
    for (vector<shared_ptr<Entity> >::iterator it = ents.begin(); it != ents.end(); ++it) {
        if (

        // Check that I am not targetting myself!
        it->get() != &attacker
            
        // Check that the target obeys the "50% rule"
        && (    ((*it)->getPos() == mc && (*it)->getOffset() < halfway_point)
             || ((*it)->getPos() != mc && (*it)->getOffset() >= halfway_point)
           )

        // We want the *highest* of all possible target creatures
        && (!result || (*it)->getHeight() > result_height)
        
        ) {
            // Check that it really is a creature
            shared_ptr<Creature> cr = dynamic_pointer_cast<Creature>(*it);
            if (cr) {
                result = cr;
                result_height = cr->getHeight();
            }
        }
    }

    return result;
}

shared_ptr<Creature> DungeonMap::getTargetCreature(const Entity & attacker,
                                                   bool allow_both_sqs) const
{
    if (attacker.getMap() != this) return shared_ptr<Creature>();

    const int halfway_point = 500;
    const int attacker_offset = attacker.getOffset();
    bool attacker_more_than_halfway = attacker_offset >= halfway_point;
    
    if (allow_both_sqs || attacker_more_than_halfway) {
        shared_ptr<Creature> result = getTargetCreatureHelper(attacker,
                DisplaceCoord(attacker.getPos(), attacker.getFacing()));
        if (result) return result;
    }
    if (allow_both_sqs || !attacker_more_than_halfway) {
        shared_ptr<Creature> result = getTargetCreatureHelper(attacker, attacker.getPos());
        if (result) return result;
    }
    return shared_ptr<Creature>();
}


//
// getAccess -- returns MapAccess code for putting an entity at a given place.
// If an entity is already present, returns A_BLOCKED, else returns mapaccess level
// from the underlying tile type(s).
//

// getAccessTilesOnly -- same but checks tiles only, NOT entities

MapAccess DungeonMap::getAccess(const MapCoord &mc, MapHeight h, Entity *ignore) const
{
    const int halfway_mark = 500;
    
    if (!valid(mc)) return A_BLOCKED;             // no such square

    // Check for entities already present. Access is blocked if an
    // entity is already present at that height. In other words only
    // one entity can be present per square per height.
    // 22-Oct-2006: changed this so that an entity which is moving outwards and which
    // is more than 50% through its move, does not block a square.
    vector<shared_ptr<Entity> > ents;
    getEntities(mc, ents);
    for (vector<shared_ptr<Entity> >::iterator it = ents.begin(); it != ents.end(); ++it) {
        if (it->get() != ignore
            && (*it)->getHeight() == h
            && ((*it)->getOffset() < halfway_mark || 
                DisplaceCoord((*it)->getPos(), (*it)->getFacing()) == mc)
            ) return A_BLOCKED;
    }   

    // If no relevant entities present then check the tiles.
    return getAccessTilesOnly(mc, h);
}

MapAccess DungeonMap::getAccessTilesOnly(const MapCoord &mc, MapHeight h) const
{
    if (!valid(mc)) return A_BLOCKED;   // no such square

    // Check for Tiles.
    MapAccess result = A_CLEAR;
    int i = index(mc);
    for (list<shared_ptr<Tile> >::const_iterator it = tiles[i].begin(); it != tiles[i].end();
    ++it) {
        MapAccess a = (*it)->getAccess(h);
        if (a < result) result = a;
    }
    return result;
}

//
// canPlaceMissile
//

bool DungeonMap::canPlaceMissile(const MapCoord &mc, MapDirection dir) const
{
    // Note it's ok to place a missile on mc, as long as no other
    // missile is already present, however access for tile ahead is
    // checked in the normal way.
    vector<shared_ptr<Entity> > ents;
    getEntities(mc, ents);
    for (vector<shared_ptr<Entity> >::iterator it = ents.begin(); it != ents.end(); ++it) {
        if ((*it)->getHeight() == H_MISSILES + dir) return false;
    }
    return (getAccess(DisplaceCoord(mc, dir), MapHeight(H_MISSILES + dir)) > A_BLOCKED);
}


//
// getitem.
//

shared_ptr<Item> DungeonMap::getItem(const MapCoord &mc) const
{
    ItemContainer::const_iterator it = items.find(mc);
    if (it != items.end()) return it->second;
    else return shared_ptr<Item>();
}


//
// add/remove items and tiles.
//

bool DungeonMap::addItem(const MapCoord &mc, shared_ptr<Item> it)
{
    if (!valid(mc)) return false;
    if (!it) return false;
    pair<ItemContainer::iterator,bool> p = items.insert(make_pair(mc,it));
    if (!p.second) return false;
    Mediator::instance().onAddItem(*this, mc, *it);
    return true;
}

bool DungeonMap::rmItem(const MapCoord &mc)
{
    if (!valid(mc)) return false;
    ItemContainer::iterator itor = items.find(mc);
    if (itor == items.end()) return false;
    Mediator::instance().onRmItem(*this, mc, *itor->second);
    items.erase(itor);
    return true;
}

bool DungeonMap::addTile(const MapCoord &mc, shared_ptr<Tile> t, const Originator &originator)
{
    if (!valid(mc)) return false;
    if (!t) return false;
    const int idx = index(mc);
    const int tdepth = t->getDepth();
    list<shared_ptr<Tile> >::iterator it;

    bool need_sweep_items = false;
    bool need_destroy_items = (t->destroyItems());
    if (!need_destroy_items && !t->itemsAllowed()) {
        // Items not allowed at the new tile. Check if square previously allowed items
        for (it = tiles[idx].begin(); it != tiles[idx].end(); ++it) {
            if (!(*it)->itemsAllowed()) break;
        }
        if (it == tiles[idx].end()) {
            // items were allowed before, but are not now
            need_sweep_items = true;
        }
    }
    
    // make the change
    for (it = tiles[idx].begin();
         it != tiles[idx].end() && (*it)->getDepth() >= tdepth;
         ++it) ; // move "it" up to the correct position
    tiles[idx].insert(it, t);

    // "post" events
    Mediator::instance().onAddTile(*this, mc, *t, originator);
    if (need_destroy_items) {
        rmItem(mc);
    } else if (need_sweep_items) {
        SweepItems(*this, mc);
    }
    return true;
}

bool DungeonMap::rmTile(const MapCoord &mc, shared_ptr<Tile> t, const Originator &originator)
{
    if (!valid(mc)) return false;
    if (!t) return false;
    const int idx = index(mc);
    list<shared_ptr<Tile> >::iterator it;

    // find where to remove the tile from   
    it = find(tiles[idx].begin(), tiles[idx].end(), t);
    if (it == tiles[idx].end()) return false; // hmm, tile doesn't seem to be here

    // remove it
    tiles[idx].erase(it);

    // "post" events
    Mediator::instance().onRmTile(*this, mc, *t, originator);

    return true;
}

void DungeonMap::clearTiles(const MapCoord &mc)
{
    if (!valid(mc)) return;
    const int idx = index(mc);

    for (list<shared_ptr<Tile> >::iterator it = tiles[idx].begin();
    it != tiles[idx].end(); ++it) {
        Mediator::instance().onRmTile(*this, mc, **it, Originator(OT_None()));
    }
    tiles[idx].clear();
}

void DungeonMap::clearAll()
{
    displaced_items.clear();
    entities.clear();
    items.clear();
    tiles.clear();
}


//
// get tiles.
//

void DungeonMap::getTiles(const MapCoord &mc, vector<shared_ptr<Tile> > &output) const
{
    output.clear();
    if (!valid(mc)) return;
    const list<shared_ptr<Tile> > & t(tiles[index(mc)]);
    copy(t.begin(), t.end(), back_inserter(output));
}


//
// Automatic item replacement.
// This automatically teleports items back into the dungeon (onto a random square).
// It's used as a last resort when we fail to drop something into the dungeon (for example
// when a knight dies but the room he's in is full of objects already). The intention is to
// stop important things like gems or books from being lost when that happens.
// (See also ItemCheckTask.)
//

// NOTE: It would probably be a good idea to merge ItemReplacementTask and ItemCheckTask
// as they are now working together in quite a complicated way.

class ItemReplacementTask : public Task {
public:
    explicit ItemReplacementTask(DungeonMap &dm) : dmap(dm), which_item(0) { }
    virtual void execute(TaskManager &tm)
    {
        // Have 20 attempts at placing an item.
        for (int attempt = 0; attempt < 20 && !dmap.displaced_items.empty(); ++attempt) {

            // Cycle through the available items.
            ++which_item;
            if (which_item >= dmap.displaced_items.size()) which_item = 0;
            DungeonMap::DisplacedItem & di = dmap.displaced_items[which_item];

            ++di.tries;
            
            // Generate a random map square
            const int x = g_rng.getInt(0, dmap.getWidth());
            const int y = g_rng.getInt(0, dmap.getHeight());
            const MapCoord mc(x,y);

            // Attempt to drop the item into the square
            // Note: don't allow nonlocal drop - if a "local" drop fails, we might as well just wait
            // until the next attempt.
            const bool result = DropItem(di.item, dmap, mc, false, false, D_NORTH, shared_ptr<Creature>());
        
            if (result || di.item->getNumber() == 0) {
                // Drop was successful.
                printRespawnMessage(di);
                dmap.displaced_items.erase(dmap.displaced_items.begin() + which_item);
                break;   // Exit after first successful replacement
            } else {
                // Drop was unsuccessful.
                if (di.important && di.tries > 10) {
                    // This is a quest critical item and we have failed several times to drop it.
                    // In this case we go into "emergency mode" and just destroy the item already present (if there is one) to make
                    // room for the new item...
                    boost::shared_ptr<Item> current_item = dmap.getItem(mc);
                    if (current_item) {
                        dmap.rmItem(mc);
                        dmap.addItem(mc, di.item);
                        printRespawnMessage(di);
                        dmap.displaced_items.erase(dmap.displaced_items.begin() + which_item);
                        break;  // It worked
                    }
                }
            }
        }
        
        if (!dmap.displaced_items.empty()) {
            // There are still outstanding items so try again after item_replacement_interval.
            tm.addTask(shared_from_this(), TP_NORMAL, tm.getGVT()
                + Mediator::instance().cfgInt("item_replacement_interval"));
        }
    }

private:
    static void printRespawnMessage(const DungeonMap::DisplacedItem &di)
    {
        const std::string name = di.item->getType().getName();
        if (!name.empty()) {
            Mediator::instance().getCallbacks().gameMsg(-1, name + " has been respawned at a random location.");
        }
    }
    
private:
    DungeonMap &dmap;
    int tries;
    int which_item;
};

void DungeonMap::addDisplacedItem(shared_ptr<Item> i)
{
    if (displaced_items.empty()) {
        shared_ptr<Task> t(new ItemReplacementTask(*this));
        TaskManager &tm(Mediator::instance().getTaskManager());
        tm.addTask(t, TP_NORMAL, tm.getGVT() + Mediator::instance().cfgInt("item_replacement_interval"));
    }
    DisplacedItem di;
    di.item = i;
    displaced_items.push_back(di);
}

void DungeonMap::countItems(std::map<const ItemType*, int> &result) const
{
    // search items on tiles.
    for (ItemContainer::const_iterator it = items.begin(); it != items.end(); ++it) {
        const ItemType * item_type = &it->second->getType();
        std::map<const ItemType*, int>::iterator result_it = result.find(item_type);
        if (result_it != result.end()) ++result_it->second;
    }

    // search items in the displaced queue
    for (std::vector<DisplacedItem>::const_iterator it = displaced_items.begin(); it != displaced_items.end(); ++it) {
        const ItemType * item_type = &it->item->getType();
        std::map<const ItemType*, int>::iterator result_it = result.find(item_type);
        if (result_it != result.end()) ++result_it->second;
    }

    // search items "stored" in tiles. This catches items in chests.
    for (TileContainer::const_iterator it1 = tiles.begin(); it1 != tiles.end(); ++it1) {
        for (list<shared_ptr<Tile> >::const_iterator it2 = it1->begin(); it2 != it1->end(); ++it2) {
            const Item * placed_item = (*it2)->getPlacedItem().get();
            if (placed_item) {
                const ItemType * item_type = &placed_item->getType();
                std::map<const ItemType*, int>::iterator result_it = result.find(item_type);
                if (result_it != result.end()) ++result_it->second;
            }
        }
    }
}

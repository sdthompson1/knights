/*
 * dungeon_map.hpp
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

/*
 * DungeonMap: holds Entities, Items and Tiles.
 * The server holds a DungeonMap which represents the definitive
 * current state of the dungeon. Each client also holds its own
 * DungeonMap, which contains only what the client knows about (and
 * will therefore be incomplete and/or out of date with respect to the
 * server).
 *
 */

#ifndef DUNGEON_MAP_HPP
#define DUNGEON_MAP_HPP

#include "map_support.hpp"

#include "boost/multi_index_container.hpp"
#include "boost/multi_index/hashed_index.hpp"
#include "boost/shared_ptr.hpp"
using namespace boost;
using namespace boost::multi_index;

#include <list>
#include <map>
#include <vector>

class Creature;
class Entity;
class Item;
class ItemReplacementTask; // used internally
class ItemType;
class Originator;
class RoomMap;
class Task;
class TaskManager;
class Tile;

class DungeonMap {
    friend class MapHelper;

public:
    struct DisplacedItem {
        DisplacedItem() : tries(0) { }
        boost::shared_ptr<Item> item;
        int tries;
    };
    
public:
    DungeonMap();
    ~DungeonMap();
    void create(int w, int h);  // initialize empty dmap of the given size.

    // room map (NB room map is deleted by DungeonMap dtor.)
    void setRoomMap(RoomMap *r); // deletes the old room map
    RoomMap * getRoomMap() const { return room_map; }
    
    // check access level for a given square & height
    // (if entity present already, returns A_BLOCKED, otherwise looks up MapAccess for
    // underlying tile.)
    MapAccess getAccess(const MapCoord &mc, MapHeight h, Entity *ignore = 0) const;

    // same, but checks only tiles -- not entities.
    MapAccess getAccessTilesOnly(const MapCoord &mc, MapHeight h) const;
    
    // check if we can place a new missile at a given square
    bool canPlaceMissile(const MapCoord &mc, MapDirection dir_of_travel) const;

    // get width, height
    int getWidth() const { return map_width; }
    int getHeight() const { return map_height; }

    // check that a given coord is in the map
    // (which means x between 0 and width-1, and y between 0 and height-1 -- there are never
    // any "holes" in a map.)
    bool valid(const MapCoord &mc) const {
        return (mc.getX() >= 0 && mc.getX() < map_width && mc.getY() >= 0
                && mc.getY() < map_height);
    }
    
    // add/remove items/tiles
    // these return true if successful.
    bool addItem(const MapCoord &mc, shared_ptr<Item> it);
    bool rmItem(const MapCoord &mc);
    bool addTile(const MapCoord &mc, shared_ptr<Tile> ti, const Originator &);
    bool rmTile(const MapCoord &mc, shared_ptr<Tile> ti, const Originator &);

    // this removes all tiles at a given mapcoord.
    void clearTiles(const MapCoord &mc);

    // remove all entities, items and tiles from the map. (Used at the end of a game...)
    void clearAll();
    
    // get entities at a given square (approached entities do not count)
    // existing contents of "results" are cleared.
    // NB if an entity is halfway between squares then it counts as being on both of those
    // squares.
    void getEntities(const MapCoord &mc, std::vector<boost::shared_ptr<Entity> > &results) const;
    
    // get all entities, including approached ones
    // return them in "results" vector (existing contents of "results" vector are cleared).
    void getAllEntities(const MapCoord &mc, std::vector<boost::shared_ptr<Entity> > &results) const;

    // get target creature: this is used for combat.
    // allow_both_sqs: if true, can target attacker's sq or the sq one ahead of the attacker.
    // if false, can target the sq. that the attacker is mainly on (either his sq or the
    // sq ahead, depending on whether offset < or > 50%).
    //  Update 29-Dec-2015: if allow_both_sqs=false we now allow a slight "overlap" between
    //  the two squares. Fixes #158.
    shared_ptr<Creature> getTargetCreature(const Entity & attacker, bool allow_both_sqs) const;
    
    // get item at a given square
    shared_ptr<Item> getItem(const MapCoord &mc) const;

    // get tiles at a given square
    // Returns results in a vector (not the most efficient method, but probably the most
    // flexible). Existing contents of the vector are deleted.
    void getTiles(const MapCoord &mc, std::vector<boost::shared_ptr<Tile> > &output) const;
    
    // displaced items:
    // Used for items that should have been dropped (eg when a knight died), but there
    // was no space to drop them. They will be automatically re-inserted into the dungeon
    // (onto a random square) as soon as possible.
    void addDisplacedItem(shared_ptr<Item> i);

    // Direct access to the displaced items vector. NOTE: Do not use this to add displaced items
    // as it will not set up the item replacement task. (Call addDisplacedItem instead.) It should
    // be ok to remove items, or modify existing ones, though.
    std::vector<DisplacedItem> & getDisplacedItems() { return displaced_items; }

    // Count items of various types. Used by item respawn code
    void countItems(std::map<ItemType*, int> &result) const;
    
private:
    // helper function for getAllEntities
    void doEntity(const MapCoord &mc, MapDirection facing, 
                  std::vector<boost::shared_ptr<Entity> > &results) const;

    // helper for getTargetCreature
    shared_ptr<Creature> getTargetCreatureHelper(const Entity &, const MapCoord &, bool) const;
    
    // get index corresponding to a mapcoord
    int index(const MapCoord &mc) const
        { ASSERT(valid(mc)); return mc.getY() * map_width + mc.getX(); }
    
private:
    // Dungeon contents. Use hash tables for entities and items (since
    // these are likely to be sparse) but a straightforward
    // index-by-square for tiles (tiles are likely to be dense, with
    // at least one on each square). Use boost::multi_index for the
    // hash tables (I don't like to rely on hash_map/hash_set since
    // they seem to be non-standardized between different compilers).

    // Note that any Entity halfway between two squares is given two
    // separate entries in entities index (except for approaching
    // entities which only get one entry, on their "base" square).

    template<class T>
    struct GetPos {
        typedef MapCoord result_type;
        const MapCoord & operator()(const std::pair<MapCoord, T> &p) const {
            return p.first;
        }
    };
    template<class T>
    struct GetSecond {
        typedef MapCoord result_type;
        const T & operator()(const std::pair<MapCoord,T> &p) const {
            return p.second;
        }
    };

    typedef boost::multi_index_container<std::pair<MapCoord, shared_ptr<Entity> >, 
        indexed_by< hashed_non_unique<GetPos<shared_ptr<Entity> > > > > EntityContainer;
    typedef boost::multi_index_container<std::pair<MapCoord, shared_ptr<Item> >,
        indexed_by< hashed_unique<GetPos<shared_ptr<Item> > > > > ItemContainer;
    typedef std::vector<std::list<shared_ptr<Tile> > > TileContainer;

    EntityContainer entities;
    ItemContainer items;
    TileContainer tiles;

    int map_width, map_height;

    RoomMap *room_map;

    // List of items that need to be re-placed
    std::vector<DisplacedItem> displaced_items;
    friend class ItemReplacementTask;
};


#endif

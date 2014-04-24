/*
 * view_manager.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2013.
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

#ifndef VIEW_MANAGER_HPP
#define VIEW_MANAGER_HPP

#include "mini_map_colour.hpp"

#include "boost/shared_ptr.hpp"
using namespace boost;

#include <vector>

class ColourChange;
class DungeonMap;
class DungeonView;
class Entity;
class Item;
class MapCoord;
class Player;
class RoomMap;
class Sound;
class Tile;
class VisMask;

class ViewManager {
public:
    explicit ViewManager(int nplyrs);
    
    // Note: all "on" functions are called *after* the event has occurred.
    
    // Players (to be called before start of game)
    void onAddPlayer(Player &);
    
    // Entities
    void onAddEntity(shared_ptr<Entity>);
    void onRmEntity(shared_ptr<Entity>);
    void onRepositionEntity(shared_ptr<Entity>);
    void onChangeEntityMotion(shared_ptr<Entity>, bool missile_mode);
    void onFlipEntityMotion(shared_ptr<Entity>);
    void onChangeEntityAnim(shared_ptr<Entity>); // called when either Anim or Overlay changes.
    void onChangeSpeechBubble(shared_ptr<Knight>);
    void onChangeEntityFacing(shared_ptr<Entity>);
    void onChangeEntityVisible(shared_ptr<Entity>);

    // Items
    void onAddItem(const DungeonMap &, const MapCoord &, const Item &);
    void onRmItem(const DungeonMap &, const MapCoord &, const Item &);
    void onChangeItemGraphic(const DungeonMap &, const MapCoord &, const Item &);

    // Tiles
    void onAddTile(const DungeonMap &, const MapCoord &, const Tile &);
    void onRmTile(const DungeonMap &, const MapCoord &, const Tile &);
    void onChangeTile(const DungeonMap &, const MapCoord &, const Tile &);

    // Icons
    void placeIcon(const DungeonMap &, const MapCoord &, const Graphic *, int dur);

    // Misc
    
    // flash screen for ent & all knights in the same room as ent
    // also: play sound effect 'originating' at a given square
    void flashScreen(shared_ptr<Entity> ent, int delay);
    void playSound(DungeonMap &dmap, const MapCoord &mc, const Sound &sound, int frequency, bool all);

    // Catch-up for newly joined observers
    void sendCurrentRoom(int player, DungeonView &dview);
    
private:
    static void getCurrentRoomLocation(const Player &, const DungeonMap &, MapCoord &,
                                       int &, int &);
    static bool squareVisibleToPlayer(const DungeonMap &, const MapCoord &, const Player &);
    static bool entityVisibleToPlayer(const Entity &, const Player &, bool);
    static void getRelativePos(const Player &, const DungeonMap *, const MapCoord &,
                               int &, int &);
    void invalidateSquare(const Player &, const RoomMap &, const MapCoord &);
    void uploadRoomSquares(const Player &);
    static void uploadSquare(DungeonView &, const DungeonMap &, const MapCoord &, int, int, bool);
    bool updateRoom(int plyr);
    
    void doAddEntity(int plyr, shared_ptr<Entity>, bool mine);
    void reallyAddEntity(int plyr, shared_ptr<Entity> ent, unsigned short int id, DungeonView &dungeon_view);
    void doRmEntity(int plyr, weak_ptr<Entity> ent, unsigned short int id);

    void sendAnimChange(int plyr, const Entity &ent, unsigned short int id);
    
    void setItemIfVisible(const DungeonMap &, const MapCoord &, const Graphic *);
    void setTileIfVisible(const DungeonMap &, const MapCoord &, int, const Graphic *,
                          shared_ptr<const ColourChange>, MiniMapColour);

    
private:
    std::vector<Player*> players;

    typedef std::map<int, std::vector<bool> > SquaresInvalidMap;
    typedef std::map<const Player *, SquaresInvalidMap> SquaresInvalidCache;
    SquaresInvalidCache squares_invalid_cache;
    
    // Entity IDs
    // Sorted into player order.
    std::vector<unsigned short int> next_id;
    std::vector<std::set<unsigned short int> > active_ids;
    std::vector<std::map<weak_ptr<Entity>, unsigned short int> > id_map;

    // Look up entity id
    // If found, then result.second will be true, and result.first will be the ID number
    // If not found, then result.second will be false.
    std::pair<unsigned short int, bool> fetchID(int plyr, weak_ptr<Entity> ent);

    // Creates a new ID for the given entity
    // It is assumed that the entity does not already have an id (ie
    // fetchID returned false)
    unsigned short int fetchNewID(int plyr, weak_ptr<Entity> ent, bool mine);
};

#endif

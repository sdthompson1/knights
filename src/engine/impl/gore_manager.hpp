/*
 * gore_manager.hpp
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

/*
 * Blood/gore effects. These are done as special Tiles which overlay the normal map tiles.
 * There are three sorts of gore tiles:
 * (i) Blood splats. These are placed whenever a creature loses hit points.
 * (ii) Monster corpses (dead vampire bats or zombies).
 * (iii) Knight corpses (these are handled slightly differently to monsters).
 *
 */

#ifndef GORE_MANAGER_HPP
#define GORE_MANAGER_HPP

class DungeonMap;
class Graphic;
class MapCoord;
class MonsterType;
class Player;
class Tile;

#include "boost/shared_ptr.hpp"
using namespace boost;

#include <map>
#include <vector>

class GoreManager {
public:
    GoreManager();
    
    //
    // setup
    //

    // For monster corpses and blood tiles, the "add" routine can be called more than once,
    // and this will create a sequence of tiles to be used one after the other. (Eg this is
    // used for the blood tiles, with increasing amounts of blood on each tile in the
    // sequence.)

    // For knight corpses, tiles with and without blood splats can be given. (The latter is
    // used when a knight is poisoned, the former when he is killed in the normal way.)

    // NB -- if addMonsterCorpse is called with monster == NULL, then we add a 'generic'
    // corpse tile, that is not associated with a particular monster, but still counts
    // as a corpse for purposes of zombie activity. (This is used for the bone tiles.)

    void setKnightCorpse(const Player &, shared_ptr<Tile> with_blood,
                         shared_ptr<Tile> without_blood);
    void addMonsterCorpse(const MonsterType *monster, shared_ptr<Tile> t);
    void addBloodTile(shared_ptr<Tile>);
    void setBloodIcon(const Graphic *);

    
    // 
    // placing blood/gore into the map.
    // These are called from Mediator at appropriate points.
    //

    void placeBlood(DungeonMap &, const MapCoord &);
    void placeKnightCorpse(DungeonMap &, const MapCoord &, const Player &, bool blood);
    void placeMonsterCorpse(DungeonMap &, const MapCoord &, const MonsterType &);

    
private:
    void placeNextTile(DungeonMap &dmap, const MapCoord &mc,
                       const std::vector<shared_ptr<Tile> > &tiles);
    void placeTile(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Tile> new_tile);

private:
    // blood tiles
    std::vector<shared_ptr<Tile> > blood_tiles;
    const Graphic *blood_icon;

    // knight corpses
    struct KnightCorpse {
        shared_ptr<Tile> with_blood;
        shared_ptr<Tile> without_blood;
    };
    std::map<const Player*, KnightCorpse> knight_corpses;

    // monster corpses
    std::map<const MonsterType *, std::vector<shared_ptr<Tile> > > monster_corpses;
};

#endif

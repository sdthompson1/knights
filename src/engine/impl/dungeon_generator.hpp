/*
 * dungeon_generator.hpp
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

#ifndef DUNGEON_GENERATOR_HPP
#define DUNGEON_GENERATOR_HPP

#include "boost/shared_ptr.hpp"
using namespace boost;

#include <vector>
using namespace std;

class CoordTransform;
class DungeonLayout;
class DungeonMap;
class HomeManager;
class MonsterManager;
class Player;
class Segment;
class Tile;


// Enums

// H_NONE is used for RandomRespawn. It does not assign players any
// starting homes nor does it guarantee any homes in the dungeon.
//
// The other settings guarantee at least nplayers homes in the
// dungeon, and assign each player an appropriate starting home.
//
// TODO: It might be nice to decouple home assignment from dungeon
// generation, so that lua can take control of the home assignment if
// it wishes.
//
enum HomeType { H_NONE, H_CLOSE, H_AWAY, H_RANDOM };


// Settings used by DungeonGenerator

struct DungeonSettings {
    const DungeonLayout *layout;
    std::vector<boost::shared_ptr<Tile> > wall_tiles, hdoor_tiles, vdoor_tiles;
    std::vector<const Segment*> normal_segments, required_segments;
    HomeType home_type;
};


//
// DungeonGenerator function itself
// Throws DungeonGenerationFailed if something goes wrong.
// NB The present algorithm assumes that all Segments are the same size.
//

void DungeonGenerator(DungeonMap &dmap,
                      CoordTransform &ct,
                      HomeManager &home_manager,
                      MonsterManager &monster_manager,
                      std::vector<boost::shared_ptr<Player> > &players,
                      int nplayers,
                      const DungeonSettings &settings);

#endif

/*
 * gore_manager.cpp
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

#include "dungeon_map.hpp"
#include "gore_manager.hpp"
#include "mediator.hpp"
#include "tile.hpp"

GoreManager::GoreManager()
    : blood_icon(0)
{ }

void GoreManager::setKnightCorpse(const Player &pl, shared_ptr<Tile> with_blood,
                                  shared_ptr<Tile> without_blood)
{
    KnightCorpse kc;
    kc.with_blood = with_blood;
    kc.without_blood = without_blood;
    knight_corpses[&pl] = kc;
}

void GoreManager::addMonsterCorpse(const MonsterType *m, shared_ptr<Tile> c)
{
    monster_corpses[m].push_back(c);
}

void GoreManager::addBloodTile(shared_ptr<Tile> b)
{
    blood_tiles.push_back(b);
}

void GoreManager::setBloodIcon(const Graphic *g)
{
    blood_icon = g;
}


void GoreManager::placeBlood(DungeonMap &dmap, const MapCoord &mc)
{
    Mediator &mediator = Mediator::instance();
    placeNextTile(dmap, mc, blood_tiles);
    mediator.placeIcon(dmap, mc, blood_icon, mediator.cfgInt("blood_icon_duration"));
}

void GoreManager::placeKnightCorpse(DungeonMap &dmap, const MapCoord &mc, const Player &pl,
                                    bool blood)
{
    KnightCorpse kc = knight_corpses[&pl];
    placeTile(dmap, mc, blood ? kc.with_blood : kc.without_blood);
}

void GoreManager::placeMonsterCorpse(DungeonMap &dmap, const MapCoord &mc,
                                     const MonsterType &m)
{
    placeNextTile(dmap, mc, monster_corpses[&m]);
}

void GoreManager::placeNextTile(DungeonMap &dmap, const MapCoord &mc,
                                const vector<shared_ptr<Tile> > &tile_list)
{
    if (tile_list.empty()) return;  // nothing to place

    // work out which tile to place
    shared_ptr<Tile> new_tile = tile_list.front();
    vector<shared_ptr<Tile> > current_tiles;
    dmap.getTiles(mc, current_tiles);
    for (vector<shared_ptr<Tile> >::iterator it = current_tiles.begin();
    it != current_tiles.end(); ++it) {
        vector<shared_ptr<Tile> >::const_iterator i2 = find(tile_list.begin(),
                                                            tile_list.end(), *it);
        if (i2 != tile_list.end()) {
            new_tile = *i2;
            ++i2;
            if (i2 != tile_list.end()) new_tile = *i2;
        }
    }
    placeTile(dmap, mc, new_tile);
}

void GoreManager::placeTile(DungeonMap &dmap, const MapCoord &mc, shared_ptr<Tile> new_tile)
{
    // This removes any necessary existing tiles, then places 'new_tile'.

    if (!new_tile) return;

    vector<shared_ptr<Tile> > tiles;
    dmap.getTiles(mc, tiles);
    for (vector<shared_ptr<Tile> >::iterator tile = tiles.begin(); tile != tiles.end();
    ++tile) {
        // For each tile (*tile) already on the square, look for *tile in the gore lists. If
        // *tile is a gore tile and it has depth less than or equal to the depth of the new
        // gore tile, then it should be deleted. (because we want the new tile to overwrite
        // it.)

        if ((*tile)->getDepth() <= new_tile->getDepth()) {

            if (find(blood_tiles.begin(), blood_tiles.end(), *tile) != blood_tiles.end()) {
                dmap.rmTile(mc, *tile, 0);
                continue;
            } 

            for (map<const Player*, KnightCorpse>::iterator kt = knight_corpses.begin();
            kt != knight_corpses.end(); ++kt) {
                if (kt->second.with_blood == *tile || kt->second.without_blood == *tile) {
                    dmap.rmTile(mc, *tile, 0);
                    continue;
                }
            }

            for (map<const MonsterType *, vector<shared_ptr<Tile> > >::iterator
            vecp = monster_corpses.begin(); vecp != monster_corpses.end(); ++vecp) {
                if (find(vecp->second.begin(), vecp->second.end(), *tile)
                != vecp->second.end()) {
                    dmap.rmTile(mc, *tile, 0);
                    continue;
                }
            }
        }
    }

    // We are now ready to add the new tile.
    // Note -- the tile is shared, rather than copied, because we want to be able to identify
    // it again.... (corpse/blood tiles are assumed not to have any mutable state).
    dmap.addTile(mc, new_tile, 0);
}



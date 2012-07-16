/*
 * monster_manager.cpp
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

#include "dungeon_map.hpp"
#include "mediator.hpp"
#include "monster.hpp"
#include "monster_manager.hpp"
#include "monster_type.hpp"
#include "originator.hpp"
#include "rng.hpp"
#include "tile.hpp"

#include <algorithm>
using namespace std;

MonsterManager::MonsterManager()
    : total_current_monsters(0), total_monster_limit(-1), zombie_chance(0),
      necronomicon_counter(0), necromancy_flag(false),
      monster_respawn_wait(0)
{ }

void MonsterManager::addZombieDecay(shared_ptr<Tile> from, shared_ptr<Tile> to)
{
    if (!from || !to) return;
    decay_sequence.insert(make_pair(from, to));
}

void MonsterManager::addZombieReanimate(shared_ptr<Tile> from, const MonsterType * to)
{
    if (!from || !to) return;
    MonsterInfo m;
    m.monster_type = to;
    m.zombie_mode = true;
    m.chance = 0;
    monster_map.insert(make_pair(from, m));
}

void MonsterManager::addMonsterGenerator(shared_ptr<Tile> from, const MonsterType * to, float chance)
{
    if (!from || !to) return;
    MonsterInfo m;
    m.monster_type = to;
    m.zombie_mode = false;
    m.chance = chance;
    monster_map.insert(make_pair(from, m));
}

void MonsterManager::limitMonster(const MonsterType *type, int max_number)
{
    monster_limit[type] = max_number;
}

void MonsterManager::limitTotalMonsters(int max_number)
{
    total_monster_limit = max_number;
}

namespace {
    // generate a random square within the given box:
    MapCoord GetRandomSquare(int left, int bottom, int right, int top)
    {
        const int x = g_rng.getInt(left, right);
        const int y = g_rng.getInt(bottom, top);
        return MapCoord(x,y);
    }
}

void MonsterManager::doMonsterGeneration(DungeonMap &dmap, int left, int bottom,
                                         int right, int top)
{
    vector<shared_ptr<Tile> > tiles;

    // Update the zombie respawn counters (#152)
    decrementZombieActivityCounters();

    // First of all, we reduce the chances of doing anything in proportion to the
    // number of monsters already in the dungeon:
    if (total_monster_limit > 0
    && g_rng.getBool(total_current_monsters * 1.0f / total_monster_limit)) {
        return;
    }
    
    // We allow five attempts at generating a monster
    // (If generation fails for "technical" reasons, eg we selected a wall square to generate
    // a monster on, then we will be allowed to try again... but only upto five times, to
    // prevent long or infinite loops.)
    for (int attempts=0; attempts<5; ++attempts) {

        // Pick a random square     
        MapCoord mc = GetRandomSquare(left, bottom, right, top);
        if (!dmap.valid(mc)) continue;

        const bool zombie_activity_inhibited = zombieActivityInhibited(mc);
        
        // Get list of tiles at the square:
        dmap.getTiles(mc, tiles);
        
        // Check to see if there are any "decaying" corpses here.
        for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end();
        ++it) {
            map<shared_ptr<Tile>,shared_ptr<Tile> >::iterator d = decay_sequence.find(*it);
            if (d != decay_sequence.end()) {
                // We found a decaying corpse. Roll for zombie activity, and we're done.
                if (rollZombieActivity()) {
                    dmap.rmTile(mc, d->first, Originator(OT_None()));
                    dmap.addTile(mc, d->second, Originator(OT_None()));
                }
                return;
            }
        }

        // Now check for "reanimating" corpses and "monster generator tiles"
        for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end();
        ++it) {
            map<shared_ptr<Tile>,MonsterInfo>::iterator m = monster_map.find((*it)->getOriginalTile());
            if (m != monster_map.end()) {
                
                // Check if we have reached our monster limit.
                // If we have, then try again in a different square (maybe another
                // monster type won't have reached its limit). 
                if (reachedMonsterLimit(m->second.monster_type)) {
                    break;
                }

                // Check if the new monster can be generated here at all. If not, then
                // try again at a different square.
                if (dmap.getAccess(mc, m->second.monster_type->getHeight()) != A_CLEAR) {
                    break;
                }

                if (m->second.zombie_mode) {

                    // If zombie activity is temporarily inhibited, then skip this tile
                    // (i.e. proceed as if it is not really a monster-tile at all)
                    if (zombie_activity_inhibited) {
                        continue;
                    }
                    
                    // Zombie activity:
                    // (i) roll to see if we should do zombie activity at all
                    // (ii) if we should, then remove the corpse tile.
                    // (If the roll fails then we should abort completely -- don't retry, because
                    // we don't want to make more than one zombie activity roll per monster
                    // generation cycle...)
                    if (!rollZombieActivity()) return;
                    dmap.rmTile(mc, *it, Originator(OT_None()));

                } else {

                    // Vampire bats and other "tile-generated" monsters.
                    // also have a dice roll, this is to reduce the vampire bat generation
                    // to sane levels (unless bats of 5 have been selected!).
                    if (!rollTileGeneratedMonster(m->second.chance)) return;
                }
                // create the monster and add it to the map
                addMonsterToMap(*m->second.monster_type, dmap, mc);
                return;
            }
        }

        // If we get here, then this attempt at monster generation failed but we have
        // been allowed another attempt.
    }

    // If we get here then all of the attempts failed... give up.
}

void MonsterManager::doNecromancy(int nzoms, DungeonMap &dmap, int left, int bottom,
                                  int right, int top)
{
    vector<shared_ptr<Tile> > tiles;

    necromancy_flag = true;
    
    // generate up to the given number of zombies
    // (Note: monster limits are ignored during the following procedure.)
    for (int monsters=0; monsters<nzoms; ++monsters) {

        // Limit the number of attempts for each monster
        // (we allow quite a few attempts, since the idea is that the generation
        // should succeed if at all possible...)
        for (int attempts=0; attempts<3*(right-left)*(top-bottom); ++attempts) {
            bool zombie_placed = false;
            
            // Pick a random square
            MapCoord mc = GetRandomSquare(left, bottom, right, top);
            if (!dmap.valid(mc)) continue;

            // Get a list of tiles at the square
            dmap.getTiles(mc, tiles);

            // Look for any "decaying" or "reanimating" tiles. If there are any,
            // zombify them.
            for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end();
            ++it) {

                // Find the original tile (the decay sequence contains
                // pointers to the original tiles, not cloned tiles)
                shared_ptr<Tile> tile = (*it)->getOriginalTile();

                // Chase it through the decay sequence
                while (1) {
                    map<shared_ptr<Tile>, shared_ptr<Tile> >::const_iterator find_it
                        = decay_sequence.find(tile);
                    if (find_it == decay_sequence.end()) break;
                    tile = find_it->second;
                }

                // Now see if we have reached a "reanimating" tile
                const MonsterType * mon_type = 0;
                map<shared_ptr<Tile>, MonsterInfo>::const_iterator find_it
                    = monster_map.find(tile);
                if (find_it != monster_map.end() && find_it->second.zombie_mode) {
                    mon_type = find_it->second.monster_type;
                }
                
                if (mon_type && dmap.getAccess(mc, H_WALKING) == A_CLEAR) {
                    dmap.rmTile(mc, *it, Originator(OT_None()));
                    addMonsterToMap(*mon_type, dmap, mc);
                    zombie_placed = true;
                    break; // Succeeded in placing a zombie!
                }
            }

            if (zombie_placed) break;
        }
    }
}

void MonsterManager::placeMonster(const MonsterType &type, DungeonMap &dmap, const MapCoord &mc, MapDirection facing)
{
    addMonsterToMap(type, dmap, mc)->setFacing(facing);
}

void MonsterManager::subtractMonster(const MonsterType &mt)
{
    // NOTE: This is called from a dtor, therefore MUST NOT raise Lua errors.
    --current_monsters[&mt];
    --total_current_monsters;
}

void MonsterManager::onPlaceMonsterCorpse(const MapCoord &mc, const MonsterType &mt)
{
    // See if this counts as a zombie monster
    bool found = false;
    for (map<shared_ptr<Tile>, MonsterInfo>::const_iterator it = monster_map.begin(); it != monster_map.end(); ++it) {
        if (it->second.monster_type == &mt && it->second.zombie_mode) {
            found = true;
            break;
        }
    }

    if (found) {
        addZombieActivityCounter(mc);
    }
}

void MonsterManager::onPlaceKnightCorpse(const MapCoord &mc)
{
    // Reset the zombie activity counter for this square
    // (we assume knight corpses are always zombifiable).
    addZombieActivityCounter(mc);
}

bool MonsterManager::rollZombieActivity() const
{
    return necronomicon_counter > 0 || g_rng.getBool(zombie_chance);
}

bool MonsterManager::rollTileGeneratedMonster(float chance) const
{
    return g_rng.getBool(chance);
}

bool MonsterManager::reachedMonsterLimit(const MonsterType * m) const
{
    // Check the total monster limit
    if (total_monster_limit >= 0 && total_current_monsters >= total_monster_limit) {
        return true;
    }

    // Check the per-monster-type limit
    map<const MonsterType *, int>::const_iterator lim = monster_limit.find(m);
    map<const MonsterType *, int>::const_iterator cur = current_monsters.find(m);
    int ncur = 0;
    if (cur != current_monsters.end()) ncur = cur->second;
    int nlim = -1;
    if (lim != monster_limit.end()) nlim = lim->second;
    if (nlim >= 0 && ncur >= nlim) {
        return true;
    }

    // We're ok...
    return false;
}


//
// helper function to add monster to map (updates the monster counts also).
//

shared_ptr<Monster> MonsterManager::addMonsterToMap(const MonsterType &mt, DungeonMap &dmap,
                                                    const MapCoord &mc)
{
    shared_ptr<Monster> mnstr = mt.makeMonster(Mediator::instance().getTaskManager());
    mnstr->addToMap(&dmap, mc);

    // add to the monster counts
    ++current_monsters[&mt];
    ++total_current_monsters;

    return mnstr;
}           

//
// zombie activity counters (#152) -- support functions
//

bool MonsterManager::zombieActivityInhibited(const MapCoord &mc) const
{
    return zombie_activity_counters.find(mc) != zombie_activity_counters.end();
}

void MonsterManager::decrementZombieActivityCounters()
{
    std::map<MapCoord, int>::iterator it = zombie_activity_counters.begin();
    while (it != zombie_activity_counters.end()) {
        --(it->second);
        if (it->second <= 0) {
            std::map<MapCoord, int>::iterator it2 = it;
            ++it;
            zombie_activity_counters.erase(it2);
        } else {
            ++it;
        }
    }
}

void MonsterManager::addZombieActivityCounter(const MapCoord &mc)
{
    zombie_activity_counters[mc] = monster_respawn_wait;
}

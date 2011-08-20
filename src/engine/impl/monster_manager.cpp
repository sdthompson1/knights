/*
 * monster_manager.cpp
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
#include "mediator.hpp"
#include "monster.hpp"
#include "monster_manager.hpp"
#include "monster_type.hpp"
#include "rng.hpp"

#include <algorithm>
using namespace std;

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
    monster_map.insert(make_pair(from, m));
}

void MonsterManager::addMonsterGenerator(shared_ptr<Tile> from, const MonsterType * to)
{
    if (!from || !to) return;
    MonsterInfo m;
    m.monster_type = to;
    m.zombie_mode = false;
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

        // Get list of tiles at the square:
        dmap.getTiles(mc, tiles);
        
        // Check to see if there are any "decaying" corpses here.
        for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end();
        ++it) {
            map<shared_ptr<Tile>,shared_ptr<Tile> >::iterator d = decay_sequence.find(*it);
            if (d != decay_sequence.end()) {
                // We found a decaying corpse. Roll for zombie activity, and we're done.
                if (rollZombieActivity()) {
                    dmap.rmTile(mc, d->first, 0);
                    dmap.addTile(mc, d->second, 0);
                }
                return;
            }
        }

        // Now check for "reanimating" corpses and "monster generator tiles"
        for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end();
        ++it) {
            map<shared_ptr<Tile>,MonsterInfo>::iterator m = monster_map.find(*it);
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

                    // Zombie activity:
                    // (i) roll to see if we should do zombie activity at all
                    // (ii) if we should, then remove the corpse tile.
                    // (If the roll fails then we should abort completely -- don't retry, because
                    // we don't want to make more than one zombie activity roll per monster
                    // generation cycle...)
                    if (!rollZombieActivity()) return;
                    dmap.rmTile(mc, *it, 0);

                } else {

                    // Vampire bats also have a dice roll, this is to reduce the vampire bat generation
                    // to sane levels (unless bats of 5 have been selected!). NOTE: If we ever add
                    // other monster types (non-bat, non-zombie) then this may need some refactoring
                    // (won't be able to assume non-zombie == bat...)
                    if (!rollBatActivity()) return;
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
        for (int attempts=0; attempts<60; ++attempts) {
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
                bool zombify = decay_sequence.find(*it) != decay_sequence.end();
                if (!zombify) {
                    map<shared_ptr<Tile>,MonsterInfo>::iterator m = monster_map.find(*it);
                    if (m != monster_map.end() && m->second.zombie_mode) {
                        zombify = true;
                    }
                }

                // Note -- we cheat slightly here because we use placeZombie, rather than
                // using the specific MonsterType from the monster_map. This works, but it
                // relies on the assumption that there is only one kind of zombie.
                if (zombify && dmap.getAccess(mc, H_WALKING) == A_CLEAR) {
                    dmap.rmTile(mc, *it, 0);
                    placeZombie(dmap, mc, MapDirection(g_rng.getInt(0,4)));
                    zombie_placed = true;
                    break; // Succeeded in placing a zombie!
                }
            }

            if (zombie_placed) break;
        }
    }
}

void MonsterManager::subtractMonster(const MonsterType &mt)
{
    --current_monsters[&mt];
    --total_current_monsters;
}

bool MonsterManager::rollZombieActivity() const
{
    return necronomicon_counter > 0 || g_rng.getBool(zombie_chance * 0.01f);
}

bool MonsterManager::rollBatActivity() const
{
    return g_rng.getBool(bat_chance * 0.01f);
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
// manual monster generation
// These are a bit primitive at the moment -- they just look for the first "zombie mode"
// or "non zombie mode" monster. It works at the moment but will obviously need modification
// if any more monster types are added.
//

shared_ptr<Monster> MonsterManager::addMonsterToMap(const MonsterType &mt, DungeonMap &dmap,
                                                    const MapCoord &mc)
{
    shared_ptr<Monster> mnstr = mt.makeMonster(*this, Mediator::instance().getTaskManager());
    mnstr->addToMap(&dmap, mc);

    // add to the monster counts
    ++current_monsters[&mt];
    ++total_current_monsters;

    return mnstr;
}           

void MonsterManager::placeZombie(DungeonMap &dmap, const MapCoord &mc, MapDirection facing)
{
    for (map<shared_ptr<Tile>, MonsterInfo>::iterator it = monster_map.begin();
    it != monster_map.end(); ++it) {
        if (it->second.zombie_mode) {
            addMonsterToMap(*it->second.monster_type, dmap, mc)->setFacing(facing);
            return;
        }
    }
}

void MonsterManager::placeVampireBat(DungeonMap &dmap, const MapCoord &mc)
{
    for (map<shared_ptr<Tile>, MonsterInfo>::iterator it = monster_map.begin();
    it != monster_map.end(); ++it) {
        if (!it->second.zombie_mode) {
            addMonsterToMap(*it->second.monster_type, dmap, mc);
            return;
        }
    }
}

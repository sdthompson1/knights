/*
 * monster_manager.hpp
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

/*
 * Controls zombie activity and the generation of vampire bats.
 *
 */

#ifndef MONSTER_MANAGER_HPP
#define MONSTER_MANAGER_HPP

#include "map_support.hpp"

#include "boost/shared_ptr.hpp"
using namespace boost;

#include <map>
#include <vector>
using namespace std;

class ItemType;
class Tile;
class Monster;
class MonsterType;

class MonsterManager {
public:
    explicit MonsterManager()
        : total_current_monsters(0), total_monster_limit(-1), zombie_chance(0), bat_chance(0),
          necronomicon_counter(0), necromancy_flag(false) { }
    
    // Initialization:
    
    // addZombieDecay: tile "from" is to be changed into tile "to" as
    // a result of zombie activity
    void addZombieDecay(shared_ptr<Tile> from, shared_ptr<Tile> to);

    // addZombieReanimate: tile "from" is to be deleted and replaced
    // with a zombie as a result of zombie activity
    void addZombieReanimate(shared_ptr<Tile> from, const MonsterType * zombie_type);

    // addMonsterGenerator: the given tile will be treated as a
    // "monster generator" (this is used for vampire bats, which are
    // generated at pits).
    void addMonsterGenerator(shared_ptr<Tile> tile, const MonsterType * monster_type);

    // setZombieChance: this must be called to activate the zombie activity.
    // it is set to a number from 0--100 (100 being the highest zombie activity)
    void setZombieChance(int z) { zombie_chance = z; }
    int getZombieChance() const { return zombie_chance; }

    // setBatChance: ditto, but for vampire bats.
    void setBatChance(int v) { bat_chance = v; }
    int getBatChance() const { return bat_chance; }
    
    // Limit the number of a certain monster that can be added to the dungeon
    void limitMonster(const MonsterType * monster_type, int max_number);

    // Limit the *total* number of monsters that can be added to the dungeon
    void limitTotalMonsters(int max_number);

    // AI
    void setZombieAI(const vector<shared_ptr<Tile> > &avoid,
                     const ItemType *hit, const ItemType *fear)
    { zombie_ai_avoid = avoid; zombie_ai_hit = hit; zombie_ai_fear = fear; }
    
    
    //
    // doMonsterGeneration:
    //
    
    // This does one "round" of monster generation. A random tile
    // within the given rectangle (which includes bl but excludes tr)
    // is generated. If it corresponds to a "monster generator" or
    // zombie (decay/reanimate) tile, then the appropriate action is
    // taken, otherwise nothing happens.
    void doMonsterGeneration(DungeonMap &dmap, int left, int bottom, int right, int top);

    // necromancy (Necronomicon effect -- animates a number of zombies within range.)
    void doNecromancy(int nzoms, DungeonMap &dmap, int left, int bottom, int right, int top);
    bool hasNecromancyBeenDone() const { return necromancy_flag; }
    
    
    //
    // manual monster generation
    //
    void placeZombie(DungeonMap &dmap, const MapCoord &mc, MapDirection facing);
    void placeVampireBat(DungeonMap &dmap, const MapCoord &mc);

    //
    // temporary changes to zombie activity (used by Necronomicon)
    //
    void fullZombieActivity() { ++necronomicon_counter; }
    void normalZombieActivity() { --necronomicon_counter; }
    
    
    //
    // Accessor functions
    //
    
    const vector<shared_ptr<Tile> > & getZombieAvoid() const { return zombie_ai_avoid; }
    const ItemType * getZombieHit() const { return zombie_ai_hit; }
    const ItemType * getZombieFear() const { return zombie_ai_fear; }

private:
    // Noncopyable
    void operator=(const MonsterManager &);
    MonsterManager(const MonsterManager &);
    
private:
    friend class Monster;
    void subtractMonster(const MonsterType &mt);    
    
private:
    bool rollZombieActivity() const;
    bool rollBatActivity() const;
    bool reachedMonsterLimit(const MonsterType * m) const;
    shared_ptr<Monster> addMonsterToMap(const MonsterType &mt, DungeonMap &dmap,
                                        const MapCoord &mc);
    
private:
    map<shared_ptr<Tile>, shared_ptr<Tile> > decay_sequence;
    struct MonsterInfo {
        const MonsterType * monster_type;
        bool zombie_mode;
    };
    map<shared_ptr<Tile>, MonsterInfo> monster_map;

    map<const MonsterType *, int> current_monsters;
    int total_current_monsters;
    map<const MonsterType *, int> monster_limit;
    int total_monster_limit;
    int zombie_chance, bat_chance;
    int necronomicon_counter;  // if +ve, should act as if zombie_chance was 100%.
    bool necromancy_flag;  // set true when doNecromancy is called.
    
    // zombie AI
    vector<shared_ptr<Tile> > zombie_ai_avoid;
    const ItemType * zombie_ai_hit;
    const ItemType * zombie_ai_fear;
};

#endif

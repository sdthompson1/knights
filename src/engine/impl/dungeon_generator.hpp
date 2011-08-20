/*
 * dungeon_generator.hpp
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

#ifndef DUNGEON_GENERATOR_HPP
#define DUNGEON_GENERATOR_HPP

#include "dungeon_generation_failed.hpp"
#include "kconfig_fwd.hpp"
#include "map_support.hpp"

#include "boost/shared_ptr.hpp"
using namespace boost;

#include <map>
#include <string>
#include <vector>
using namespace std;

class DungeonMap;
class ItemGenerator;
class ItemType;
class KnightsConfigImpl;
class MenuSelections;
class MonsterManager;
class RandomDungeonLayout;
class Segment;
class SegmentSet;
class Tile;

//
// Usage: create a DungeonGenerator, apply your DungeonDirectives to it, then call "generate".
// NB The present algorithm assumes that all Segments are the same size.
//

enum HomeType { H_NONE, H_CLOSE, H_AWAY, H_RANDOM, H_RANDOM_RESPAWN };
enum ExitType { E_NONE, E_SELF, E_OTHER, E_RANDOM, E_SPECIAL };

class DungeonGenerator {
public:
    // hdoor and vdoor are wooden doors (used to knock through between adjacent segments).
    DungeonGenerator(const SegmentSet &rs, shared_ptr<Tile> wall_,
                     shared_ptr<Tile> hdoor1, shared_ptr<Tile> hdoor2,
                     shared_ptr<Tile> vdoor1, shared_ptr<Tile> vdoor2) 
        : segment_set(rs), wall(wall_), horiz_door_1(hdoor1), horiz_door_2(hdoor2),
          vert_door_1(vdoor1), vert_door_2(vdoor2), rlayout(0), exit_type(E_NONE),
          exit_category(-1),
          home_type(H_NONE), premapped(false), pretrapped(false), nkeys(0),
          vampire_bats(0), zombie_activity(0), total_stuff_weight(0),
          item_respawn_time(-1),
          lockpicks(0), lockpick_init_time(-1), lockpick_interval(-1),
          team_mode(false)
    { }

    // dungeon generation parameters
    // these are typically called by DungeonDirectives.
    void setExitType(ExitType e, int ecat) { exit_type = e; exit_category = ecat; }
    void setHomeType(HomeType h) { home_type = h; }
    void setLayouts(const vector<const RandomDungeonLayout *> &lay) { layouts = lay; }
    void setPremapped(bool p) { premapped = p; }
    void setPretrapped(bool p) { pretrapped = p; }
    void setNumKeys(int n) { nkeys = n; }
    void addRequiredItem(int number, const ItemType &itype);
    void addRequiredSegment(int segment_category)
        { required_segments.push_back(segment_category); }
    void setStuff(int tile_category, int chance, const ItemGenerator *generator, int weight);
    void setVampireBats(int v) { vampire_bats = v; }
    void setZombieActivity(int z) { zombie_activity = z; }
    void addStartingGear(const ItemType &it, const vector<int> &nos) { gears.push_back(make_pair(&it, nos)); }
    void setRespawnItems(const vector<const ItemType *> &items) { respawn_items = items; }
    void setItemRespawnTime(int ms) { item_respawn_time = ms; }
    void setLockpicks(const ItemType *i) { lockpicks = i; }
    void setLockpickSpawn(int it, int i) { lockpick_init_time = it; lockpick_interval = i; }
    void setTeamMode(bool t) { team_mode = t; }
    
    // "generate" routine -- generates the dungeon map.
    void generate(DungeonMap &dmap, int nplayers, bool tutorial_mode);

    // Add vampire bats to an already-generated dungeon map.
    void addVampireBats(DungeonMap &dmap, MonsterManager &mmgr,
                        int nbats_normal, int nbats_guarded_exit);
    
    // query home/exit locations
    int getNumHomes() const { return 2; }
    void getHome(int i, MapCoord &pos, MapDirection &facing_toward_home) const;
    ExitType getExitType() const { return exit_type; }
    void getExit(int i, MapCoord &pos, MapDirection &facing_toward_exit) const;
    int getNumHomesOverall() const { return unassigned_homes.size() + assigned_homes.size(); }
    void getHomeOverall(int i, MapCoord &pos, MapDirection &facing_toward_home) const;
    
    // query "incidental" data
    // Note: Should maybe factor these out into a separate class (MiscGameSettings?) but that is slightly
    // difficult because of the way the "dungeon directives" system works. For now it's more convenient
    // to store these settings here, even though they have nothing to do with dungeon generation.
    int getVampireBats() const { return vampire_bats; }
    int getZombieActivity() const { return zombie_activity; }
    bool isPremapped() const { return premapped; }
    const vector<pair<const ItemType*, vector<int> > > & getStartingGears() const { return gears; }
    const vector<const ItemType*> & getRespawnItems() const { return respawn_items; }
    int getItemRespawnTime() const { return item_respawn_time; }
    const ItemType * getLockpicks() const { return lockpicks; }
    int getLockpickInitialTime() const { return lockpick_init_time; }
    int getLockpickInterval() const { return lockpick_interval; }
    bool getTeamMode() const { return team_mode; }
    
private:
    struct BlockInfo {
        int x, y;
        bool special;
    };
    struct IsSpecial {
        bool operator()(const BlockInfo &b) { return b.special; }
    };

    void fetchEdge(int&,int&);
    void fetchEdgeOrBlock(int&,int&);
    void setHomeSegment(int,int,int,int);
    void setSpecialSegment(int,int,int);
    void doLayout(int);
    static void shiftHomes(vector<pair<MapCoord,MapDirection> > &, int, int);
    void chopTopSide();
    void chopBottomSide();
    void chopLeftSide();
    void chopRightSide();
    void compress();
    void copySegmentsToMap(DungeonMap&);
    void fillWithWalls(DungeonMap&, const MapCoord&, int, int);
    bool placeDoor(DungeonMap&, const MapCoord &, const MapCoord &, const MapCoord &,
                   const MapCoord &, const MapCoord &, shared_ptr<Tile>, shared_ptr<Tile>);
    void knockThroughDoors(DungeonMap&);
    void generateExits();
    void generateRequiredItems(DungeonMap &);
    bool forbidden(int) const;
    void generateStuff(DungeonMap &);
    void checkTutorial(DungeonMap &);

    static int findItemCategory(const DungeonMap &, const MapCoord &,
                                const vector<shared_ptr<Tile> > &);
    static void placeItem(DungeonMap &, const MapCoord &, vector<shared_ptr<Tile> > &,
                          const ItemType &, int no);
    void generateLocksAndTraps(DungeonMap &, int);

    bool getHorizExit(int x, int y) { return horiz_exits[y*(lwidth-1)+x]; }
    bool getVertExit(int x, int y) { return horiz_exits[y*lwidth+x]; }

    void placeRegularBats(DungeonMap &dmap, MonsterManager &mmgr, int nbats);
    void placeGuardedBats(DungeonMap &dmap, MonsterManager &mmgr, int nbats,
                          shared_ptr<Tile> bat_tile, int left, int bottom,
                          int right, int top);
    void checkConnectivity(DungeonMap &dmap, const MapCoord &from_where, int num_keys);
    
private:
    // "fixed" data
    const SegmentSet &segment_set;
    shared_ptr<Tile> wall, horiz_door_1, horiz_door_2, vert_door_1, vert_door_2;
    const RandomDungeonLayout *rlayout; // currently selected layout
    vector<const RandomDungeonLayout*> layouts;
    ExitType exit_type;
    int exit_category;
    HomeType home_type;
    bool premapped; 
    bool pretrapped;
    int nkeys;
    vector<int> required_segments;  // contains segment categories
    vector<const ItemType*> required_items; // contains only non-null pointers
    int vampire_bats, zombie_activity;
    struct StuffInfo {
        int chance;
        int weight;   // actually max(input weight, 0)
        const ItemGenerator *generator;
        bool forbid;  // true if input weight was <0
    };
    map<int,StuffInfo> stuff;  // LHS is the tile category.
    int total_stuff_weight;
    vector<pair<const ItemType *, vector<int> > > gears; // Starting gear.
    vector<const ItemType*> respawn_items;
    int item_respawn_time;
    const ItemType * lockpicks;
    int lockpick_init_time;
    int lockpick_interval;
    bool team_mode;
    
    // "temporary" data (set during dungeon generation)
    vector<BlockInfo> blocks, edges;  // layout info.
    vector<bool> horiz_exits, vert_exits;  // more layout info
    vector<const Segment *> segments;            // actual segments chosen
    vector<int> segment_categories;              // the category of each chosen segment.
    int lwidth, lheight;       // layout dimensions (in segments)
    int rwidth, rheight;       // segment dimensions (in tiles)
    vector<pair<MapCoord,MapDirection> > unassigned_homes, assigned_homes, exits;   // homes
};


class DungeonDirective {
public:
    virtual ~DungeonDirective() { }

    // this pops an argument list from the kfile.
    static DungeonDirective * create(const string &name, KnightsConfigImpl &kc);

    // "apply" requires the MenuSelections object (since some DungeonDirectives are tied
    // to the value of certain menu options; eg DungeonItem(i_gem, GetMenu("gems_needed")).)
    virtual void apply(DungeonGenerator &dg, const MenuSelections &) const = 0;
};

#endif

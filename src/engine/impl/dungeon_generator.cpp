/*
 * dungeon_generator.cpp
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

#include "coord_transform.hpp"
#include "dungeon_generator.hpp"
#include "dungeon_layout.hpp"
#include "dungeon_map.hpp"
#include "item.hpp"
#include "item_generator.hpp"
#include "knights_config_impl.hpp"
#include "lockable.hpp"
#include "menu_int.hpp"
#include "menu_selections.hpp"
#include "monster_manager.hpp"
#include "monster_type.hpp"
#include "rng.hpp"
#include "room_map.hpp"
#include "segment.hpp"
#include "tile.hpp"

#include "kfile.hpp"
using namespace KConfig;

#ifdef lst2
#undef lst2
#endif

namespace {
    void GenerateRandomTransform(bool &x_reflect, int &nrot)
    {
        x_reflect = g_rng.getBool();
        nrot = g_rng.getInt(0, 4);
    }

    bool FindSpecialExit(const Segment &seg, bool x_reflect, int nrot, HomeInfo &hi)
    {
        const std::vector<HomeInfo> & homes = seg.getHomes(x_reflect, nrot);
        for (std::vector<HomeInfo>::const_iterator it = homes.begin(); it != homes.end(); ++it) {
            if (it->special_exit) {
                hi = *it;
                return true;
            }
        }
        return false;
    }        
}

//
// DungeonDirectives
//

class DungeonExit : public DungeonDirective {
public:
    explicit DungeonExit(ExitType e, int rc) : et(e), rcat(rc) { }
    virtual void apply(DungeonGenerator &dg, const MenuSelections &) const
        { dg.setExitType(et, rcat); }
private:
    ExitType et;
    int rcat;
};

class DungeonGear : public DungeonDirective {
public:
    DungeonGear(const ItemType& it, const vector<int> &nos) : itype(it), numbers(nos) { }
    virtual void apply(DungeonGenerator &dg, const MenuSelections &) const
        { dg.addStartingGear(itype, numbers); }
private:
    const ItemType &itype;
    vector<int> numbers;
};

class DungeonHome : public DungeonDirective {
public:
    explicit DungeonHome(HomeType h) : ht(h) { }
    virtual void apply(DungeonGenerator &dg, const MenuSelections &) const
        { dg.setHomeType(ht); }
private:
    HomeType ht;
};

class DungeonInitialMonsters : public DungeonDirective {
public:
    DungeonInitialMonsters(const MonsterType *m, const MenuInt &n) : mtype(m), number(n) { }
    virtual void apply(DungeonGenerator &dg, const MenuSelections &ms) const
    {
        // Initially there are 0 monsters if n == 0, otherwise 2*n + 1 monsters.
        const int n = number.getValue(ms);
        dg.setInitialMonsters(mtype, n == 0 ? 0 : 2*n + 1);
    }
private:
    const MonsterType * mtype;
    const MenuInt & number;
};

class DungeonItem : public DungeonDirective {
public:
    DungeonItem(const MenuInt &no, const ItemType &it) : number(no), itype(it) { }
    virtual void apply(DungeonGenerator &dg, const MenuSelections &ms) const
        { dg.addRequiredItem(number.getValue(ms), itype); }
private:
    const MenuInt &number;
    const ItemType &itype;
};

class DungeonKeys : public DungeonDirective {
public:
    DungeonKeys(KnightsConfigImpl &kc, KFile::List &lst);
    virtual void apply(DungeonGenerator &dg, const MenuSelections &) const;
private:
    vector<const ItemType *> keys;
};

DungeonKeys::DungeonKeys(KnightsConfigImpl &kc, KFile::List &lst)
{
    if (lst.getSize() < 2) return;
    for (int i=0; i<lst.getSize(); ++i) {
        lst.push(i);
        keys.push_back(kc.popItemType());
    }
}

void DungeonKeys::apply(DungeonGenerator &dg, const MenuSelections &) const
{
    if (keys.size()<2) {
        dg.setNumKeys(0);
    } else {
        dg.setNumKeys(keys.size()-1);
        for (int i=0; i<keys.size(); ++i) {
            // require one of each key plus one set of lock picks
            if (keys[i]) dg.addRequiredItem(1, *keys[i]);
        }
    }

    if (keys.size() >= 1) dg.setLockpicks(keys[0]);
}

class DungeonLayoutDir : public DungeonDirective { 
public:
    void addLayout(const RandomDungeonLayout *r) { layouts.push_back(r); }
    virtual void apply(DungeonGenerator &dg, const MenuSelections &) const
        { dg.setLayouts(layouts); }
private:
    vector<const RandomDungeonLayout *> layouts;
};

// This controls auto spawning of lockpicks
// The lockpick item is assumed to be the first item supplied to DungeonKeys.
class DungeonLockpickSpawn : public DungeonDirective {
public:
    DungeonLockpickSpawn(int init_time_, int interval_)
        : init_time(init_time_), interval(interval_) { }
    virtual void apply(DungeonGenerator &dg, const MenuSelections &) const
        { dg.setLockpickSpawn(init_time, interval); }
private:
    int init_time;  // time before starting to generate lockpicks, in ms
    int interval;   // interval between generations, in ms
};

class DungeonMonsterGeneration : public DungeonDirective {
public:
    explicit DungeonMonsterGeneration(const MenuInt &m) : monster_level(m) { }
    virtual void apply(DungeonGenerator &dg, const MenuSelections &ms) const
    { dg.setTileGeneratedMonsterLevel(monster_level.getValue(ms)); }
private:
    const MenuInt & monster_level;
};

class DungeonMonsterLimit : public DungeonDirective {
public:
    DungeonMonsterLimit(const MonsterType *m, const MenuInt &n) : mtype(m), number(n) { }
    virtual void apply(DungeonGenerator &dg, const MenuSelections &ms) const
    {
        // Limit to 3*n monsters
        dg.setMonsterLimit(mtype, 3 * number.getValue(ms));
    }
private:
    const MonsterType *mtype;
    const MenuInt &number;
};

class DungeonPremapped : public DungeonDirective {
public:
    virtual void apply(DungeonGenerator &dg, const MenuSelections &) const
        { dg.setPremapped(true); }
};

class DungeonPretrapped : public DungeonDirective {
public:
    virtual void apply(DungeonGenerator &dg, const MenuSelections &) const
        { dg.setPretrapped(true); }
};

class DungeonRespawnItems : public DungeonDirective {
public:
    explicit DungeonRespawnItems(std::vector<const ItemType *> &i) : items(i) { }
    virtual void apply(DungeonGenerator &dg, const MenuSelections &) const
        { dg.setRespawnItems(items); }
private:
    std::vector<const ItemType*> items;
};

class DungeonRespawnTime : public DungeonDirective {
public:
    explicit DungeonRespawnTime(int t) : milliseconds(t) { }
    virtual void apply(DungeonGenerator &dg, const MenuSelections &) const
        { dg.setItemRespawnTime(milliseconds); }
private:
    int milliseconds;
};

class DungeonSegment : public DungeonDirective {
public:
    explicit DungeonSegment(int r) : segment_cat(r) { }
    virtual void apply(DungeonGenerator &dg, const MenuSelections &) const
        { dg.addRequiredSegment(segment_cat); }
private:
    int segment_cat;
};

class DungeonStuff : public DungeonDirective {
public:
    DungeonStuff(int tc, float ch, const ItemGenerator *ig_, int wt)
        : tile_cat(tc), chance(ch), ig(ig_), weight(wt) { }
    virtual void apply(DungeonGenerator &dg, const MenuSelections &) const
        { dg.setStuff(tile_cat, chance, ig, weight); }
private:
    int tile_cat;
    float chance;
    const ItemGenerator *ig;
    int weight;
};

class DungeonZombies : public DungeonDirective { 
public:
    // sets the zombie activity level
    explicit DungeonZombies(const MenuInt &a) : activity(a) { }
    virtual void apply(DungeonGenerator &dg, const MenuSelections &ms) const
        { dg.setZombieActivity(activity.getValue(ms)); }
private:
    const MenuInt &activity;
};



//
// DungeonDirective::create
//

DungeonDirective * DungeonDirective::create(const string &name, KnightsConfigImpl &kc)
{
    if (!kc.getKFile()) return 0;

    if (name.substr(0,7) != "Dungeon") {
        kc.getKFile()->errExpected("MenuDirective");
        return 0;
    }
    string n = name.substr(7);

    if (n == "Zombies") {
        KFile::List lst(*kc.getKFile(), "", 1);
        lst.push(0);
        const MenuInt *number = kc.popMenuInt();
        if (!number) kc.getKFile()->errExpected("integer or GetMenu directive");
        return new DungeonZombies(*number);

    } else if (n == "Exit") {
        KFile::List lst(*kc.getKFile(), "", 1);
        lst.push(0);
        string x = kc.getKFile()->popString();
        if (x == "same_as_entry") return new DungeonExit(E_SELF,0);
        else if (x == "others_entry") return new DungeonExit(E_OTHER,0);
        else if (x == "total_random") return new DungeonExit(E_RANDOM,0);
        else {
            int rcat = kc.getSegmentCategory(x);
            if (rcat >= 0) {
                return new DungeonExit(E_SPECIAL, rcat);
            } else {
                kc.getKFile()->errExpected("exit type");
            }
        }

    } else if (n == "Gear") {
        KFile::List lst(*kc.getKFile(), "", 2);

        lst.push(0);
        const ItemType *const itype = kc.popItemType();
        if (!itype) kc.getKFile()->errExpected("item type");

        lst.push(1);
        KFile::List lst2(*kc.getKFile(), "item quantities");
        if (lst2.getSize() < 1) kc.getKFile()->error("empty quantity list for DungeonGear");

        vector<int> nos(lst2.getSize());
        for (int i=0; i<lst2.getSize(); ++i) {
            lst2.push(i);
            nos[i] = kc.getKFile()->popInt();
        }

        return new DungeonGear(*itype, nos);

    } else if (n == "Home") {
        KFile::List lst(*kc.getKFile(), "", 1);
        lst.push(0);
        string x = kc.getKFile()->popString();
        if (x == "total_random") return new DungeonHome(H_RANDOM);
        else if (x == "close_to_other") return new DungeonHome(H_CLOSE);
        else if (x == "away_from_other") return new DungeonHome(H_AWAY);
        else if (x == "random_respawn") return new DungeonHome(H_RANDOM_RESPAWN);
        else if (x == "different_every_time") return new DungeonHome(H_DIFFERENT_EVERY_TIME);
        else kc.getKFile()->errExpected("entry type");

    } else if (n == "InitialMonsters") {
        KFile::List lst(*kc.getKFile(), "", 2);
        lst.push(0);
        const MonsterType * mtype = kc.popMonsterType();
        lst.push(1);
        const MenuInt *number = kc.popMenuInt();
        return new DungeonInitialMonsters(mtype, *number);
        
    } else if (n == "Item") {
        KFile::List lst(*kc.getKFile(), "", 2);
        lst.push(0);
        const MenuInt *number = kc.popMenuInt();
        if (!number) kc.getKFile()->errExpected("integer or GetMenu directive");
        lst.push(1);
        const ItemType *const itype = kc.popItemType();
        if (!itype) kc.getKFile()->errExpected("item type");
        return new DungeonItem(*number, *itype);

    } else if (n == "Keys") {
        KFile::List lst(*kc.getKFile(), "");
        return new DungeonKeys(kc, lst);

    } else if (n == "Layout") {
        KFile::List lst(*kc.getKFile(), "");
        DungeonLayoutDir * dir = new DungeonLayoutDir;
        for (int i=0; i<lst.getSize(); ++i) {
            lst.push(i);
            dir->addLayout( kc.popRandomDungeonLayout() );
        }
        return dir;

    } else if (n == "LockpickSpawn") {
        KFile::List lst(*kc.getKFile(), "", 2);
        lst.push(0);
        const int init_time = kc.getKFile()->popInt();
        lst.push(1);
        const int interval = kc.getKFile()->popInt();
        return new DungeonLockpickSpawn(init_time, interval);

    } else if (n == "MonsterGeneration") {
        KFile::List lst(*kc.getKFile(), "", 1);
        lst.push(0);
        const MenuInt *number = kc.popMenuInt();
        return new DungeonMonsterGeneration(*number);
        
    } else if (n == "MonsterLimit") {
        KFile::List lst(*kc.getKFile(), "", 2);
        lst.push(0);
        const MonsterType * mtype = kc.popMonsterType();
        lst.push(1);
        const MenuInt *number = kc.popMenuInt();
        return new DungeonMonsterLimit(mtype, *number);
        
    } else if (n == "Premapped") {
        KFile::List lst(*kc.getKFile(), "", 0);
        return new DungeonPremapped;
        
    } else if (n == "Pretrapped") {
        KFile::List lst(*kc.getKFile(), "", 0);
        return new DungeonPretrapped;

    } else if (n == "RespawnItems") {
        KFile::List lst(*kc.getKFile(), "", 1);
        lst.push(0);
        KFile::List lst2(*kc.getKFile(), "items to respawn");
        std::vector<const ItemType*> items;
        for (int i = 0; i < lst2.getSize(); ++i) {
            lst2.push(i);
            const ItemType *itype = kc.popItemType();
            if (!itype) kc.getKFile()->errExpected("item type");
            items.push_back(itype);
        }
        return new DungeonRespawnItems(items);

    } else if (n == "RespawnTime") {
        KFile::List lst(*kc.getKFile(), "", 1);
        lst.push(0);
        const int milliseconds = kc.getKFile()->popInt();
        return new DungeonRespawnTime(milliseconds);
        
    } else if (n == "Segment") {
        KFile::List lst(*kc.getKFile(), "", 1);
        lst.push(0);
        string rname = kc.getKFile()->popString();
        int rcat = kc.getSegmentCategory(rname);
        if (rcat >= 0) return new DungeonSegment(rcat);
    
    } else if (n == "Stuff") {
        KFile::List lst(*kc.getKFile(), "", 3, 4);
        lst.push(0);
        string tname = kc.getKFile()->popString();
        int tcat = kc.getTileCategory(tname);
        lst.push(1);
        float prob = kc.popProbability();
        lst.push(2);
        ItemGenerator *ig = kc.popItemGenerator();
        lst.push(3);
        int wt = kc.getKFile()->popInt(0);
        return new DungeonStuff(tcat, prob, ig, wt);

    } else {
        kc.getKFile()->errExpected("MenuDirective");
    }

    return 0;
}


//
// The dungeon generator itself
//

void DungeonGenerator::addRequiredItem(int number, const ItemType &itype)
{
    for (int i=0; i<number; ++i) {
        required_items.push_back(&itype);
    }
}

void DungeonGenerator::setStuff(int tile_category, float chance, const ItemGenerator *generator,
                                int weight)
{
    if (stuff.find(tile_category) != stuff.end()) {
        total_stuff_weight -= stuff[tile_category].weight;
    }
    StuffInfo si;
    si.chance = chance;
    si.generator = generator;
    si.weight = std::max(0, weight);
    si.forbid = weight < 0;
    total_stuff_weight += si.weight;
    stuff[tile_category] = si;
}

void DungeonGenerator::setMonsterLimit(const MonsterType *m, int max_monsters)
{
    monster_limits[m] = max_monsters;
}

void DungeonGenerator::setInitialMonsters(const MonsterType *m, int count)
{
    initial_monsters[m] = count;
}

std::string DungeonGenerator::generate(DungeonMap &dmap, MonsterManager &monster_manager,
                                       CoordTransform &ct, int nplayers, bool tutorial_mode)
{
    // If there is an exception of any kind we just re-try the
    // generation. If it still fails after N attempts (e.g. because
    // the dungeon size is too small for the quest) then we re-try
    // with the next possible layout. If we run out of layouts the
    // exception is re-thrown.

    const int MAX_ATTEMPTS_PER_LAYOUT = 50;
    
    for (vector<const RandomDungeonLayout*>::const_iterator i = layouts.begin();
    i != layouts.end(); ++i) {
        for (int j = 0; j < MAX_ATTEMPTS_PER_LAYOUT; ++j) {
            rlayout = *i;

            try {
                // clear everything...
                blocks.clear();
                edges.clear();
                horiz_exits.clear();
                vert_exits.clear();
                segments.clear();
                segment_categories.clear();
                segment_x_reflect.clear();
                segment_nrot.clear();
                lwidth = lheight = rwidth = rheight = 0;
                unassigned_homes.clear();
                assigned_homes.clear();
                exits.clear();
                ct.clear();

                // create the layout
                doLayout(nplayers);    // lays out the segments, and 'assigns' homes and special exit point.
                compress();            // deletes any unused space around the edges of the map.

                // create the DungeonMap itself
                dmap.create(lwidth*(rwidth+1)+1, lheight*(rheight+1)+1);
                copySegmentsToMap(dmap, monster_manager, ct);         // copies the segments into the map.
                knockThroughDoors(dmap);      // creates doorways between the different segments.

                // fill in exits
                generateExits();
    
                // lock doors and chests
                generateLocksAndTraps(dmap, nkeys);
    
                // generate items
                generateRequiredItems(dmap);
                generateStuff(dmap);

                // Generate initial monsters
                for (map<const MonsterType *, int>::const_iterator it = initial_monsters.begin(); it != initial_monsters.end(); ++it) {
                    placeInitialMonsters(dmap, monster_manager, *it->first, it->second);
                }
                
                // Set monster limits
                // Note: we only set the individual monster limits here. The total monsters limit is set in KnightsConfigImpl::initializeGame.
                for (map<const MonsterType *, int>::const_iterator it = monster_limits.begin(); it != monster_limits.end(); ++it) {
                    monster_manager.limitMonster(it->first, it->second);
                }
                
                // Check that all keys/lockpicks are accessible.
                // We do this check separately for each of the player homes.
                for (vector<pair<MapCoord,MapDirection> >::const_iterator it = assigned_homes.begin(); it != assigned_homes.end(); ++it) {
                    const MapCoord & home_location = DisplaceCoord(it->first, Opposite(it->second));
                    checkConnectivity(dmap, home_location, nkeys);
                }

                if (tutorial_mode) {
                    // Check that this map is acceptable for a tutorial
                    checkTutorial(dmap);
                }
                
                // If we get here then we have had a successful dungeon generation.
                std::string warning_msg;
                if (i != layouts.begin()) {
                    warning_msg = "Could not generate a \"" + (*layouts.begin())->getName() + "\" dungeon for this quest."
                        " Using \"" + (*i)->getName() + "\" instead.";
                }
                return warning_msg;

            } catch (const DungeonGenerationFailed &) {
                vector<const RandomDungeonLayout*>::const_iterator i2 = i;
                ++i2;
                if (i2 == layouts.end() && j == MAX_ATTEMPTS_PER_LAYOUT-1) {
                    // Give up.
                    throw;
                }
                // Otherwise we go on to the next attempt
            }
            // (Other exceptions are allowed to propagate upwards)
        }
    }

    throw DungeonGenerationFailed();  // Shouldn't get here
}


void DungeonGenerator::fetchEdge(int &x, int &y)
{
    if (edges.empty()) throw DungeonGenerationFailed();
    x = edges.back().x;
    y = edges.back().y;
    edges.pop_back();
}

void DungeonGenerator::fetchEdgeOrBlock(int &x, int &y)
{
    // This fetches an edge (preferably), or else a block.
    if (!edges.empty()) {
        x = edges.back().x;
        y = edges.back().y;
        edges.pop_back();
    } else if (!blocks.empty()) {
        x = blocks.back().x;
        y = blocks.back().y;
        blocks.pop_back();
    } else {
        throw DungeonGenerationFailed();
    }
}

void DungeonGenerator::setHomeSegment(int x, int y, int minhomes, int assign)
{
    const int max_attempts = 50;

    if (y < 0 || y >= lheight || x < 0 || x >= lwidth) return;

    // generate a segment and store it into the "segments" vector
    const Segment *r = 0;
    for (int i=0; i<max_attempts; ++i) {
        r = segment_set.getHomeSegment(minhomes);
        if (find(segments.begin(), segments.end(), r) == segments.end()) break;
    }
    if (!r) throw DungeonGenerationFailed();
    segments[y*lwidth+x] = r;

    // Trac #41. Generate random rotation/reflection for the segment
    bool rand_reflect;
    GenerateRandomTransform(rand_reflect,
                            segment_nrot[y*lwidth+x]);
    segment_x_reflect[y*lwidth+x] = rand_reflect;

    // copy homes to appropriate lists (either assigned or unassigned).
    if (r) {
        rwidth = r->getWidth();
        rheight = r->getHeight();
        const int xbase = x*(rwidth+1)+1;
        const int ybase = y*(rheight+1)+1;
        vector<HomeInfo> h(r->getHomes(segment_x_reflect[y*lwidth+x], segment_nrot[y*lwidth+x]));
        RNG_Wrapper myrng(g_rng);
        random_shuffle(h.begin(), h.end(), myrng);
        for (int i=0; i<h.size(); ++i) {
            MapCoord mc(h[i].x + xbase, h[i].y + ybase);
            MapDirection facing(h[i].facing);
            if (i < assign) {
                assigned_homes.push_back(make_pair(mc,facing));
            } else {
                unassigned_homes.push_back(make_pair(mc,facing));
            }
        }
    }
}

void DungeonGenerator::setSpecialSegment(int x, int y, int category)
{
    // homes are ignored this time. we just generate the segment.
    const int max_attempts = 50;
    if (y < 0 || y >= lheight || x < 0 || x >= lwidth) return;
    const Segment *r = 0;
    for (int i=0; i<max_attempts; ++i) {
        r = segment_set.getSpecialSegment(category);
        if (find(segments.begin(), segments.end(), r) == segments.end()) break;
    }
    if (!r) throw DungeonGenerationFailed();
    segments[y*lwidth + x] = r;
    segment_categories[y*lwidth+x] = category;

    // Trac #41. Generate random rotation/reflection for the segment
    bool rand_reflect;
    GenerateRandomTransform(rand_reflect,
                            segment_nrot[y*lwidth + x]);
    segment_x_reflect[y*lwidth + x] = rand_reflect;
}

void DungeonGenerator::doLayout(int nplayers)
{
    RNG_Wrapper myrng(g_rng);

    int x, y;
    
    // Require nplayers homes normally, but 0 for 'random_respawn' mode.
    // Also require at least 1 more if 'total random exit' selected (Trac #75)
    const int homes_required = (home_type == H_RANDOM_RESPAWN ? 0 : nplayers) + (exit_type == E_RANDOM ? 1 : 0);
    
    // randomize the layout first:
    auto_ptr<DungeonLayout> layout = rlayout->choose(lua);
    
    // get width and height
    lwidth = layout->getWidth();
    lheight = layout->getHeight();

    // work out where the edges and blocks are.
    // flip and/or rotate the layout if necessary.
    const bool flipx = g_rng.getBool();
    const bool flipy = g_rng.getBool();
    const bool rotate = g_rng.getBool();
    for (int i=0; i<lwidth; ++i) {
        for (int j=0; j<lheight; ++j) {
            x=i;
            y=j;
            if (flipx) x = lwidth-1-x;
            if (flipy) y = lheight-1-y;
            if (rotate) {
                const int tmp = y;
                y = x;
                x = lheight-1-tmp;
            }

            BlockInfo bi;
            bi.x = x;
            bi.y = y;
            bi.special = false;
            switch (layout->getBlockType(i,j)) {
            case BT_NONE:
                // Don't add anything here :)
                break;
            case BT_BLOCK:
                blocks.push_back(bi);
                break;
            case BT_SPECIAL:
                bi.special = true;
                // fall through
            case BT_EDGE:
                edges.push_back(bi);
                break;
            }
        }
    }

    const int new_lwidth = rotate? lheight : lwidth;
    const int new_lheight = rotate? lwidth : lheight;

    // exits information also needs to be flipped and/or rotated
    vert_exits.resize(new_lwidth*(new_lheight-1));
    horiz_exits.resize((new_lwidth-1)*new_lheight);
    for (int i=0; i<lwidth-1; ++i) {
        for (int j=0; j<lheight; ++j) {
            x=i;
            y=j;
            if (flipx) x = lwidth-2-x;
            if (flipy) y = lheight-1-y;
            if (rotate) {
                // ynew = x
                // xnew = lheight-1-y
                vert_exits[x*new_lwidth + (lheight-1-y)] = layout->hasHorizExit(i,j);
            } else {
                horiz_exits[y*(new_lwidth-1) + x] = layout->hasHorizExit(i,j);
            }
        }
    }
    for (int i=0; i<lwidth; ++i) {
        for (int j=0; j<lheight-1; ++j) {
            x=i;
            y=j;
            if (flipx) x = lwidth-1-x;
            if (flipy) y = lheight-2-y;
            if (rotate) {
                // ynew = x
                // xnew = lheight-2-y
                horiz_exits[x*(new_lwidth-1) + (lheight-2-y)] = layout->hasVertExit(i,j);
            } else {
                vert_exits[y*new_lwidth + x] = layout->hasVertExit(i,j);
            }
        }
    }

    // randomize blocks and edges
    // set new width and height
    // resize segments array.
    random_shuffle(blocks.begin(), blocks.end(), myrng);
    random_shuffle(edges.begin(), edges.end(), myrng);
    lwidth = new_lwidth;
    lheight = new_lheight;
    segments.resize(lwidth*lheight);
    segment_categories.resize(lwidth*lheight);
    segment_x_reflect.resize(lwidth*lheight);
    segment_nrot.resize(lwidth*lheight);
    fill(segment_categories.begin(), segment_categories.end(), -1);

    // "Away From Other" homes, on Edges
    if (home_type == H_AWAY) {
        for (int i=0; i<nplayers; ++i) {
            fetchEdge(x, y);
            setHomeSegment(x, y, 1, 1);
        }
    }

    // "DungeonSegment()" segments -- on Edges if possible, Blocks otherwise.
    // NOTE that homes in these segments will not be added to the home-lists. This affects
    // guarded exit points only (at time of writing)... and these are dealt with separately
    // (see generateExits.)
    for (vector<int>::const_iterator it = required_segments.begin();
    it != required_segments.end(); ++it) {
        fetchEdgeOrBlock(x, y);
        setSpecialSegment(x, y, *it);
    }

    // Eliminate "special" tiles if they have not been assigned.
    // Also no further distinction between Edges and Blocks beyond this point.
    edges.erase(remove_if(edges.begin(), edges.end(), IsSpecial()), edges.end());
    copy(edges.begin(), edges.end(), back_inserter(blocks));
    edges.clear();

    // "Close To Other" homes
    if (home_type == H_CLOSE) {
        fetchEdgeOrBlock(x, y);
        setHomeSegment(x, y, nplayers, nplayers);
    }

    // Fill in all remaining blocks.
    while (!blocks.empty()) {
        const int h = homes_required - assigned_homes.size() - unassigned_homes.size();  // minimum number of homes still required
        const int nblocks = int(blocks.size());
        int n;  // *minimum* number of homes to generate in the new block
        if (h > 2*nblocks) throw DungeonGenerationFailed();
        else if (h == 2*nblocks) n = 2;  // need 2 in every block
        else if (h == 2*nblocks-1) n = 1;  // need at least one here plus two in all others
        else n = 0;  // can get away with 0 here if we want to
        fetchEdgeOrBlock(x, y);
        setHomeSegment(x, y, n, 0);
    }

    // Make sure there are enough homes in the "assigned" list.
    // (exception: if using H_RANDOM_RESPAWN, homes are irrelevant, so no need for this check)
    random_shuffle(assigned_homes.begin(), assigned_homes.end(), myrng);
    random_shuffle(unassigned_homes.begin(), unassigned_homes.end(), myrng);
    while (assigned_homes.size() < nplayers && home_type != H_RANDOM_RESPAWN) {
        if (unassigned_homes.empty()) {
            // this can happen eg if you ask for gnome room on a 1x1 map... there won't be
            // any space left for the homes!!
            throw DungeonGenerationFailed();
        }
        assigned_homes.push_back(unassigned_homes.back());
        unassigned_homes.pop_back();
    }
}

void DungeonGenerator::shiftHomes(vector<pair<MapCoord,MapDirection> > &homes, int dx, int dy)
{
    for (int i=0; i<homes.size(); ++i) {
        homes[i].first.setX(homes[i].first.getX() + dx);
        homes[i].first.setY(homes[i].first.getY() + dy);
    }
}

void DungeonGenerator::chop(int xofs, int yofs, int new_lwidth, int new_lheight)
{
    vector<const Segment *> new_segments(new_lwidth * new_lheight);
    vector<int> new_cats(new_lwidth * new_lheight);
    vector<bool> new_x_reflect(new_lwidth * new_lheight);
    vector<int> new_nrot(new_lwidth * new_lheight);

    for (int x = 0; x < new_lwidth; ++x) {
        for (int y = 0; y < new_lheight; ++y) {

            const int new_idx = y * new_lwidth + x;
            const int old_idx = (y + yofs) * lwidth + (x + xofs);

            new_segments[new_idx] = segments[old_idx];
            new_cats[new_idx] = segment_categories[old_idx];
            new_x_reflect[new_idx] = segment_x_reflect[old_idx];
            new_nrot[new_idx] = segment_nrot[old_idx];
        }
    }

    segments.swap(new_segments);
    segment_categories.swap(new_cats);
    segment_x_reflect.swap(new_x_reflect);
    segment_nrot.swap(new_nrot);

    vector<bool> new_horiz_exits((new_lwidth - 1) * new_lheight);
    for (int x = 0; x < new_lwidth - 1; ++x) {
        for (int y = 0; y < new_lheight; ++y) {
            const int new_idx = y * (new_lwidth - 1) + x;
            const int old_idx = (y + yofs) * (lwidth - 1) + (x + xofs);
            new_horiz_exits[new_idx] = horiz_exits[old_idx];
        }
    }
    horiz_exits.swap(new_horiz_exits);

    vector<bool> new_vert_exits(new_lwidth * (new_lheight - 1));
    for (int x = 0; x < new_lwidth; ++x) {
        for (int y = 0; y < new_lheight - 1; ++y) {
            const int new_idx = y * new_lwidth + x;
            const int old_idx = (y + yofs) * lwidth + (x + xofs);
            new_vert_exits[new_idx] = vert_exits[old_idx];
        }
    }
    vert_exits.swap(new_vert_exits);

    lwidth = new_lwidth;
    lheight = new_lheight;
}

void DungeonGenerator::chopLeftSide()
{
    chop(1, 0, lwidth - 1, lheight);
}

void DungeonGenerator::chopRightSide()
{
    chop(0, 0, lwidth - 1, lheight);
}

void DungeonGenerator::chopTopSide()
{
    chop(0, 1, lwidth, lheight - 1);
}

void DungeonGenerator::chopBottomSide()
{
    chop(0, 0, lwidth, lheight - 1);
}

void DungeonGenerator::compress()
{
    // crop left side
    while (1) {
        if (lwidth<=0 || lheight<=0) throw DungeonGenerationFailed();
        bool empty = true;
        for (int y=0; y<lheight; ++y) {
            if (segments[y*lwidth+0] != 0) {
                empty = false;
                break;
            }
        }
        if (empty) {
            chopLeftSide();
            shiftHomes(assigned_homes, -(rwidth+1), 0);
            shiftHomes(unassigned_homes, -(rwidth+1), 0);
        } else {
            break;
        }
    }

    // crop right side
    while (1) {
        if (lwidth <= 0 || lheight <= 0) throw DungeonGenerationFailed();
        bool empty = true;
        for (int y=0; y<lheight; ++y) {
            if (segments[y*lwidth+(lwidth-1)] != 0) {
                empty = false;
                break;
            }
        }
        if (empty) {
            chopRightSide();
        } else {
            break;
        }
    }

    // crop top side
    while (1) {
        if (lwidth <= 0 || lheight <= 0) throw DungeonGenerationFailed();
        bool empty = true;
        for (int x=0; x<lwidth; ++x) {
            if (segments[0*lwidth+x] != 0) {
                empty = false;
                break;
            }
        }
        if (empty) {
            chopTopSide();
            shiftHomes(assigned_homes, 0, -(rheight+1));
            shiftHomes(unassigned_homes, 0, -(rheight+1));
        } else {
            break;
        }
    }

    // crop bottom side
    while (1) {
        if (lwidth <= 0 || lheight <= 0) throw DungeonGenerationFailed();
        bool empty = true;
        for (int x=0; x<lwidth; ++x) {
            if (segments[(lheight-1)*lwidth + x] != 0) {
                empty = false;
                break;
            }
        }
        if (empty) {
            chopBottomSide();
        } else {
            break;
        }
    }
}

void DungeonGenerator::copySegmentsToMap(DungeonMap & dmap, MonsterManager &monster_manager, CoordTransform &ct)
{
    if (!dmap.getRoomMap()) {
        dmap.setRoomMap(new RoomMap);
    }

    // copy segments to map (this also adds rooms)
    for (int x=0; x<lwidth; ++x) {
        for (int y=0; y<lheight; ++y) {
            MapCoord corner( x*(rwidth+1)+1, y*(rheight+1)+1 );
            if (segments[y*lwidth+x]) {                
                segments[y*lwidth+x]->copyToMap(dmap, monster_manager, corner, segment_x_reflect[y*lwidth+x], segment_nrot[y*lwidth+x]);
                ct.add(corner, rwidth, rheight, segment_x_reflect[y*lwidth+x], segment_nrot[y*lwidth+x]);
            } else {
                fillWithWalls(dmap, corner, rwidth, rheight);
            }
        }
    }

    // tell RoomMap that all rooms have been added
    dmap.getRoomMap()->doneAddingRooms();
    
    // fill in walls (around the edge of each segment)
    if (wall) {

        for (int xs=0; xs<lwidth; ++xs) {
            for (int ys=0; ys<lheight; ++ys) {
                // horizontal walls
                for (int x=0; x<rwidth+2; ++x) {
                    MapCoord mc(xs*(rwidth+1)+x, ys*(rheight+1));
                    dmap.clearTiles(mc);
                    dmap.addTile(mc, wall->clone(false), Originator(OT_None()));
                    mc.setY((ys+1)*(rheight+1));
                    dmap.clearTiles(mc);
                    dmap.addTile(mc, wall->clone(false), Originator(OT_None()));
                }
                
                // vertical walls
                for (int y=0; y<rheight+2; ++y) {
                    MapCoord mc(xs*(rwidth+1), ys*(rheight+1)+y);
                    dmap.clearTiles(mc);
                    dmap.addTile(mc, wall->clone(false), Originator(OT_None()));
                    mc.setX((xs+1)*(rwidth+1));
                    dmap.clearTiles(mc);
                    dmap.addTile(mc, wall->clone(false), Originator(OT_None()));
                }
            }
        }
    }
}

void DungeonGenerator::fillWithWalls(DungeonMap & dmap, const MapCoord &corner,
                                     int width, int height)
{
    if (wall) {
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                const MapCoord mc(corner.getX() + x, corner.getY() + y);
                dmap.clearTiles(mc);
                dmap.addTile(mc, wall->clone(false), Originator(OT_None()));
            }
        }
    }
}

bool DungeonGenerator::placeDoor(DungeonMap & dmap, const MapCoord &mc,
                                 const MapCoord &side1, const MapCoord &side2,
                                 const MapCoord &front, const MapCoord &back,
                                 shared_ptr<Tile> door_tile_1, shared_ptr<Tile> door_tile_2)
{
    if (!door_tile_1) return false;
    vector<shared_ptr<Tile> > tiles;

    // front and back must have A_CLEAR at H_WALKING
    if (dmap.getAccess(front, H_WALKING) != A_CLEAR) return false;
    if (dmap.getAccess(back, H_WALKING) != A_CLEAR) return false;

    // front and back must have at least one tile assigned
    // also: front and back must not be stair tiles.
    // also: front and back must allow items (this stops doors being generated in front of
    // pits).
    dmap.getTiles(front, tiles);
    if (tiles.empty()) return false;
    for (int i=0; i<tiles.size(); ++i) {
        if (tiles[i]->isStairOrTop() || !tiles[i]->itemsAllowed()) {
            return false;
        }
    }
    dmap.getTiles(back, tiles);
    if (tiles.empty()) return false;
    for (int i=0; i<tiles.size(); ++i) {
        if (tiles[i]->isStairOrTop() || !tiles[i]->itemsAllowed()) {
            return false;
        }
    }

    // sides must not already have doors on them -- check this using access
    // (should be A_BLOCKED).
    if (dmap.getAccess(side1, H_WALKING) != A_BLOCKED) return false;
    if (dmap.getAccess(side2, H_WALKING) != A_BLOCKED) return false;

    // The proposed door tile should not be a corner of a room.
    if (dmap.getRoomMap()->isCorner(mc)) return false;
    
    // OK.
    // Remove all existing tiles
    dmap.getTiles(mc, tiles);
    for (int i=0; i<tiles.size(); ++i) dmap.rmTile(mc, tiles[i], Originator(OT_None()));

    // Add the new door tile.
    dmap.addTile(mc, door_tile_1->clone(false), Originator(OT_None()));
    if (door_tile_2) {
        dmap.addTile(mc, door_tile_2->clone(false), Originator(OT_None()));
    }

    return true;
}

void DungeonGenerator::knockThroughDoors(DungeonMap & dmap)
{
    const int max_attempts = 30;

    // NB there are two copies of the door code in this routine, one
    // for horiz doors and one for vert doors ....
    
    // Horizontal doors (vertical exits)
    // Doorway between (x,y) and (x,y+1).
    for (int x=0; x<lwidth; ++x) {
        for (int y=0; y<lheight-1; ++y) {
            if (segments[y*lwidth+x] && segments[(y+1)*lwidth+x] && vert_exits[y*lwidth+x]) {

                // Try to place 3 doors (but only up to max_attempts attempts)

                // NOTE: some of the doors may actually be "duplicates" i.e. the same 
                // square is selected for a door more than once.
                // This is OK, it just means that we will get fewer than three doors
                // connecting the segments in this case.

                int ndoors_placed = 0;
                for (int i=0; i<max_attempts; ++i) {
                    MapCoord mc(g_rng.getInt(0, rwidth) + x*(rwidth+1) + 1,
                                (y+1)*(rheight+1));
                    MapCoord side1 = DisplaceCoord(mc, D_WEST);
                    MapCoord side2 = DisplaceCoord(mc, D_EAST);
                    MapCoord front = DisplaceCoord(mc, D_NORTH);
                    MapCoord back = DisplaceCoord(mc, D_SOUTH);
                    if ( placeDoor(dmap, mc, side1, side2, front, back, horiz_door_1,
                    horiz_door_2) ) {
                        ++ndoors_placed;
                    }
                    if (ndoors_placed == 3) break;  // success -- all three doors were placed.
                }

                // We might not have placed all 3 doors within the
                // time limit. If only 1 or 2 doors were placed, we
                // can accept that, but we can't accept 0 doors.
                if (ndoors_placed == 0) throw DungeonGenerationFailed();
            }
        }
    }

    // Vertical doors (horizontal exits)
    // Doorway between (x,y) and (x+1,y)
    // This is similar to the above (but with x and y swapped over basically).
    for (int x=0; x<lwidth-1; ++x) {
        for (int y=0; y<lheight; ++y) {
            if (segments[y*lwidth+x] && segments[y*lwidth+(x+1)]
            && horiz_exits[y*(lwidth-1)+x]) {
                int ndoors_placed = 0;
                for (int i=0; i<max_attempts; ++i) {
                    MapCoord mc((x+1)*(rwidth+1),
                                g_rng.getInt(0, rheight) + y*(rheight+1) + 1);
                    MapCoord side1 = DisplaceCoord(mc, D_NORTH);
                    MapCoord side2 = DisplaceCoord(mc, D_SOUTH);
                    MapCoord front = DisplaceCoord(mc, D_WEST);
                    MapCoord back = DisplaceCoord(mc, D_EAST);
                    if ( placeDoor(dmap, mc, side1, side2, front, back, vert_door_1,
                    vert_door_2) ) {
                        ++ndoors_placed;
                    }
                    if (ndoors_placed == 3) break;
                }
                if (ndoors_placed == 0) throw DungeonGenerationFailed();
            }
        }
    }
}


void DungeonGenerator::generateExits()
{
    // This generates the "standard" player exit points (not the special exit points).
    switch (exit_type) {
    case E_SPECIAL:
        {
            // This is a little unsophisticated, but fine for what we want to use it for:
            // Look for the first segment of the given category.
            // Then look for the first home within that segment.
            // Then add it to unassigned_homes, as well as setting it as the exit point.
            // UPDATE for #20: We now insist that the home has the special_exit flag set on it.
            bool found = false;
            for (int x=0; x<lwidth; ++x) {
                for (int y=0; y<lheight; ++y) {
                    if (segment_categories[y*lwidth+x] == exit_category) {
                        const Segment *seg = segments[y*lwidth+x];
                        ASSERT(seg); // if seg_category is set, then so must seg be
                        if (seg->getNumHomes() > 0) {

                            // find the first home with special exit flag set.
                            HomeInfo hi;
                            found = FindSpecialExit(*seg, segment_x_reflect[y*lwidth+x], segment_nrot[y*lwidth+x], hi);

                            if (found) {
                                                            
                                // assigned_homes.size() is just giving us the number of players here.
                                for (int i=0; i<assigned_homes.size(); ++i) {
                                
                                    exits.push_back(make_pair(MapCoord(hi.x + x*(rwidth+1)+1,
                                                                       hi.y + y*(rheight+1)+1),
                                                              hi.facing));
                                }

                                // add it to unassigned_homes, too (this is so wand of
                                // securing can work on it):
                                unassigned_homes.push_back(exits.back());
                            
                                break;
                            }
                        }
                    }
                }
                if (found) break;
            }
        }
        break;
    case E_RANDOM:
        {
            // choose from the unassigned homes only (Trac #75)
            const int k = g_rng.getInt(0, unassigned_homes.size());
            for (int i=0; i<assigned_homes.size(); ++i) {
                exits.push_back(unassigned_homes[k]);
            }
        }
        break;
    default:
        {
            for (int i=0; i<assigned_homes.size(); ++i) {
                exits.push_back(make_pair(MapCoord(), D_NORTH));
            }
        }
        break;
    }
}


void DungeonGenerator::getHome(int i, MapCoord &pos, MapDirection &facing_toward_home) const
{
    if (home_type == H_RANDOM_RESPAWN) {
        pos = MapCoord();
        facing_toward_home = D_NORTH;
    } else {
        // "assigned_homes" is what we want here
        if (i < 0 || i >= assigned_homes.size()) throw UnexpectedError("Home number is out of range");
        pos = assigned_homes[i].first;
        facing_toward_home = assigned_homes[i].second;
        pos = DisplaceCoord(pos, Opposite(facing_toward_home)); // we want the square 1 in front of the home.
    }
}

void DungeonGenerator::getHomeOverall(int i, MapCoord &pos, MapDirection &facing) const
{
    // This needs to use either unassigned homes or assigned homes, as appropriate
    if (i < 0 || i >= getNumHomesOverall()) return;
    if (i < assigned_homes.size()) {
        pos = assigned_homes[i].first;
        facing = assigned_homes[i].second;
    } else {
        pos = unassigned_homes[i - assigned_homes.size()].first;
        facing = unassigned_homes[i - assigned_homes.size()].second;
    }
    pos = DisplaceCoord(pos, Opposite(facing));
}
     

void DungeonGenerator::getExit(int i, MapCoord &pos, MapDirection &facing) const
{
    if (i < 0 || i >= exits.size()) return;
    pos = exits[i].first;
    facing = exits[i].second;
    pos = DisplaceCoord(pos, Opposite(facing));
}

void DungeonGenerator::generateRequiredItems(DungeonMap &dmap)
{
    const int maxtries = 5;
    const int w = dmap.getWidth(), h = dmap.getHeight();

    vector<shared_ptr<Tile> > tiles;
    
    for (vector<const ItemType *>::iterator it = required_items.begin();
    it != required_items.end(); ++it) {
        bool found = false;
        for (int tries = 0; tries < maxtries; ++tries) {
            // Select a tile category
            if (total_stuff_weight == 0) throw DungeonGenerationFailed(); // can't have required items w/o stuff categories!!
            int t = g_rng.getInt(0, total_stuff_weight);
            int chosen_cat = -999;
            for (map<int,StuffInfo>::iterator si = stuff.begin(); si != stuff.end(); ++si) {
                t -= si->second.weight;
                if (t < 0) {
                    chosen_cat = si->first;
                    break;
                }
            }
            ASSERT(t<0);

            // Now randomly pick squares until we find one of the required category
            // (Or, if this is the last try, we accept any tile category that doesn't have 'forbid' set.)
            MapCoord mc;
            for (int q=0; q<w*h; ++q) {
                mc.setX(g_rng.getInt(0,w));
                mc.setY(g_rng.getInt(0,h));

                // work out the tile category
                dmap.getTiles(mc, tiles);
                const int cat = findItemCategory(dmap, mc, tiles);

                if (cat == chosen_cat || (tries==maxtries-1 && cat >= 0 && !forbidden(cat))) {
                    // OK, put the item here
                    placeItem(dmap, mc, tiles, **it, 1);
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        if (!found) {
            // Failed to place the item. This is fatal.
            throw DungeonGenerationFailed();
        }
    }   
}

bool DungeonGenerator::forbidden(int cat) const
{
    const map<int,StuffInfo>::const_iterator it = stuff.find(cat);
    if (it == stuff.end()) return false;
    return it->second.forbid;
}

int DungeonGenerator::findItemCategory(const DungeonMap &dmap, const MapCoord &mc,
                                        const vector<shared_ptr<Tile> > &tiles)
{
    int chosen_cat = -1;

    for (vector<shared_ptr<Tile> >::const_iterator it = tiles.begin();
    it != tiles.end(); ++it) {
        int cat = (*it)->getItemCategory();
        if (cat >=0) {
            chosen_cat = cat;
        }
        if ((*it)->canPlaceItem()) {
            // Don't generate an item if one has already been 'placed'
            if ((*it)->itemPlacedAlready()) return -1;
        } else {
            // For tiles that aren't explicitly accepting items, we go by the itemsAllowed()
            // flag, and generate items only where itemsAllowed()==true.
            // (Note that this doesn't work the same for 'placed' items, e.g. barrels have
            // itemsAllowed()==false but can still accept a placed item.)
            if ((*it)->itemsAllowed()==false) {
                return -1;
            }
        }
    }

    // Don't generate an item if one is already present
    if (dmap.getItem(mc)) return -1;

    return chosen_cat;
}

void DungeonGenerator::placeItem(DungeonMap &dmap, const MapCoord &mc,
                                 vector<shared_ptr<Tile> > &tiles, const ItemType &itype,
                                 int no)
{
    shared_ptr<Item> item(new Item(itype, no));
    bool placed = false;
    for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end(); ++it) {
        if ((*it)->canPlaceItem() && !(*it)->itemPlacedAlready()) {
            (*it)->placeItem(item);
            placed = true;
            break;
        }
    }
    if (!placed) dmap.addItem(mc, item);
}

void DungeonGenerator::generateStuff(DungeonMap &dmap) 
{
    vector<shared_ptr<Tile> > tiles;
    for (int i=0; i<dmap.getWidth(); ++i) {
        for (int j=0; j<dmap.getHeight(); ++j) {
            MapCoord mc(i,j);

            // Find item-category associated with this tile. Non-negative value means we
            // should try to generate an item.
            dmap.getTiles(mc, tiles);
            const int chosen_cat = findItemCategory(dmap, mc, tiles);
            
            // Look for the chosen category in "stuff" (our container of ItemGenerators).
            if (chosen_cat >= 0) {
                map<int,StuffInfo>::const_iterator it = stuff.find(chosen_cat);
                if (it != stuff.end() && g_rng.getBool(it->second.chance)) {
                    const ItemGenerator *generator = it->second.generator;
                    if (generator) {
                        // We are to generate an item
                        pair<const ItemType *, int> result = generator->get();
                        ASSERT(result.first);
                        placeItem(dmap, mc, tiles, *result.first, result.second);
                    }
                }
            }
        }
    }
}

void DungeonGenerator::generateLocksAndTraps(DungeonMap &dmap, int nkeys)
{
    vector<shared_ptr<Tile> > tiles;
    for (int i=0; i<dmap.getWidth(); ++i) {
        for (int j=0; j<dmap.getHeight(); ++j) {
            MapCoord mc(i,j);
            dmap.getTiles(mc, tiles);
            for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end();
            ++it) {
                shared_ptr<Lockable> lockable = dynamic_pointer_cast<Lockable>(*it);
                if (lockable) {
                    // try to generate a trap. (only if pretrapped is on)
                    bool trap_placed = pretrapped ? lockable->generateTrap(dmap, mc) : false;
                    if (!trap_placed) {
                        // if that fails, then try to generate a lock instead.
                        lockable->generateLock(nkeys);
                    }
                }
            }
        }
    }
}

void DungeonGenerator::placeInitialMonsters(DungeonMap &dmap, MonsterManager &mmgr,
                                            const MonsterType &mtype, int num_monsters)
{
    const MapHeight monster_height = mtype.getHeight();
    
    vector<shared_ptr<Tile> > tiles;
    for (int i = 0; i < num_monsters; ++i) {
        for (int tries = 0; tries < 10; ++tries) {
            const int x = g_rng.getInt(0, dmap.getWidth());
            const int y = g_rng.getInt(0, dmap.getHeight());
            const MapCoord mc(x, y);

            // To place a monster, need a non-stair tile with clear access at the relevant height.
            if (dmap.getAccess(mc, monster_height) != A_CLEAR) continue;
            dmap.getTiles(mc, tiles);
            bool ok = true;
            for (int i=0; i<tiles.size(); ++i) {
                if (tiles[i]->isStairOrTop()) {
                    ok = false;
                    break;
                }
            }
            if (!ok) continue;

            // Place the monster
            // (Note: facing is just set to D_NORTH initially; the monster AI will soon turn around if it wants to)
            mmgr.placeMonster(mtype, dmap, mc, D_NORTH);
            break;
        }
    }
}

void DungeonGenerator::checkConnectivity(DungeonMap &dmap, const MapCoord &from_where, int num_keys)
{
    // This checks to see whether it is possible to obtain all keys
    // (or the lock picks) starting from "from_where".

    // If not, it tries to correct the situation by dropping a random
    // lockpick somewhere in the dungeon. If that fails for any reason
    // then we give up and throw DungeonGenerationFailed.


    // "open" = squares yet to be checked.
    // "visited" = have reached this square from the starting point, and the knight can access it.
    // "blocked" = knight can't access this square.
    // "locked" = knight can get to this locked door but doesn't have the key to open it.
    
    std::set<MapCoord> open, visited, blocked, locked;
    std::set<int> keys_found;
    open.insert(from_where);

    while (!open.empty()) {

        // Get the first square from the open list
        MapCoord mc = *open.begin();
        open.erase(open.begin());

        // Ignore squares that are outside the map
        if (!dmap.valid(mc)) continue;

        // Ignore squares that have already been visited
        if (visited.find(mc) != visited.end() || blocked.find(mc) != blocked.end() || locked.find(mc) != locked.end()) continue;

        // Let's get a list of tiles on this square
        vector<shared_ptr<Tile> > tiles;
        dmap.getTiles(mc, tiles);
    
        // Find out whether there is a key (or lock picks) on this square.
        // (This is complicated slightly because there could be an item hidden inside a chest.)
        shared_ptr<Item> item = dmap.getItem(mc);
        if (!item) {
            for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end(); ++it) {
                item = (*it)->getPlacedItem();
                if (item) {
                    break;
                }
            }
        }
        
        const ItemType * item_type = item ? &item->getType() : 0;
    
        // As soon as we find lock picks, we are done (as they can then open any door).
        if (item_type && item_type->getKey() == -1) return;

        // Check whether we have found one of the keys.
        const int key_num = item_type ? item_type->getKey() : 0;
        if (key_num > 0) {
            // Add this key to our list of keys found.
            keys_found.insert(key_num);
            
            // Check whether we have found all keys. If so we are done.
            if (keys_found.size() >= num_keys) return;

            // This key might open one of the existing "locked" tiles
            // so we force them to be re-checked, by adding them back
            // into "open".
            open.insert(locked.begin(), locked.end());
            locked.clear();
        }

        // We now want to detect whether the square is passable. If so
        // we can add its neighbours to "open". If it's impassable
        // currently, but might become passable if we find the right
        // key, then add it to "locked".

        bool can_get_through = true;
        bool is_locked = false;

        for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end(); ++it) {
            // Check whether the tile has explicit connectivity info set
            if ((*it)->getConnectivityCheck() == -1) {
                can_get_through = false;
                break;
            } else if ((*it)->getConnectivityCheck() == 1) {
                continue;
            }

            // Check whether access is clear at H_WALKING
            const MapAccess access = (*it)->getAccess(H_WALKING);
            if (access == A_CLEAR) continue;
            
            // This tile might block access, unless:
            // (i) It is destructible
            // (ii) It is locked, but can be opened by one of our keys.
            if ((*it)->destructible()) continue;

            const Lockable * lockable = dynamic_cast<Lockable*>(it->get());
            if (lockable) {
                const int lock_num = lockable->getLockNum();
                if (lock_num > 0 && keys_found.find(lock_num) == keys_found.end()) {
                    can_get_through = false;
                    is_locked = true;
                    break;
                } else {
                    // Either lock_num == 0 (i.e. its an unlocked or "special-locked" iron door)
                    // or lock_num != 0 but we have the key for it. In this case the door is
                    // passable.
                    // NOTE: We assume that the levers/pressure pads that open "special-locked" doors are
                    // always accessible. 
                    can_get_through = true;
                    break;
                }
            }

            // If we get to here then it must be impassable.
            can_get_through = false;
            break;
        }
        
        if (can_get_through) {
            // We can reach this tile
            // Add its neighbours to "open".
            open.insert(DisplaceCoord(mc, D_NORTH));
            open.insert(DisplaceCoord(mc, D_EAST));
            open.insert(DisplaceCoord(mc, D_SOUTH));
            open.insert(DisplaceCoord(mc, D_WEST));
        }

        // Mark it as visited so that we don't visit it again.
        if (is_locked) {
            locked.insert(mc);
        } else if (can_get_through) {
            visited.insert(mc);
        } else {
            blocked.insert(mc);
        }
    }

    // We explored the whole map without finding all keys, or lockpicks.

    // Try to drop a random lockpick somewhere. Pick a square from "visited",
    // so that we know the square will be accessible to our knight.

    bool lockpicks_placed = false;
    if (lockpicks && !visited.empty()) {
        std::vector<MapCoord> squares(visited.begin(), visited.end());

        // Up to 10 attempts.
        for (int i = 0; i < 10 && !lockpicks_placed; ++i) {
            const int r = g_rng.getInt(0, squares.size());
            const MapCoord mc = squares[r];

            // Work out whether we can place items here.
            vector<shared_ptr<Tile> > tiles;
            dmap.getTiles(mc, tiles);
            const int cat = findItemCategory(dmap, mc, tiles);
            
            if (cat >= 0 && !forbidden(cat)) {

                // We avoid putting lockpicks in any tile with "canPlaceItem" set.
                // (We don't want the lockpicks appearing in barrels as players might never
                // find them. "canPlaceItem" is the simplest way to prevent this.)
                bool ok = true;
                for (vector<shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
                    if ((*it)->canPlaceItem()) {
                        ok = false;
                        break;
                    }
                }

                if (ok) {
                    // Place the lockpicks.
                    placeItem(dmap, mc, tiles, *lockpicks, 1);
                    lockpicks_placed = true;
                }
            }
        }
    }
    
    if (!lockpicks_placed) {
        // Give up.
        throw DungeonGenerationFailed();
    }
}


void DungeonGenerator::checkTutorial(DungeonMap &dmap)
{
    MapCoord home_pos;  // the floor space in front of the home.
    MapDirection facing_toward_home;
    getHome(0, home_pos, facing_toward_home);

    // Check the home and the eight surrounding squares (except the entry point square itself).
    for (int x = home_pos.getX() - 1; x <= home_pos.getX() + 1; ++x) {
        for (int y = home_pos.getY() - 1; y <= home_pos.getY() + 1; ++y) {
            MapCoord mc(x,y);
            if (dmap.valid(mc) && mc != DisplaceCoord(home_pos, facing_toward_home)) {

                // Check there are no tutorial tiles on this square
                std::vector<boost::shared_ptr<Tile> > tiles;
                dmap.getTiles(mc, tiles);
                for (std::vector<boost::shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
                    if ((*it)->getTutorialKey() > 0) throw DungeonGenerationFailed();
                }

                // Check there are no items on this square
                if (dmap.getItem(mc)) throw DungeonGenerationFailed();
            }
        }
    }

    // Check that any wooden doors in the starting room are unlocked.
    const RoomMap *rmap = dmap.getRoomMap();
    if (!rmap) throw DungeonGenerationFailed();
    int room_arr[2];
    rmap->getRoomAtPos(home_pos, room_arr[0], room_arr[1]);
    for (int r = 0; r < 2; ++r) {
        int room = room_arr[r];
        if (room != -1) {
            MapCoord top_left;
            int w, h;
            rmap->getRoomLocation(room, top_left, w, h);
            for (int x = 0; x < w; ++x) {
                for (int y = 0; y < h; ++y) {
                    MapCoord mc(top_left.getX() + x, top_left.getY() + y);
                    std::vector<boost::shared_ptr<Tile> > tiles;
                    dmap.getTiles(mc, tiles);
                    for (std::vector<boost::shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
                        Lockable * lockable = dynamic_cast<Lockable*>(it->get());
                        if (lockable && lockable->destructible()) { // wooden door
                            // -- it could also be a chest, but (a) this is unlikely because most starting rooms
                            // don't contain chests, and (b) we don't really care, as this is only the tutorial mode
                            // anyway.
                            if (lockable->isLocked()) {
                                throw DungeonGenerationFailed();
                            }
                        }
                    }
                }
            }
        }
    }
}

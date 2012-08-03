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
#include "dungeon_generation_failed.hpp"
#include "dungeon_generator.hpp"
#include "dungeon_layout.hpp"
#include "dungeon_map.hpp"
#include "home_manager.hpp"
#include "item.hpp"
#include "lockable.hpp"
#include "monster_manager.hpp"
#include "monster_type.hpp"
#include "my_exceptions.hpp"
#include "player.hpp"
#include "rng.hpp"
#include "room_map.hpp"
#include "segment.hpp"
#include "segment_set.hpp"
#include "tile.hpp"

namespace {

    // ----------------------------------------------------------------------------------

    // Layout of dungeon blocks.
    
    struct BlockInfo {
        int x, y;
        bool special;
    };

    struct IsSpecial {
        bool operator()(const BlockInfo &b) { return b.special; }
    };

    void FetchBlockFrom(std::vector<BlockInfo> &edges, int &x, int &y)
    {
        // This fetches an edge (from back of the list) or fails if there are no edges left.
        if (edges.empty()) throw DungeonGenerationFailed();
        x = edges.back().x;
        y = edges.back().y;
        edges.pop_back();
    }
    
    void FetchEdgeOrBlock(std::vector<BlockInfo> &edges,
                          std::vector<BlockInfo> &blocks,
                          int &x, int &y)
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


    // Function to lay out the edges and blocks.
    
    void DoLayout(int nplayers,                  // in
                  int homes_required,            // in
                  const DungeonLayout &layout,   // in
                  HomeType home_type,            // in
                  int &lwidth,                   // out
                  int &lheight,                  // out
                  std::vector<BlockInfo> &edges,   // out
                  std::vector<BlockInfo> &blocks,  // out
                  std::vector<bool> &horiz_exits,  // out
                  std::vector<bool> &vert_exits)   // out
    {
        RNG_Wrapper myrng(g_rng);
        int x, y;

        // get width and height
        lwidth = layout.getWidth();
        lheight = layout.getHeight();

        // work out where the edges and blocks are.
        // flip and/or rotate the layout if necessary.

        edges.clear();
        blocks.clear();
        
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
                switch (layout.getBlockType(i,j)) {
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

        // work out exits information (needs to be flipped/rotated)

        vert_exits.clear();
        horiz_exits.clear();
        
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
                    vert_exits[x*new_lwidth + (lheight-1-y)] = layout.hasHorizExit(i,j);
                } else {
                    horiz_exits[y*(new_lwidth-1) + x] = layout.hasHorizExit(i,j);
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
                    horiz_exits[x*(new_lwidth-1) + (lheight-2-y)] = layout.hasVertExit(i,j);
                } else {
                    vert_exits[y*new_lwidth + x] = layout.hasVertExit(i,j);
                }
            }
        }

        // randomize blocks and edges
        std::random_shuffle(blocks.begin(), blocks.end(), myrng);
        std::random_shuffle(edges.begin(), edges.end(), myrng);

        // update width & height
        lwidth = new_lwidth;
        lheight = new_lheight;
    }
    
    
    // ----------------------------------------------------------------------------------

    // Placement of segments.

    struct SegmentInfo {
        SegmentInfo() : segment(0), x_reflect(false), nrot(0) { }
        
        const Segment * segment;
        bool x_reflect;
        int nrot;
    };

    int CountNonSpecialHomes(const std::vector<HomeInfo> &homes)
    {
        int ct = 0;
        for (std::vector<HomeInfo>::const_iterator it = homes.begin(); it != homes.end(); ++it) {
            if (!it->special_exit) ++ct;
        }
        return ct;
    }
    
    // Copies all homes from the given segment (whether special or
    // not) into 'all_homes'. Returns the number added.
    int CopyHomes(const SegmentInfo &info,
                  int rwidth, int rheight,
                  int x,
                  int y,
                  std::vector<HomeInfo> &all_homes)
    {
        const int xbase = x*(rwidth+1)+1;    // assuming that all segments are same size
        const int ybase = y*(rheight+1)+1;
        
        const std::vector<HomeInfo> h(info.segment->getHomes(info.x_reflect, info.nrot));
        int ct = 0;
        
        for (std::vector<HomeInfo>::const_iterator it = h.begin(); it != h.end(); ++it) {
            all_homes.push_back(*it);
            ++ct;
            // offset x,y position based on where this segment is within the dungeon.
            all_homes.back().x += xbase;
            all_homes.back().y += ybase;
        }

        return ct;
    }

    
    // Place a segment into segment_infos, choosing a random
    // reflection/rotation. Also appends all homes in the segment
    // (whether special or not) to 'all_homes'. Returns number of
    // homes added (whether special or not).
    int AddSegment(const Segment *segment,
                   int x,
                   int y,
                   int lwidth,
                   int rwidth, int rheight,
                   std::vector<SegmentInfo> &segment_infos,
                   std::vector<HomeInfo> &all_homes)
    {
        SegmentInfo &inf = segment_infos[y*lwidth + x];
        inf.segment = segment;
        inf.x_reflect = g_rng.getBool();
        inf.nrot = g_rng.getInt(0, 4);
        return CopyHomes(inf, rwidth, rheight, x, y, all_homes);
    }

    
    // Copies N non-special homes, chosen randomly from the last M elements of "all_homes", into "assigned_homes". 
    // Precondition: there must be at least that many non-special homes available.
    // Note: this will reorder all_homes.
    void AssignHomes(std::vector<HomeInfo> &all_homes,
                     int how_many_to_choose_from,
                     int how_many_to_choose,
                     std::vector<HomeInfo> &assigned_homes)
    {
        ASSERT(how_many_to_choose_from <= int(all_homes.size()));
        
        // Shuffle the last M homes
        RNG_Wrapper myrng(g_rng);
        std::random_shuffle(all_homes.end() - how_many_to_choose_from, all_homes.end(), myrng);

        // Loop through that list.
        for (std::vector<HomeInfo>::const_iterator it = all_homes.end() - how_many_to_choose_from;
        it != all_homes.end() && how_many_to_choose > 0;
        ++it) {
            if (!it->special_exit) {
                assigned_homes.push_back(*it);
                --how_many_to_choose;
            }
        }

        ASSERT(how_many_to_choose == 0); // Caller must ensure there are enough non-special homes available.
    }        
    

    // SetHomeSegment: Adds a normal segment into the layout.
    // 
    // Only segments with 'minhomes' non-special homes (or more) will
    // be considered.
    //
    // 'assign' is the number of non-special homes to copy into
    // 'assigned_homes'.
    //
    void SetHomeSegment(const SegmentSet &segment_set,
                        int x,
                        int y,
                        int lwidth,
                        int rwidth, int rheight,
                        std::vector<SegmentInfo> &segment_infos,
                        std::vector<HomeInfo> &assigned_homes,
                        std::vector<HomeInfo> &all_homes,
                        int minhomes,
                        int assign)
    {
        ASSERT(assign <= minhomes); // can't assign more homes than we have available.
        
        // Choose a random segment
        // (Do not use a segment that is already in the "segments" vector, unless we have no other choice.)
        const int max_attempts = 50;
        const Segment *r = 0;
        for (int i = 0; i < max_attempts; ++i) {
            r = segment_set.getSegment(minhomes);
               // ... returns a segment with at least 'minhomes' non-special homes
               // (returns 0 if impossible)
            
            if (!r) break;

            // See if it already exists in segment_infos
            bool found = false;
            for (std::vector<SegmentInfo>::const_iterator it = segment_infos.begin();
            it != segment_infos.end(); ++it) {
                if (it->segment == r) {
                    found = true;
                    break;
                }
            }
            if (!found) break;
        }
        if (!r) throw DungeonGenerationFailed();

        // Add it to the dungeon
        const int nhomes_avail = AddSegment(r, x, y, lwidth, rwidth, rheight, segment_infos, all_homes);

        // Assign homes if required
        // NOTE: Precondition of AssignHomes is satisfied, because SegmentSet::getSegment
        // guaranteed that there are at least minhomes non-special homes, and minhomes >= assign.
        if (assign > 0) {
            AssignHomes(all_homes, nhomes_avail, assign, assigned_homes);
        }
    }
    

    // PlaceSegments: chooses which segment to place into each dungeon
    // block, and records the home locations. Also, if H_CLOSE or
    // H_AWAY requested, assigns homes to players.
    
    void PlaceSegments(int nplayers,
                       int homes_required,
                       HomeType home_type,
                       int lwidth, int lheight,
                       int rwidth, int rheight,
                       const SegmentSet &segment_set,
                       const std::vector<const Segment *> & required_segments,
                       std::vector<BlockInfo> &edges,           // in/out
                       std::vector<BlockInfo> &blocks,          // in/out
                       std::vector<SegmentInfo> &segment_infos, // out
                       std::vector<HomeInfo> &assigned_homes,   // out
                       std::vector<HomeInfo> &all_homes)        // out
    {
        RNG_Wrapper myrng(g_rng);
                    
        segment_infos.clear();
        segment_infos.resize(lwidth*lheight);

        // "Away from Other" homes -- generate nplayers different segments
        // (MUST be on edges) and assign one home from each segment as the player homes.
        if (home_type == H_AWAY) {
            for (int i = 0; i < nplayers; ++i) {
                int x, y;
                FetchBlockFrom(edges, x, y);
                ASSERT(x >= 0 && x < lwidth && y >= 0 && y < lheight);
                SetHomeSegment(segment_set, x, y, lwidth, rwidth, rheight, 
                               segment_infos, assigned_homes, all_homes, 1, 1);
            }
        }

        // Place Required segments -- on Edges if possible, Blocks otherwise.
        for (std::vector<const Segment*>::const_iterator it = required_segments.begin();
        it != required_segments.end(); ++it) {
            int x, y;
            FetchEdgeOrBlock(edges, blocks, x, y);
            ASSERT(x >= 0 && x < lwidth && y >= 0 && y < lheight);
            AddSegment(*it, x, y, lwidth, rwidth, rheight, segment_infos, all_homes);
        }
        
        // Eliminate "special" blocks if they have not been assigned.
        edges.erase(std::remove_if(edges.begin(), edges.end(), IsSpecial()), edges.end());

        // Place "Close To Other" homes -- generate a single segment,
        // preferably on an edge, and assign all players a home in
        // that same segment.
        if (home_type == H_CLOSE) {
            int x, y;
            FetchEdgeOrBlock(edges, blocks, x, y);
            ASSERT(x >= 0 && x < lwidth && y >= 0 && y < lheight);
            SetHomeSegment(segment_set, x, y, lwidth, rwidth, rheight, segment_infos,
                           assigned_homes, all_homes,
                           nplayers, nplayers);
        }

        // From this point on, there is no distinction between edges and blocks.
        // Shuffle them together.
        edges.erase(std::remove_if(edges.begin(), edges.end(), IsSpecial()), edges.end());
        std::copy(edges.begin(), edges.end(), std::back_inserter(blocks));
        edges.clear();
        std::random_shuffle(blocks.begin(), blocks.end(), myrng);        
        
        // Place all remaining segments.
        while (!blocks.empty()) {
            const int h = homes_required - CountNonSpecialHomes(all_homes);  
                    // minimum number of non-special homes still required
            
            const int nblocks = int(blocks.size());
            
            int n;  // *minimum* number of non-special homes to generate in the new block
            if (h > 2*nblocks) throw DungeonGenerationFailed();
            else if (h == 2*nblocks) n = 2;  // need 2 in every block
            else if (h == 2*nblocks-1) n = 1;  // need at least one here plus two in all others
            else n = 0;  // can get away with 0 here if we want to
            
            int x, y;
            FetchBlockFrom(blocks, x, y);
            ASSERT(x >= 0 && x < lwidth && y >= 0 && y < lheight);
            SetHomeSegment(segment_set, x, y, lwidth, rwidth, rheight, segment_infos, assigned_homes, all_homes, n, 0);
        }

        // Assign H_RANDOM homes if required.
        if (home_type == H_RANDOM) {
            // Explicitly check the precondition of AssignHomes
            if (CountNonSpecialHomes(all_homes) < nplayers) {
                throw DungeonGenerationFailed();
            } else {
                AssignHomes(all_homes, all_homes.size(), nplayers, assigned_homes);
            }
        }
        
        // Shuffle the homes lists.
        std::random_shuffle(assigned_homes.begin(), assigned_homes.end(), myrng);
        std::random_shuffle(all_homes.begin(), all_homes.end(), myrng);

        // Check number of assigned homes equals number of players (unless H_NONE selected).
        ASSERT(home_type != H_NONE && assigned_homes.size() == nplayers
               || home_type == H_NONE && assigned_homes.empty());
    }

    // ------------------------------------------------------------------------------

    // Compressing the dungeon layout.

    void Chop(int &lwidth, int &lheight,
              std::vector<SegmentInfo> &segment_infos,
              std::vector<bool> &horiz_exits,
              std::vector<bool> &vert_exits,
              int xofs, int yofs, int new_width, int new_height)
    {
        std::vector<SegmentInfo> new_segments;
        new_segments.reserve(new_width * new_height);
        
        for (int y = 0; y < new_height; ++y) {
            for (int x = 0; x < new_width; ++x) {
                const int old_idx = (y + yofs) * lwidth + (x + xofs);
                new_segments.push_back(segment_infos[old_idx]);
            }
        }

        std::vector<bool> new_horiz_exits;
        new_horiz_exits.reserve((new_width-1) * new_height);
        for (int y = 0; y < new_height; ++y) {
            for (int x = 0; x < new_width - 1; ++x) {
                const int old_idx = (y + yofs) * (lwidth - 1) + (x + xofs);
                new_horiz_exits.push_back(horiz_exits[old_idx]);
            }
        }

        std::vector<bool> new_vert_exits;
        new_vert_exits.reserve(new_width * (new_height - 1));
        for (int y = 0; y < new_height - 1; ++y) {
            for (int x = 0; x < new_width; ++x) {
                const int old_idx = (y + yofs) * lwidth + (x + xofs);
                new_vert_exits.push_back(vert_exits[old_idx]);
            }
        }

        lwidth = new_width;
        lheight = new_height;
        segment_infos.swap(new_segments);
        horiz_exits.swap(new_horiz_exits);
        vert_exits.swap(new_vert_exits);
    }

    void ShiftHomes(std::vector<HomeInfo> &homes, int dx, int dy)
    {
        for (std::vector<HomeInfo>::iterator it = homes.begin(); it != homes.end(); ++it) {
            it->x += dx;
            it->y += dy;
        }
    }
    
    void Compress(int &lwidth,
                  int &lheight,
                  int rwidth,
                  int rheight,
                  std::vector<SegmentInfo> &segment_infos,
                  std::vector<bool> &horiz_exits,
                  std::vector<bool> &vert_exits,
                  std::vector<HomeInfo> &assigned_homes,
                  std::vector<HomeInfo> &all_homes)
    {
        // crop left side
        while (1) {
            if (lwidth <= 0 || lheight <= 0) throw DungeonGenerationFailed();

            bool empty = true;
            for (int y = 0; y < lheight; ++y) {
                if (segment_infos[y*lwidth + 0].segment != 0) {
                    empty = false;
                    break;
                }
            }
            if (empty) {
                Chop(lwidth, lheight, segment_infos, horiz_exits, vert_exits,
                     1, 0, lwidth-1, lheight);
                ShiftHomes(assigned_homes, -(rwidth+1), 0);
                ShiftHomes(all_homes, -(rwidth+1), 0);
            } else {
                break;
            }
        }

        // crop right side
        while (1) {
            if (lwidth <= 0 || lheight <= 0) throw DungeonGenerationFailed();

            bool empty = true;
            for (int y = 0; y < lheight; ++y) {
                if (segment_infos[y*lwidth + (lwidth-1)].segment != 0) {
                    empty = false;
                    break;
                }
            }
            if (empty) {
                Chop(lwidth, lheight, segment_infos, horiz_exits, vert_exits,
                     0, 0, lwidth-1, lheight);
            } else {
                break;
            }
        }

        // crop top side
        while (1) {
            if (lwidth <= 0 || lheight <= 0) throw DungeonGenerationFailed();

            bool empty = true;
            for (int x = 0; x < lwidth; ++x) {
                if (segment_infos[0*lwidth + x].segment != 0) {
                    empty = false;
                    break;
                }
            }
            if (empty) {
                Chop(lwidth, lheight, segment_infos, horiz_exits, vert_exits,
                     0, 1, lwidth, lheight-1);
                ShiftHomes(assigned_homes, 0, -(rheight+1));
                ShiftHomes(all_homes, 0, -(rheight+1));
            } else {
                break;
            }
        }

        // crop bottom side
        while (1) {
            if (lwidth <= 0 || lheight <= 0) throw DungeonGenerationFailed();

            bool empty = true;
            for (int x = 0; x < lwidth; ++x) {
                if (segment_infos[(lheight-1)*lwidth + x].segment != 0) {
                    empty = false;
                    break;
                }
            }
            if (empty) {
                Chop(lwidth, lheight, segment_infos, horiz_exits, vert_exits,
                     0, 0, lwidth, lheight-1);
            } else {
                break;
            }
        }
    }

    // ------------------------------------------------------------------------------

    // Copying segments to map

    void CopyTilesToSquare(DungeonMap &dmap, int x, int y,
                           const std::vector<boost::shared_ptr<Tile> > &tiles)
    {
        const MapCoord mc(x,y);
        dmap.clearTiles(mc);
        for (std::vector<boost::shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
            dmap.addTile(mc, (*it)->clone(false), Originator(OT_None()));
        }
    }
    
    void FillWithTiles(DungeonMap &dmap, int x, int y, int width, int height,
                       const std::vector<boost::shared_ptr<Tile> > &tiles)
    {
        for (int y2 = 0; y2 < height; ++y2) {
            for (int x2 = 0; x2 < width; ++x2) {
                CopyTilesToSquare(dmap, x + x2, y + y2, tiles);
            }
        }
    }
    
    void CopySegmentsToMap(int lwidth, int lheight,
                           int rwidth, int rheight,
                           const std::vector<boost::shared_ptr<Tile> > &wall_tiles,
                           const std::vector<SegmentInfo> &segment_infos,
                           DungeonMap &dmap, CoordTransform &ct, MonsterManager &monster_manager)
    {
        if (!dmap.getRoomMap()) {
            dmap.setRoomMap(new RoomMap);
        }

        // Copy segments to map (this also adds rooms)
        for (int x = 0; x < lwidth; ++x) {
            for (int y = 0; y < lheight; ++y) {
                MapCoord corner( x*(rwidth+1)+1, y*(rheight+1)+1 );
                const SegmentInfo & info = segment_infos[y*lwidth + x];
                if (info.segment) {
                    info.segment->copyToMap(dmap, monster_manager, corner, info.x_reflect, info.nrot);
                    ct.add(corner, rwidth, rheight, info.x_reflect, info.nrot);
                } else {
                    FillWithTiles(dmap, corner.getX(), corner.getY(), rwidth, rheight, wall_tiles);
                }
            }
        }

        // Tell RoomMap that all rooms have been added
        dmap.getRoomMap()->doneAddingRooms();

        // Fill in walls (around the edge of each segment)
        for (int xs = 0; xs < lwidth; ++xs) {
            for (int ys = 0; ys < lheight; ++ys) {

                // horizontal walls
                for (int x = 0; x < rwidth+2; ++x) {
                    CopyTilesToSquare(dmap, xs*(rwidth+1)+x,  ys   *(rheight+1), wall_tiles);
                    CopyTilesToSquare(dmap, xs*(rwidth+1)+x, (ys+1)*(rheight+1), wall_tiles);
                }
                
                // vertical walls
                for (int y = 0; y < rheight+2; ++y) {
                    CopyTilesToSquare(dmap,  xs   *(rwidth+1), ys*(rheight+1)+y, wall_tiles);
                    CopyTilesToSquare(dmap, (xs+1)*(rwidth+1), ys*(rheight+1)+y, wall_tiles);
                }
            }
        }
    }

    // ------------------------------------------------------------------------------

    // Knocking through doors

    bool PlaceDoor(DungeonMap &dmap, const MapCoord &mc,
                   const MapCoord &side1, const MapCoord &side2,
                   const MapCoord &front, const MapCoord &back,
                   const std::vector<boost::shared_ptr<Tile> > &door_tiles)
    {
        // front and back must have A_CLEAR at H_WALKING
        if (dmap.getAccess(front, H_WALKING) != A_CLEAR) return false;
        if (dmap.getAccess(back, H_WALKING) != A_CLEAR) return false;

        // front and back must have at least one tile assigned
        // also: front and back must not be stair tiles.
        // also: front and back must allow items (this stops doors being generated in front of
        // pits).
        std::vector<boost::shared_ptr<Tile> > tiles;
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

        // OK. Copy door to the map
        CopyTilesToSquare(dmap, mc.getX(), mc.getY(), door_tiles);
        return true;
    }
        
    void KnockThroughDoors(int lwidth, int lheight,
                           int rwidth, int rheight,
                           const std::vector<boost::shared_ptr<Tile> > &hdoor_tiles,
                           const std::vector<boost::shared_ptr<Tile> > &vdoor_tiles,
                           const std::vector<SegmentInfo> & segment_infos,
                           const std::vector<bool> &horiz_exits,
                           const std::vector<bool> &vert_exits,
                           DungeonMap &dmap)
    {
        const int max_attempts = 30;

        // NB there are two copies of the door code in this routine, one
        // for horiz doors and one for vert doors ....
    
        // Horizontal doors (vertical exits)
        // Doorway between (x,y) and (x,y+1).
        for (int x = 0; x < lwidth; ++x) {
            for (int y = 0; y < lheight - 1; ++y) {

                const SegmentInfo &info1 = segment_infos[y*lwidth + x];
                const SegmentInfo &info2 = segment_infos[(y+1)*lwidth + x];

                if (info1.segment && info2.segment && vert_exits[y*lwidth + x]) {
                    
                    // Try to place 3 doors (but only up to max_attempts attempts)

                    // NOTE: some of the doors may actually be "duplicates" i.e. the same 
                    // square is selected for a door more than once.
                    // This is OK, it just means that we will get fewer than three doors
                    // connecting the segments in this case.

                    int ndoors_placed = 0;
                    for (int i = 0; i < max_attempts; ++i) {
                        MapCoord mc(g_rng.getInt(0, rwidth) + x*(rwidth+1) + 1,
                                    (y+1)*(rheight+1));
                        MapCoord side1 = DisplaceCoord(mc, D_WEST);
                        MapCoord side2 = DisplaceCoord(mc, D_EAST);
                        MapCoord front = DisplaceCoord(mc, D_NORTH);
                        MapCoord back = DisplaceCoord(mc, D_SOUTH);
                        if (PlaceDoor(dmap, mc, side1, side2, front, back, hdoor_tiles)) {
                            ++ndoors_placed;
                            if (ndoors_placed == 3) break;  // success -- all three doors were placed.
                        }
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
        for (int x = 0; x < lwidth - 1; ++x) {
            for (int y = 0; y < lheight; ++y) {

                const SegmentInfo &info1 = segment_infos[y*lwidth + x];
                const SegmentInfo &info2 = segment_infos[y*lwidth + (x+1)];

                if (info1.segment && info2.segment && horiz_exits[y*(lwidth-1)+x]) {
                    
                    int ndoors_placed = 0;
                    for (int i = 0; i < max_attempts; ++i) {
                        MapCoord mc((x+1)*(rwidth+1),
                                    g_rng.getInt(0, rheight) + y*(rheight+1) + 1);
                        MapCoord side1 = DisplaceCoord(mc, D_NORTH);
                        MapCoord side2 = DisplaceCoord(mc, D_SOUTH);
                        MapCoord front = DisplaceCoord(mc, D_WEST);
                        MapCoord back = DisplaceCoord(mc, D_EAST);
                        if (PlaceDoor(dmap, mc, side1, side2, front, back, vdoor_tiles)) {
                            ++ndoors_placed;
                            if (ndoors_placed == 3) break;
                        }
                    }
                    if (ndoors_placed == 0) throw DungeonGenerationFailed();
                }
            }
        }
    }

    // ------------------------------------------------------------------------------

    void FindRsize(const std::vector<const Segment *> &segments, int &rwidth, int &rheight)
    {
        // Currently we assume all segments are the same size
        if (!segments.empty()) {
            rwidth = segments.front()->getWidth();
            rheight = segments.front()->getHeight();
        }
    }

    // ------------------------------------------------------------------------------

    // Item generation helpers

    bool Forbidden(int cat,
                   const std::vector<std::pair<int,int> > &weights)
    {
        for (std::vector<std::pair<int,int> >::const_iterator it = weights.begin(); it != weights.end(); ++it) {
            if (it->first == cat) {
                return false;
            }
        }
        return true;
    }

    int FindItemCategory(const DungeonMap &dmap,
                         const MapCoord &mc,
                         const std::vector<boost::shared_ptr<Tile> > &tiles)
    {
        int chosen_cat = -1;

        for (std::vector<boost::shared_ptr<Tile> >::const_iterator it = tiles.begin();
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

    void PlaceItem(DungeonMap &dmap,
                   const MapCoord &mc,
                   std::vector<boost::shared_ptr<Tile> > &tiles,
                   ItemType &itype,
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

    // -----------------------------------------------------------------------------

    // Connectivity check implementation

    void ConnectivityCheckImpl(DungeonMap &dmap, const MapCoord &from_where, int num_keys, ItemType &lockpicks)
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
            if (visited.find(mc) != visited.end()
                || blocked.find(mc) != blocked.end()
                || locked.find(mc) != locked.end()) continue;

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
        if (!visited.empty()) {
            std::vector<MapCoord> squares(visited.begin(), visited.end());
            
            // Up to 10 attempts.
            for (int i = 0; i < 10 && !lockpicks_placed; ++i) {
                const int r = g_rng.getInt(0, squares.size());
                const MapCoord mc = squares[r];
                
                // Work out whether we can place items here.
                vector<shared_ptr<Tile> > tiles;
                dmap.getTiles(mc, tiles);
                const int cat = FindItemCategory(dmap, mc, tiles);
                
                if (cat >= 0) {
                    
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
                        PlaceItem(dmap, mc, tiles, lockpicks, 1);
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
}


// ------------------------------------------------------------------------------

// Top level dungeon generation function

void DungeonGenerator(DungeonMap &dmap,
                      CoordTransform &ct,
                      HomeManager &home_manager,
                      MonsterManager &monster_manager,
                      const std::vector<Player*> &players,
                      const DungeonSettings &settings)
{
    int rwidth = 0, rheight = 0;
    FindRsize(settings.normal_segments, rwidth, rheight);
    FindRsize(settings.required_segments, rwidth, rheight);

    // Homes required is equal to number of players (unless H_NONE is set).
    const int homes_required = (settings.home_type == H_NONE ? 0 : players.size());

    int lwidth, lheight;
    std::vector<BlockInfo> edges, blocks;
    std::vector<bool> horiz_exits, vert_exits;
    
    DoLayout(players.size(), homes_required, *settings.layout, settings.home_type,
             lwidth, lheight, edges, blocks, horiz_exits, vert_exits);

    std::vector<SegmentInfo> segment_infos;
    std::vector<HomeInfo> assigned_homes, all_homes;
    
    PlaceSegments(players.size(), homes_required,
                  settings.home_type, lwidth, lheight, rwidth, rheight,
                  SegmentSet(settings.normal_segments),
                  settings.required_segments,
                  edges, blocks,
                  segment_infos, assigned_homes, all_homes);

    Compress(lwidth, lheight, rwidth, rheight,
             segment_infos, horiz_exits, vert_exits,
             assigned_homes, all_homes);

    dmap.create(lwidth*(rwidth+1)+1, lheight*(rheight+1)+1);
    
    CopySegmentsToMap(lwidth, lheight, rwidth, rheight,
                      settings.wall_tiles,
                      segment_infos,
                      dmap, ct, monster_manager);

    KnockThroughDoors(lwidth, lheight, rwidth, rheight,
                      settings.hdoor_tiles, settings.vdoor_tiles,
                      segment_infos, horiz_exits, vert_exits,
                      dmap);

    // Inform the home manager that we have added homes to the dungeon.
    for (std::vector<HomeInfo>::const_iterator it = all_homes.begin(); it != all_homes.end(); ++it) {
        // HomeManager requires the position just outside the home
        MapCoord pos(it->x, it->y);  // home square itself
        MapDirection facing = it->facing;  // points towards home
        pos = DisplaceCoord(pos, Opposite(facing));
        home_manager.addHome(dmap, pos, facing);
    }

    // Give players their homes
    for (size_t i = 0; i < assigned_homes.size() && i < players.size(); ++i) {
        MapCoord pos(assigned_homes[i].x, assigned_homes[i].y);  // home square itself
        MapDirection facing = assigned_homes[i].facing;  // points towards home
        pos = DisplaceCoord(pos, Opposite(facing));
        players[i]->resetHome(&dmap, pos, facing);
    }
}

// --------------------------------------------------------------------------------------

void GenerateLocksAndTraps(DungeonMap &dmap,
                           int nkeys,
                           bool pretrapped)
{
    std::vector<boost::shared_ptr<Tile> > tiles;
    for (int i = 0; i < dmap.getWidth(); ++i) {
        for (int j = 0; j < dmap.getHeight(); ++j) {
            MapCoord mc(i,j);
            dmap.getTiles(mc, tiles);
            for (std::vector<boost::shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end();
            ++it) {
                boost::shared_ptr<Lockable> lockable = dynamic_pointer_cast<Lockable>(*it);
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

// --------------------------------------------------------------------------------------

void GenerateItem(DungeonMap &dmap,
                  ItemType &itype,
                  const std::vector<std::pair<int,int> > &weights,
                  int total_weight)
{
    const int maxtries = 5;
    typedef std::vector<std::pair<int,int> > weight_table;
    ASSERT(total_weight > 0);
    
    const int w = dmap.getWidth(), h = dmap.getHeight();
    
    std::vector<boost::shared_ptr<Tile> > tiles;
    bool found = false;

    for (int tries = 0; tries < maxtries; ++tries) {
        // Select a tile category
        int t = g_rng.getInt(0, total_weight);
        int chosen_cat = -999;
        for (weight_table::const_iterator it = weights.begin(); it != weights.end(); ++it) {
            t -= it->second;
            if (t < 0) {
                chosen_cat = it->first;
                break;
            }
        }
        ASSERT(t<0);

        // Now randomly pick squares until we find one of the required
        // category. (Or, if this is the last try, we accept any
        // category in the weights table, even if it has weight of
        // zero.)
        MapCoord mc;
        for (int q = 0; q < w*h; ++q) {
            mc.setX(g_rng.getInt(0, w));
            mc.setY(g_rng.getInt(0, h));

            // work out the tile category
            dmap.getTiles(mc, tiles);
            const int cat = FindItemCategory(dmap, mc, tiles);

            if (cat == chosen_cat || tries == maxtries-1 && cat >= 0 && !Forbidden(cat, weights)) {
                // OK, put the item here
                PlaceItem(dmap, mc, tiles, itype, 1);
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

// --------------------------------------------------------------------------------------

void GenerateStuff(DungeonMap &dmap,
                   const std::map<int, StuffInfo> &stuff)
{
    std::vector<boost::shared_ptr<Tile> > tiles;
    for (int i = 0; i < dmap.getWidth(); ++i) {
        for (int j = 0; j < dmap.getHeight(); ++j) {
            MapCoord mc(i,j);

            // Find item-category associated with this tile. Non-negative value means we
            // should try to generate an item.
            dmap.getTiles(mc, tiles);
            const int chosen_cat = FindItemCategory(dmap, mc, tiles);
            
            // Look for the chosen category in our container of ItemGenerators.
            if (chosen_cat >= 0) {
                std::map<int, StuffInfo>::const_iterator it = stuff.find(chosen_cat);
                if (it != stuff.end() && g_rng.getBool(it->second.chance)) {
                    // We are to generate an item
                    std::pair<ItemType *, int> result = it->second.gen.get();
                    ASSERT(result.first);
                    PlaceItem(dmap, mc, tiles, *result.first, result.second);
                }
            }
        }
    }
}

// --------------------------------------------------------------------------------------

void GenerateMonsters(DungeonMap &dmap,
                      MonsterManager &mmgr,
                      const MonsterType &mtype,
                      int num_monsters)
{
    const MapHeight monster_height = mtype.getHeight();
    
    std::vector<boost::shared_ptr<Tile> > tiles;
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

// --------------------------------------------------------------------------------------

void ConnectivityCheck(const std::vector<Player*> &players,
                       int num_keys,
                       ItemType &lockpicks)
{
    // Run the connectivity check from every player's starting square, in turn.
    for (std::vector<Player*>::const_iterator it = players.begin(); it != players.end(); ++it) {
        DungeonMap *dmap = (*it)->getHomeMap();
        const MapCoord &mc = (*it)->getHomeLocation();

        if (dmap && !mc.isNull()) {
            ConnectivityCheckImpl(*dmap, mc, num_keys, lockpicks);
        }
    }
}

// --------------------------------------------------------------------------------------

void CheckTutorial(Player &player)
{
    DungeonMap *dmap = player.getHomeMap();
    MapCoord home_pos = player.getHomeLocation();  // the floor space in front of the home.
    MapDirection facing_toward_home = player.getHomeFacing();

    // Check the home and the eight surrounding squares (except the entry point square itself).
    for (int x = home_pos.getX() - 1; x <= home_pos.getX() + 1; ++x) {
        for (int y = home_pos.getY() - 1; y <= home_pos.getY() + 1; ++y) {
            MapCoord mc(x,y);
            if (dmap->valid(mc) && mc != DisplaceCoord(home_pos, facing_toward_home)) {

                // Check there are no tutorial tiles on this square
                std::vector<boost::shared_ptr<Tile> > tiles;
                dmap->getTiles(mc, tiles);
                for (std::vector<boost::shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
                    if ((*it)->getTutorialKey() > 0) throw DungeonGenerationFailed();
                }

                // Check there are no items on this square
                if (dmap->getItem(mc)) throw DungeonGenerationFailed();
            }
        }
    }

    // Check that any wooden doors in the starting room are unlocked.
    const RoomMap *rmap = dmap->getRoomMap();
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
                    dmap->getTiles(mc, tiles);
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

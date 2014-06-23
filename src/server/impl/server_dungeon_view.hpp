/*
 * server_dungeon_view.hpp
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

#ifndef SERVER_DUNGEON_VIEW_HPP
#define SERVER_DUNGEON_VIEW_HPP

#include "dungeon_view.hpp"

#include <list>
#include <map>
#include <vector>

class ServerDungeonView : public DungeonView {
public:
    typedef unsigned char ubyte;
    
    explicit ServerDungeonView(std::vector<ubyte> &out_) : out(out_), current_room(-1),
                                                           current_room_width(0), current_room_height(0) { }

    void appendDungeonViewCmds(int observer_num, std::vector<ubyte> &vec);
    void clearDungeonViewCmds();
    void rmObserverNum(int observer_num);  // clear caches

    virtual void setCurrentRoom(int r, int width, int height);

    virtual void addEntity(unsigned short int id, int x, int y, MapHeight ht, MapDirection facing,
                           const Anim * anim, const Overlay *ovr, int af, int atz_diff,
                           bool ainvis, bool ainvuln, // (anim data)
                           bool approached,
                           int cur_ofs, MotionType motion_type, int motion_time_remaining,
                           const UTF8String &name);
    virtual void rmEntity(unsigned short int id);
    virtual void repositionEntity(unsigned short int id, int new_x, int new_y);
    virtual void moveEntity(unsigned short int id, MotionType motion_type,
                            int motion_duration, bool missile_mode);
    virtual void flipEntityMotion(unsigned short int id, int initial_delay, int motion_duration);
    virtual void setAnimData(unsigned short int id, const Anim *, const Overlay *, int af,
                             int atz_diff, bool ainvis, bool ainvuln, bool currently_moving);
    virtual void setFacing(unsigned short int id, MapDirection new_facing);
    virtual void setSpeechBubble(unsigned short int id, bool show);

    virtual void clearTiles(int x, int y, bool force);
    virtual void setTile(int x, int y, int depth, const Graphic *gfx, boost::shared_ptr<const ColourChange> cc, bool force);

    virtual void setItem(int x, int y, const Graphic *gfx, bool force);

    virtual void placeIcon(int x, int y, const Graphic *gfx, int dur);

    virtual void flashMessage(const std::string &msg, int ntimes);
    virtual void cancelContinuousMessages();
    virtual void addContinuousMessage(const std::string &msg);

private:
    std::vector<ubyte> &out;

    int current_room;
    int current_room_width, current_room_height;

    enum SquareState {
        UNSEEN,
        SEEN,
        ITEM_CLEARED
    };
    
    struct RoomData {
        int width, height;
        std::vector<SquareState> square_seen; // whether this observer has "seen" this square before (i.e. had tiles/items sent).
    };

    // first = observer num
    // second = room num
    std::map<std::pair<int, int>, RoomData> cached_rooms;

    struct Cmd {
        enum { CLEAR_TILES, SET_TILE, SET_ITEM } type;
        int x;
        int y;
        int depth;
        const Graphic *gfx;
        boost::shared_ptr<const ColourChange> cc;
        bool force;
    };

    std::vector<Cmd> cmds;
};

#endif

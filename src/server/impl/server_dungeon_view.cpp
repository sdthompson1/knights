/*
 * server_dungeon_view.cpp
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

#include "anim.hpp"
#include "graphic.hpp"
#include "overlay.hpp"
#include "protocol.hpp"
#include "server_dungeon_view.hpp"

#include <limits>

namespace {
    void WriteRoomCoord(Coercri::OutputByteBuf &buf, int x, int y)
    {
        buf.writeNibbles(x+1, y+1);
    }

    void WriteTileInfo(Coercri::OutputByteBuf &buf, int depth, bool cc)
    {
        int d = depth + 64;
        if (d < 0 || d > 127) throw ProtocolError("WriteTileInfo: depth out of range");
        buf.writeUbyte(cc ? 128+d : d);
    }

    int ClampToUshort(int x)
    {
        if (x < 0) return 0;
        else if (x > 65535) return 65535;
        else return x;
    }

    int ClampToShort(int x)
    {
        if (x < -32768) return -32768;
        else if (x > 32767) return 32767;
        else return x;
    }
}


void ServerDungeonView::appendDungeonViewCmds(int observer_num, std::vector<ubyte> &vec)
{
    Coercri::OutputByteBuf buf(vec);

    // First find the RoomData corresponding to the current room (for this observer_num)
    std::map<std::pair<int,int>, RoomData>::iterator room_data_it = cached_rooms.find(std::make_pair(observer_num, current_room));
    if (room_data_it == cached_rooms.end()) {
        RoomData r;
        r.width = current_room_width;
        r.height = current_room_height;
        r.square_seen.resize(current_room_width * current_room_height);
        room_data_it = cached_rooms.insert(std::make_pair(std::make_pair(observer_num, current_room), r)).first;
    }
    RoomData & room_data = room_data_it->second;

    // Now run through the commands.
    for (std::vector<Cmd>::const_iterator cmd_it = cmds.begin(); cmd_it != cmds.end(); ++cmd_it) {

        const int idx = cmd_it->y * current_room_width + cmd_it->x;
        const SquareState seen = room_data.square_seen[idx];
        const bool must_send = cmd_it->force || (seen != SEEN);

        if (must_send) {
            switch (cmd_it->type) {
            case Cmd::SET_TILE:
                buf.writeUbyte(SERVER_SET_TILE);
                WriteRoomCoord(buf, cmd_it->x, cmd_it->y);
                WriteTileInfo(buf, cmd_it->depth, bool(cmd_it->cc));
                buf.writeVarInt(cmd_it->gfx ? cmd_it->gfx->getID() : 0);
                if (cmd_it->cc) cmd_it->cc->serialize(buf);
                break;
            case Cmd::CLEAR_TILES:
                buf.writeUbyte(SERVER_CLEAR_TILES);
                WriteRoomCoord(buf, cmd_it->x, cmd_it->y);
                break;
            case Cmd::SET_ITEM:
                {
                    // default state for unseen squares is no item so no point sending 
                    // "SET_ITEM NULL" cmds in that case... it just wastes bandwidth
                    const bool they_already_know = (seen==UNSEEN) && cmd_it->gfx == 0;
                    if (!they_already_know) {
                        buf.writeUbyte(SERVER_SET_ITEM);
                        WriteRoomCoord(buf, cmd_it->x, cmd_it->y);
                        buf.writeVarInt(cmd_it->gfx ? cmd_it->gfx->getID() : 0);
                    }
                }
                break;
            }
        }

        if (seen != SEEN) {
            // Mark the square as seen, so that future (unforced) updates are not re-sent unnecessarily.
            // Note: we want all commands in a "batch" to be sent BEFORE marking the square seen.
            // So look ahead at the next cmd and if it's on this square too, then hold off on setting 'seen'.
            std::vector<Cmd>::const_iterator look_ahead = cmd_it;
            ++look_ahead;
            if (look_ahead == cmds.end() || look_ahead->x != cmd_it->x || look_ahead->y != cmd_it->y) {
                room_data.square_seen[idx] = SEEN;
            }
        }
    }
}

void ServerDungeonView::clearDungeonViewCmds()
{
    cmds.clear();
}

void ServerDungeonView::rmObserverNum(int observer_num)
{
    std::map<std::pair<int,int>, RoomData>::iterator
        first = cached_rooms.lower_bound(std::make_pair(observer_num, std::numeric_limits<int>::min())),
        last  = cached_rooms.upper_bound(std::make_pair(observer_num, std::numeric_limits<int>::max()));
    cached_rooms.erase(first, last);
}


void ServerDungeonView::setCurrentRoom(int r, int width, int height)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_SET_CURRENT_ROOM);
    buf.writeVarInt(r);
    WriteRoomCoord(buf, width, height);

    // If there are any (forced) commands in the queue for the old room, then we have a problem,
    // since these will be 'lost' (we don't have any mechanism to queue these up across the room change).
    // The simplest solution is to mark those squares as 'unseen', so that they will be refreshed
    // when we next enter this room.
    bool force_cmd_exists = false;
    for (std::vector<Cmd>::const_iterator it = cmds.begin(); it != cmds.end(); ++it) {
        if (it->force) {
            force_cmd_exists = true;
            break;
        }
    }
    if (force_cmd_exists) {
        for (std::map<std::pair<int,int>, RoomData>::iterator it = cached_rooms.begin(); it != cached_rooms.end(); ++it) {
            if (it->first.second == current_room) {
                for (std::vector<Cmd>::const_iterator cmd_it = cmds.begin(); cmd_it != cmds.end(); ++cmd_it) {
                    if (cmd_it->force) {
                        const int idx = cmd_it->y * current_room_width + cmd_it->x;
                        it->second.square_seen[idx] = ITEM_CLEARED;  // make sure the item gets resent. fixes scroll bug.
                    }
                }
            }
        }
    }

    // Now we can safely drop any existing cmds.
    cmds.clear();
    
    // Update the current_room variables
    current_room = r;
    current_room_width = width;
    current_room_height = height;
}

void ServerDungeonView::addEntity(unsigned short int id, int x, int y, MapHeight ht, MapDirection facing,
                                  const Anim *anim, const Overlay *ovr, int af, int atz_diff,
                                  bool ainvuln,
                                  int cur_ofs, MotionType motion_type, int motion_time_remaining,
                                  const std::string &name)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_ADD_ENTITY);
    buf.writeVarInt(id);
    WriteRoomCoord(buf, x, y);
    buf.writeNibbles(int(ht), facing);
    buf.writeVarInt(anim ? anim->getID() : 0);
    buf.writeVarInt(ovr ? ovr->getID() : 0);
    buf.writeNibbles(af, (int(motion_type)<<1) + int(ainvuln));
    if (af != 0) buf.writeShort(ClampToShort(atz_diff));
    buf.writeUshort(ClampToUshort(cur_ofs));
    if (motion_type != MT_NOT_MOVING) buf.writeUshort(ClampToUshort(motion_time_remaining));
    buf.writeString(name);
}

void ServerDungeonView::rmEntity(unsigned short int id)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_RM_ENTITY);
    buf.writeVarInt(id);
}

void ServerDungeonView::repositionEntity(unsigned short int id, int new_x, int new_y)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_REPOSITION_ENTITY);
    buf.writeVarInt(id);
    WriteRoomCoord(buf, new_x, new_y);
}

void ServerDungeonView::moveEntity(unsigned short int id, MotionType motion_type,
                                   int motion_duration, bool missile_mode)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_MOVE_ENTITY);
    buf.writeVarInt(id);
    buf.writeNibbles(motion_type, missile_mode?1:0);
    buf.writeUshort(ClampToUshort(motion_duration));
}

void ServerDungeonView::flipEntityMotion(unsigned short int id, int initial_delay, int motion_duration)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_FLIP_ENTITY_MOTION);
    buf.writeVarInt(id);
    buf.writeUshort(ClampToUshort(initial_delay));
    buf.writeUshort(ClampToUshort(motion_duration));
}

void ServerDungeonView::setAnimData(unsigned short int id, const Anim *anim, const Overlay *ovr,
                                    int af, int atz_diff, bool ainvuln, bool currently_moving)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_SET_ANIM_DATA);
    buf.writeVarInt(id);
    buf.writeVarInt(anim ? anim->getID() : 0);
    buf.writeVarInt(ovr ? ovr->getID() : 0);
    buf.writeNibbles(af, int(ainvuln)*2 + int(currently_moving));
    buf.writeShort(ClampToShort(atz_diff));
}

void ServerDungeonView::setFacing(unsigned short int id, MapDirection new_facing)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_SET_FACING);
    buf.writeVarInt(id);
    buf.writeUbyte(new_facing);
}

void ServerDungeonView::setSpeechBubble(unsigned short int id, bool show)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_SET_SPEECH_BUBBLE);
    buf.writeVarInt(id);
    buf.writeUbyte(show ? 1 : 0);
}

void ServerDungeonView::clearTiles(int x, int y, bool force)
{
    Cmd t;
    t.type = Cmd::CLEAR_TILES;
    t.x = x;
    t.y = y;
    t.force = force;
    cmds.push_back(t);
}

void ServerDungeonView::setTile(int x, int y, int depth, const Graphic *gfx, boost::shared_ptr<const ColourChange> cc, bool force)
{
    Cmd t;
    t.type = Cmd::SET_TILE;
    t.x = x;
    t.y = y;
    t.depth = depth;
    t.gfx = gfx;
    t.cc = cc;
    t.force = force;
    cmds.push_back(t);
}

void ServerDungeonView::setItem(int x, int y, const Graphic *gfx, bool force)
{
    Cmd i;
    i.type = Cmd::SET_ITEM;
    i.x = x;
    i.y = y;
    i.gfx = gfx;
    i.force = force;
    cmds.push_back(i);
}

void ServerDungeonView::placeIcon(int x, int y, const Graphic *gfx, int dur)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_PLACE_ICON);
    WriteRoomCoord(buf, x, y);
    buf.writeVarInt(gfx ? gfx->getID() : 0);
    buf.writeUshort(ClampToUshort(dur));
}

void ServerDungeonView::flashMessage(const std::string &msg, int ntimes)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_FLASH_MESSAGE);
    buf.writeString(msg);
    buf.writeUbyte(ntimes);
}

void ServerDungeonView::cancelContinuousMessages()
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_CANCEL_CONTINUOUS_MESSAGES);
}

void ServerDungeonView::addContinuousMessage(const std::string &msg)
{
    Coercri::OutputByteBuf buf(out);
    buf.writeUbyte(SERVER_ADD_CONTINUOUS_MESSAGE);
    buf.writeString(msg);
}

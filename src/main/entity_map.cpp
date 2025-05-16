/*
 * entity_map.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2025.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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
#include "config_map.hpp"
#include "entity_map.hpp"
#include "graphic_element.hpp"
#include "overlay.hpp"
#include "round.hpp"
#include "rng.hpp"

using std::list;
using std::make_pair;
using std::map;
using std::vector;

EntityMap::EntityMap(const ConfigMap &cfg, int approach_offset_)
 : bat_anim_timescale_us(int64_t(cfg.getInt("bat_anim_timescale")) * 1000),
   approach_offset(approach_offset_),
   vbat_last_time_us(-5000000)
{
    for (int i = 0; i < 64; ++i) vbat_frames[i] = 0;
}


void EntityMap::getCurrentOffset(int64_t time_us, const Data &ent, const Command &cmd,
                                 int64_t &nt_so_far_us, int &cur_ofs)
{
    ASSERT(cmd.type == Command::MOVE);

    nt_so_far_us = (time_us - ent.start_time_us) * ent.tnt_us / (ent.finish_time_us - ent.start_time_us);
    if (nt_so_far_us < 0) nt_so_far_us = 0;
    if (nt_so_far_us > cmd.move_info.nt_us) nt_so_far_us = cmd.move_info.nt_us;

    const int fo = getFinalOffset(cmd.move_info.type);
    cur_ofs = cmd.move_info.so + (fo - cmd.move_info.so) * nt_so_far_us / cmd.move_info.nt_us;
}

int EntityMap::getFinalOffset(MotionType type)
{
    switch (type) {
    case MT_MOVE: return 1000;
    case MT_APPROACH: return approach_offset;
    case MT_WITHDRAW: return 0;
    default: ASSERT(0); return 0;
    }
}

void EntityMap::addEntity(int64_t time_us, unsigned short int key, int x, int y, MapHeight h, MapDirection facing,
                          const Anim *anim, const Overlay *ovr, int af, int64_t atz_us, bool ainvis, bool ainvuln,
                          int cur_ofs, MotionType motion_type, int64_t motion_time_remaining_us,
                          const UTF8String &name)
{
    Data d;
    d.x = x;
    d.y = y;
    d.height = h;
    d.anim = anim;
    d.ovr = ovr;
    d.af = af;
    d.atz_us = atz_us;
    d.ainvis = ainvis;
    d.ainvuln = ainvuln;
    d.facing = facing;
    d.approached = (motion_type == MT_NOT_MOVING && cur_ofs != 0);
    d.show_speech_bubble = false;
    d.name = name;
    if (motion_type == MT_NOT_MOVING) {
        d.start_time_us = 0;
        d.finish_time_us = 0;
        d.tnt_us = 0;
    } else {
        d.start_time_us = time_us;
        d.finish_time_us = time_us + motion_time_remaining_us;
        d.tnt_us = motion_time_remaining_us;
        Command cmd;
        cmd.type = Command::MOVE;
        cmd.move_info.so = cur_ofs;
        cmd.move_info.type = motion_type;
        cmd.move_info.nt_us = motion_time_remaining_us;
        d.cmds.push_back(cmd);
    }
    entities.insert(make_pair(key, d));
}

void EntityMap::rmEntity(unsigned short int key)
{
    map<unsigned short int,Data>::iterator it = entities.find(key);
    if (it == entities.end()) return;

    entities.erase(it);
}

void EntityMap::recomputeEntityMotion(map<unsigned short int,Data>::iterator it, int64_t time_us)
{
    // Make sure any "reposition" or similar commands are popped
    // from the top of the command queue.
    update(time_us, it->second);

    // If a motion cmd is in progress then change its start time to
    // current time, and update its start offset to current
    // offset. This will prevent the current offset "jumping" when we
    // add a new command.
    if (!it->second.cmds.empty()) {
        // A motion command is in progress.
        Command &cmd(it->second.cmds.front());
        ASSERT(cmd.type == Command::MOVE);
        ASSERT(cmd.move_info.nt_us != 0);
        ASSERT(it->second.start_time_us < it->second.finish_time_us);

        // calculate the offset of the entity right now (taking into
        // account the motion cmd that is currently in progress).
        int64_t nt_so_far_us;
        int cur_ofs;
        getCurrentOffset(time_us, it->second, cmd, nt_so_far_us, cur_ofs);
        
        // Update the existing command
        cmd.move_info.so = cur_ofs;
        cmd.move_info.nt_us -= nt_so_far_us;
        it->second.tnt_us -= nt_so_far_us;
    } else {
        ASSERT(it->second.tnt_us == 0);
    }

    it->second.start_time_us = time_us;
}       

void EntityMap::moveEntity(int64_t time_us, unsigned short int key,
                           MotionType motion_type, int64_t motion_duration_us,
                           bool missile_mode)
{
    if (motion_type == MT_NOT_MOVING) return;
        
    map<unsigned short int,Data>::iterator it = entities.find(key);
    if (it == entities.end()) return;

    // Adjust the command sequence so that it's safe to add new motion
    // cmds afterwards.
    recomputeEntityMotion(it, time_us);

    // New starting offset
    const int new_so = missile_mode? 500 :
        (it->second.approached? approach_offset : 0);

    // Check if there is an existing command
    const bool existing_cmd = (!it->second.cmds.empty());

    // Add a new command to represent the move
    Command cmd;
    cmd.type = Command::MOVE;
    cmd.move_info.so = new_so;
    cmd.move_info.nt_us = motion_duration_us;
    cmd.move_info.type = motion_type;
    it->second.cmds.push_back(cmd);
    it->second.approached = (motion_type == MT_APPROACH);
    it->second.tnt_us += cmd.move_info.nt_us;

    if (existing_cmd) {
        // For the new move to play at the correct speed, we theoretically need to
        // allow all current moves to finish, then take exactly "motion_duration_us"
        // microseconds to play out the new move.

        // This, however, will leave us lagging behind the server by
        // (finish_time_us - time_us) microseconds.

        // We will cap the max allowed lag at THRESHOLD_US
        // microseconds. If lag would be more than this, then we
        // effectively "speed up" local animations, by setting an
        // earlier finish_time.

        const int THRESHOLD_MS = 100;  // Max acceptable lag behind the server (in milliseconds)
        const int64_t THRESHOLD_US = int64_t(THRESHOLD_MS) * 1000;

        it->second.finish_time_us =
            std::min(it->second.finish_time_us, time_us + THRESHOLD_US) + motion_duration_us;

    } else {
        // Just set it to finish at the expected time, i.e. motion_duration from now.
        it->second.finish_time_us = time_us + motion_duration_us;
    }
}

void EntityMap::flipEntityMotion(int64_t time_us, unsigned short int key,
                                 int64_t initial_delay_us, int64_t input_motion_duration_us)
{
    // We handle this by making a move of duration (initial_delay + input_motion_duration),
    // then fixing up the initial_delay at the end.
    ASSERT(input_motion_duration_us > 0);
    const int64_t actual_motion_duration_us = input_motion_duration_us + initial_delay_us;
        
    map<unsigned short int,Data>::iterator it = entities.find(key);
    if (it == entities.end()) return;

    // Adjust the command block so that it starts at the current time.
    recomputeEntityMotion(it, time_us);

    // Two cases here:
    // 
    // (i) The MOVE command that the flip applies to has already been
    // processed and discarded. In this case the command list should
    // be empty.
    //
    // (ii) The MOVE command that the flip applies to is still around
    // (either currently executing, or queued for future
    // execution). In this case it must be the very last thing on the
    // command list. (This is due to how movement works, you can't
    // e.g. do a SET_FACING *then* a flip-motion, the only time you
    // can flip motion is immediately following a MOVE cmd.)

    ASSERT(it->second.cmds.empty() ||
           (it->second.cmds.back().type == Command::MOVE && it->second.cmds.back().move_info.type == MT_MOVE));

    if (it->second.cmds.empty()) {
        // Case (i). Here we need to manually turn the entity around and
        // start a normal motion.
        setFacing(key, Opposite(it->second.facing));
        moveEntity(time_us, key, MT_MOVE, actual_motion_duration_us, false);
    } else {
        // Case (ii). What we do here depends on whether the flip
        // applies to the currently executing motion cmd (the one at
        // the head of the queue) or to some future motion cmd.
        Command & cmd(it->second.cmds.back());
        if (&cmd == &(it->second.cmds.front())) {
            // Case (ii)(a)
            // The flip applies to a motion cmd that is in progress
            // *right now*. We can alter the cmd directly.

            // Reverse the current motion direction (by updating pos & facing)
            MapCoord new_pos = DisplaceCoord(MapCoord(it->second.x, it->second.y), it->second.facing);
            it->second.x = new_pos.getX();
            it->second.y = new_pos.getY();
            it->second.facing = Opposite(it->second.facing);
            cmd.move_info.so = 1000 - cmd.move_info.so;

            // Update the motion duration and finish time
            const int64_t old_nt_us = cmd.move_info.nt_us;
            const int64_t new_nt_us = actual_motion_duration_us;
            cmd.move_info.nt_us = new_nt_us;
            it->second.tnt_us += (new_nt_us - old_nt_us);
            it->second.finish_time_us = time_us + actual_motion_duration_us;
        } else {
            // Case (ii)(b)
            //
            // The flip applies to some motion command that is to
            // execute in the future. (This can only happen if the
            // client is lagging behind the server considerably.)
            // We proceed as follows:
            // 
            // 1. Adjust this motion command to only take one time
            // unit.
            // 2. Add a set-facing/move-entity command sequence (as
            // in case (i) above) after that.
            //
            // Unfortunately this will result in the entity whizzing
            // all the way forward to the next square before turning
            // back (whereas we want the flip to apply somewhere
            // halfway through the move). This is acceptable for now
            // though (since this should all happen very quickly --
            // the user will hopefully hardly see it -- and this case
            // should be fairly rare anyway).

            const int64_t old_nt_us = cmd.move_info.nt_us;
            cmd.move_info.nt_us = 1;
            it->second.tnt_us += (1 - old_nt_us);
            setFacing(key, Opposite(it->second.facing));
            moveEntity(time_us, key, MT_MOVE, actual_motion_duration_us, false);
        }
    }

    // Add in the initial_delay, by doing some surgery on the
    // start_time and nt values.
    // (unless there is more than one move command in the queue -- in
    // which case should probably get things moving as quickly as
    // possible.)
    if (!it->second.cmds.empty()
    && it->second.cmds.front().type == Command::MOVE
    && it->second.cmds.front().move_info.nt_us == it->second.tnt_us) {
        ASSERT(it->second.tnt_us == actual_motion_duration_us);
        int64_t time_increase_us = initial_delay_us;

        // Account for any existing delay
        // (TODO: is this actually necessary?)
        if (it->second.start_time_us > time_us) time_increase_us -= (it->second.start_time_us - time_us);
        if (time_increase_us < 0) time_increase_us = 0;
                
        it->second.start_time_us += time_increase_us;
        it->second.cmds.front().move_info.nt_us -= time_increase_us;
        it->second.tnt_us -= time_increase_us;
    }
}

void EntityMap::repositionEntity(unsigned short int key, int new_x, int new_y)
{
    map<unsigned short int,Data>::iterator it = entities.find(key);
    if (it == entities.end()) return;
        
    Command cmd;
    cmd.type = Command::REPOSITION;
    cmd.reposition_info.x = new_x;
    cmd.reposition_info.y = new_y;
    it->second.cmds.push_back(cmd);
    it->second.approached = false;
}

void EntityMap::setAnimData(unsigned short int key, const Anim *anim, const Overlay *ovr,
                            int af, int64_t atz_us, bool ainvis, bool ainvuln, bool during_motion)
{
    map<unsigned short int, Data>::iterator it = entities.find(key);
    if (it == entities.end()) return;
    Command cmd;
    cmd.type = Command::SET_ANIM;
    cmd.anim_info.anim = anim;
    cmd.anim_info.ovr = ovr;
    cmd.anim_info.af = af;
    cmd.anim_info.atz_us = atz_us;
    cmd.anim_info.ainvis = ainvis;
    cmd.anim_info.ainvuln = ainvuln;

    if (during_motion) {
        // If "during_motion" is set then we add the new cmd before
        // the final motion cmd - this happens for things like
        // attack-while-moving, where we want the new anim to appear
        // instantly, not to wait until the end of the current move

        list<Command> & cmds = it->second.cmds;
        list<Command>::iterator ins = cmds.end();

        while (ins != cmds.begin()) {
            --ins;
            if (ins->type == Command::MOVE) {
                break;
            }
        }

        cmds.insert(ins, cmd);

    } else {
        // If during_motion is false, add the new cmd at the end of
        // the queue - this is for things like a change of facing,
        // where we *do* want to wait for the current move to finish.

        it->second.cmds.push_back(cmd);
    }
}

void EntityMap::setFacing(unsigned short int key, MapDirection f)
{
    map<unsigned short int,Data>::iterator it = entities.find(key);
    if (it == entities.end()) return;
    Command cmd;
    cmd.type = Command::SET_FACING;
    cmd.facing_info = f;
    it->second.cmds.push_back(cmd);
}

void EntityMap::setSpeechBubble(unsigned short int key, bool show)
{
    map<unsigned short int,Data>::iterator it = entities.find(key);
    if (it == entities.end()) return;
    // This bypasses the command queue and is just executed immediately
    it->second.show_speech_bubble = show;
}

void EntityMap::update(int64_t time_us, EntityMap::Data &ent)
{
    while (!ent.cmds.empty()) {
        const Command &cmd(ent.cmds.front());

        switch (cmd.type) {
        case Command::MOVE:
            {
                int64_t dur_us = (ent.finish_time_us - ent.start_time_us);
                int64_t fini_time_us = (cmd.move_info.nt_us * dur_us / ent.tnt_us)
                    + ent.start_time_us;
                if (time_us < fini_time_us) {
                    // This command is still in progress
                    return;
                } else {
                    // This command has completed
                    ent.tnt_us -= cmd.move_info.nt_us;
                    // Next command starts when previous one finished:
                    ent.start_time_us = fini_time_us;
                    // If it was a "move" command, we have to update position, too:
                    // (it's no good waiting for the server to send the reposition command,
                    // because while we're waiting, the entity will have the wrong
                    // pos/offset.)
                    if (cmd.move_info.type == MT_MOVE) {
                        switch (ent.facing) {
                        case D_NORTH:
                            --ent.y;
                            break;
                        case D_EAST:
                            ++ent.x;
                            break;
                        case D_SOUTH:
                            ++ent.y;
                            break;
                        case D_WEST:
                            --ent.x;
                            break;
                        }
                    }
                }
            }
            break;
                        
        case Command::REPOSITION:
            ent.x = cmd.reposition_info.x;
            ent.y = cmd.reposition_info.y;
            break;
                        
        case Command::SET_ANIM:
            ent.anim = cmd.anim_info.anim;
            ent.ovr = cmd.anim_info.ovr;
            ent.af = cmd.anim_info.af;
            ent.atz_us = cmd.anim_info.atz_us;
            ent.ainvis = cmd.anim_info.ainvis;
            ent.ainvuln = cmd.anim_info.ainvuln;
            break;
                        
        case Command::SET_FACING:
            ent.facing = cmd.facing_info;
            break;
        }
                
        ent.cmds.pop_front();
    }
}
        
        
void EntityMap::update(int64_t time_us)
{
    for (map<unsigned short int,Data>::iterator it = entities.begin(); it != entities.end(); ++it) {
        update(time_us, it->second);
    }
}

const Graphic * EntityMap::chooseGraphic(const Anim *anim,
                                         MapDirection facing, int frame,
                                         int x, int y, int64_t time_us)
{
    if (!anim->getVbatMode()) {
        // Straight lookup
        return anim->getGraphic(facing, frame);
    } else {
        // Special lookup for vampire bats.
        if (frame != 0) {
            // "Biting" frame
            return anim->getGraphic(D_WEST, 0);
        } else {
            // "Flapping" frame -- this has to be chosen randomly.
            if (time_us > vbat_last_time_us + (bat_anim_timescale_us<<7)) {
                // (every 2**7 == 128 timescales, we regenerate the random table.)
                for (int i = 0; i < 64; ++i) {
                    vbat_frames[i] = g_rng.getInt(0,3);
                }
                vbat_last_time_us = time_us;
            }

            // The exact graphic is chosen based on time, facing and
            // position. This should sufficiently randomize things.
            // (Note we don't just switch graphics each frame; that 
            // would be way too fast. The procedure here is designed 
            // to make the same graphic remain for a fixed time before 
            // we switch to a new, randomly chosen graphic.)
            int idx = time_us / bat_anim_timescale_us + int(facing) + (x<<3) + y;
            idx >>= 2;
            idx = idx & 63;
            return anim->getGraphic(MapDirection(vbat_frames[idx]), 0);
        }
    }
}

void EntityMap::addGraphic(const Data &ent, int64_t time_us, int tl_x, int tl_y, int entity_depth,
                           map<int, vector<GraphicElement> > &gfx_buffer,
                           vector<TextElement> &txt_buffer,
                           int pixels_per_square, bool add_entity_name,
                           const Graphic *speech_bubble,
                           int speech_depth)
{
    const Anim *anim = ent.anim;
    if (anim) {
                
        // Work out the graphic and colour-change for this frame
        const int frame = (time_us >= ent.atz_us && ent.atz_us > 0) ? 0 : ent.af;
        const Graphic *lower_graphic = chooseGraphic(anim, ent.facing, frame, ent.x, ent.y, time_us);
        const ColourChange *lower_cc = &(anim->getColourChange(ent.ainvuln));
        
        if (lower_graphic) {
            int ox, oy;
            int ofs;
            if (ent.cmds.empty()) {
                ofs = ent.approached? approach_offset : 0;
            } else {
                const Command &cmd(ent.cmds.front());
                ASSERT(cmd.type == Command::MOVE);
                int64_t dummy;
                // Set ofs to current offset
                getCurrentOffset(time_us, ent, cmd, dummy, ofs);
            }
            switch (ent.facing) {
            case D_NORTH:
                ox = 0;
                oy = -ofs;
                break;
            case D_EAST:
                ox = ofs;
                oy = 0;
                break;
            case D_SOUTH:
                ox = 0;
                oy = ofs;
                break;
            case D_WEST:
                ox = -ofs;
                oy = 0;
                break;
            }
            
            const int d = entity_depth - 10*ent.height;

            GraphicElement ge;
            const int entity_x = tl_x
                + ent.x * pixels_per_square
                + DivRoundNearest(ox * pixels_per_square, 1000);
            ge.sx = entity_x;
            const int entity_y = tl_y
                + ent.y * pixels_per_square
                + DivRoundNearest(oy * pixels_per_square, 1000);
            ge.sy = entity_y;
            ge.gr = lower_graphic;
            ge.cc = lower_cc;
            ge.semitransparent = ent.ainvis;
            gfx_buffer[d].push_back(ge);

            const Overlay *ovr = ent.ovr;
            if (ovr) {
                const Graphic *upper_graphic;
                int upper_ofsx, upper_ofsy;
                ovr->getGraphic(ent.facing, frame, upper_graphic, upper_ofsx, upper_ofsy);
                
                if (upper_graphic) {
                    // rescale overlay offsets from points to pixels
                    upper_ofsx = Round(float(upper_ofsx) / 1000.0f * float(pixels_per_square));
                    upper_ofsy = Round(float(upper_ofsy) / 1000.0f * float(pixels_per_square));

                    // add the overlay to the graphics buffer
                    ge.gr = upper_graphic;
                    ge.cc = 0;
                    ge.sx += upper_ofsx;
                    ge.sy += upper_ofsy;
                    gfx_buffer[d-1].push_back(ge);
                }
            }

            if (ent.show_speech_bubble && speech_bubble) {
                ge.gr = speech_bubble;
                ge.sx = entity_x;
                ge.sy = entity_y;
                ge.cc = 0;
                gfx_buffer[speech_depth].push_back(ge);
            }
            
            if (!ent.name.asUTF8().empty() && add_entity_name) {
                TextElement te;
                te.sx = entity_x + (pixels_per_square/2);
                te.sy = entity_y + (pixels_per_square/2);
                te.text = ent.name;
                txt_buffer.push_back(te);
            }
        }
    }
}

void EntityMap::getEntityGfx(int64_t time_us, int tl_x, int tl_y, int pixels_per_square, int entity_depth,
                             map<int, vector<GraphicElement> > &gfx_buffer,
                             vector<TextElement> &txt_buffer,
                             bool show_own_name,
                             const Graphic *speech_bubble,
                             int speech_depth)
{
    update(time_us);
        
    for (map<unsigned short int,Data>::iterator it = entities.begin(); it != entities.end(); ++it) {
        addGraphic(it->second, time_us, tl_x, tl_y, entity_depth, gfx_buffer, txt_buffer,
                   pixels_per_square, show_own_name || it->first != 0,
                   speech_bubble, speech_depth);
    }
}

/*
 * entity_map.hpp
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

/*
 * Holds Entities within a LocalDungeonView.
 *
 */

#ifndef ENTITY_MAP_HPP
#define ENTITY_MAP_HPP

#include "client_callbacks.hpp"
#include "graphic_element.hpp"  // MSVC compiler gives a link error on getEntityMap if we don't have this...
#include "map_support.hpp"

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <vector>

class Anim;
class ConfigMap;
class GraphicElement;
class Overlay;

class EntityMap {
public:
    explicit EntityMap(const ConfigMap &cfg, int approach_offset_);

    void addEntity(int64_t time_us, unsigned short int key, int x, int y, MapHeight h, MapDirection facing,
                   const Anim *anim, const Overlay *ovr, int af, int64_t atz_us, bool ainvis, bool ainvuln,
                   int cur_ofs, MotionType motion_type, int64_t motion_time_remaining_us,
                   const UTF8String &name);
    void rmEntity(unsigned short int key);
    void moveEntity(int64_t time_us, unsigned short int key, MotionType motion_type, int64_t motion_duration_us, bool missile_mode);
    void flipEntityMotion(int64_t time_us, unsigned short int key, int64_t initial_delay_us, int64_t motion_duration_us);
    void repositionEntity(unsigned short int key, int new_x, int new_y);
    void setAnimData(unsigned short int key, const Anim *, const Overlay *,
                     int af, int64_t atz_us, bool ainvis, bool ainvuln, bool during_motion);
    void setFacing(unsigned short int key, MapDirection new_facing);
    void setSpeechBubble(unsigned short int key, bool show);
    void clear() { entities.clear(); } // delete all contained entities
        
    // getEntityGfx (used for drawing)
    // Coords of top-left of the map display area (square coord 0,0) must be passed in,
    // together with pixels_per_square.
    // Entity depth is a base depth at which entities are drawn (it's modified by MapHeights).
    // Results will be _added_ to "gfx_buffer" (previous contents of "gfx_buffer" will be
    // retained as well).
    void getEntityGfx(int64_t time_us, int tl_x, int tl_y, int pixels_per_square, int entity_depth,
                      std::map<int, std::vector<GraphicElement> > &gfx_buffer,
                      std::vector<TextElement> &txt_buffer, bool show_own_name,
                      const Graphic *speech_bubble, int speech_depth,
                      std::function<ClientState(const UTF8String&)> player_state_lookup);

private:
    // Commands are not executed immediately, but are stored up into a
    // "command block".  This way, if we get several commands over the
    // network in one go (due to lag or whatever), we can still spread
    // out their execution over several frames. (This can be thought
    // of as a form of interpolation.)
    struct Command {
        enum { MOVE, REPOSITION, SET_ANIM, SET_FACING } type;
        union {
            struct {
                // so: starting offset
                // nt: "natural time" (how long the move would take at normal speed).
                // ("us" = microseconds)
                int so;
                int64_t nt_us;
                MotionType type;
            } move_info;
            struct {
                int x, y;
            } reposition_info;
            struct {
                // new appearance data, for an update command.
                const Anim *anim;
                const Overlay *ovr;
                int af;
                int64_t atz_us;
                bool ainvis, ainvuln;
            } anim_info;
            MapDirection facing_info;
        };
    };

    struct Data {
        int x, y;
        MapHeight height;
        const Anim *anim;
        const Overlay *ovr;
        int af;
        int64_t atz_us;
        bool ainvis, ainvuln;
        MapDirection facing;
        bool approached;          // true if currently "approached" (and not moving)
        bool show_speech_bubble;
        UTF8String name;
        
        int64_t start_time_us;   // time at which the cmd block was started
        int64_t finish_time_us;  // time at which the cmd block should be completed
        int64_t tnt_us;          // total natural time (over all cmds)
        std::list<Command> cmds;  // the command block itself
    };

    int getFinalOffset(MotionType);
    void getCurrentOffset(int64_t time_us, const Data &ent, const Command &cmd,
                          int64_t &nt_so_far, int &cur_ofs);
    void update(int64_t time_us, Data &dat);
    void update(int64_t time_us);
    void addGraphic(const Data &ent, int64_t time_us, int tl_x, int tl_y,
                    int entity_depth,
                    std::map<int, std::vector<GraphicElement> > &gfx_buffer,
                    std::vector<TextElement> &txt_buffer,
                    int pixels_per_square,
                    bool add_entity_name,
                    const Graphic *speech_bubble,
                    int speech_depth,
                    std::function<ClientState(const UTF8String&)> player_state_lookup);

    void recomputeEntityMotion(std::map<unsigned short int,Data>::iterator it, int64_t time_us);

    const Graphic * chooseGraphic(const Anim *anim, MapDirection facing, int frame, int x, int y, int64_t time_us);

private:
    // key --> ID number for the entity;
    // data --> current client-side position, together with appearance
    // info (anim, overlay, facing etc) as well as "command block"
    // showing future changes to position and appearance that have not
    // yet been applied.
    std::map<unsigned short int, Data> entities;

    const int64_t bat_anim_timescale_us;
    const int approach_offset;

    int64_t vbat_last_time_us;
    int vbat_frames[64];
};

#endif

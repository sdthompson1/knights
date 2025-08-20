/*
 * local_dungeon_view.cpp
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
#include "draw.hpp"
#include "gfx_manager.hpp"
#include "graphic.hpp"
#include "local_dungeon_view.hpp"
#include "round.hpp"

#include <iostream>

namespace {
    // NB item_depth must be greater than the other depths here.
    const int speech_depth = -300;
    const int icon_depth = -205;
    const int entity_depth = -200;  // note this is modified (in steps of 10) for MapHeight, 
                                    // and 1 is subtracted for weapon overlays.
    const int item_depth = -100;

    const int MY_X_OFFSCREEN_VALUE = -999;
}

LocalDungeonView::LocalDungeonView(const ConfigMap &cfg, int approach_offset, const Graphic *speech_bubble_)
    : config_map(cfg),
      time_us(0),
      current_room(-1),
      entity_map(cfg, approach_offset),
      my_x(MY_X_OFFSCREEN_VALUE), my_y(0),
      last_known_time_ms(-999999),
      last_known_x(0), last_known_y(0),
      my_approached(false),
      last_mtime_ms(-999999),
      speech_bubble(speech_bubble_)
{ }


void LocalDungeonView::draw(Coercri::GfxContext &gc, GfxManager &gm, bool screen_flash,
                            int phy_dungeon_left, int phy_dungeon_top,
                            int phy_dungeon_width, int phy_dungeon_height,
                            int phy_pixels_per_square, float dungeon_scale_factor,
                            const Coercri::Font &txt_font,
                            bool show_own_name,
                            std::function<UTF8String(const PlayerID&)> player_name_lookup,
                            int &room_tl_x, int &room_tl_y)
{
    using std::list;
    using std::map;
    using std::vector;

    static const ColourChange empty_cc;

    // Clear out expired icons
    int time_ms = time_us / 1000;
    while (!icons.empty() && time_ms > icons.top().expiry_ms) {
        map<int,RoomData>::iterator ri = rooms.find(icons.top().room_no);
        if (ri != rooms.end()) {
            list<RoomData::GfxEntry> &glst(ri->second.lookupGfx(icons.top().x,
                                                                icons.top().y));
            for (list<RoomData::GfxEntry>::iterator it = glst.begin(); it != glst.end();
            ++it) {
                if (it->depth == icon_depth) {
                    glst.erase(it);
                    break;
                }
            }
        }
        icons.pop();
    }

    // Clear out expired messages
    // Note - this is not perfect as queue is ordered by start_time, rather than stop_time.
    // But this does not matter much in practice.
    while (!messages.empty() && time_ms >= messages.front().stop_time_ms) {
        messages.pop_front();
    }

    // Draw current room
    if (my_x != MY_X_OFFSCREEN_VALUE) last_known_time_ms = time_ms;

    if (aliveRecently()) {
        // We are alive, or have been dead for no longer than
        // "death_draw_map_time". Draw the map.
        std::map<int, RoomData>::iterator ri = rooms.find(current_room);
        if (ri != rooms.end()) {
            RoomData &rd(ri->second);

            // Clear out the graphics buffer
            for (std::map<int, std::vector<GraphicElement> >::iterator it = gfx_buffer.begin();
            it != gfx_buffer.end(); ++it) {
                it->second.clear();
            }
            txt_buffer.clear();
            
            // Find on-screen top-left corner for the room
            room_tl_x = phy_dungeon_left + (phy_dungeon_width - rd.width*phy_pixels_per_square) / 2;
            room_tl_y = phy_dungeon_top + (phy_dungeon_height - rd.height*phy_pixels_per_square) / 2;

            // Build a list of graphics to be drawn

            // Draw tiles (if screen not flashing)
            if (!screen_flash) {
                for (int i=0; i<rd.width; ++i) {
                    for (int j=0; j<rd.height; ++j) {
                        const int screen_x = i*phy_pixels_per_square + room_tl_x;
                        const int screen_y = j*phy_pixels_per_square + room_tl_y;
                        
                        const list<RoomData::GfxEntry> &glst(rd.lookupGfx(i,j));
                        for (list<RoomData::GfxEntry>::const_iterator it = glst.begin();
                        it != glst.end(); ++it) {
                            GraphicElement ge;
                            ge.sx = screen_x;
                            ge.sy = screen_y;
                            ge.gr = it->gfx;
                            ge.cc = it->cc.get();
                            ge.semitransparent = false;
                            gfx_buffer[it->depth].push_back(ge);
                        }
                    }
                }
            }
            
            // Add the entities (always)
            entity_map.getEntityGfx(time_us, room_tl_x, room_tl_y, phy_pixels_per_square, entity_depth, gfx_buffer, txt_buffer, show_own_name,
                                    speech_bubble, speech_depth, player_name_lookup);

            // Now draw all buffered graphics
            for (map<int, vector<GraphicElement> >::reverse_iterator it = gfx_buffer.rbegin();
            it != gfx_buffer.rend(); ++it) {
                for (vector<GraphicElement>::const_iterator it2 = it->second.begin();
                it2 != it->second.end(); ++it2) {

                    int old_width, old_height;
                    gm.getGraphicSize(*it2->gr, old_width, old_height);
                    const float size_hint = it2->gr->getSizeHint();
                    const int new_width = Round(old_width * dungeon_scale_factor / size_hint);
                    const int new_height = Round(old_height * dungeon_scale_factor / size_hint);

                    if (it2->cc) {
                        gm.drawTransformedGraphic(gc, it2->sx, it2->sy, *it2->gr, new_width, new_height, *it2->cc, it2->semitransparent);
                    } else if (it2->semitransparent) {
                        gm.drawTransformedGraphic(gc, it2->sx, it2->sy, *it2->gr, new_width, new_height, empty_cc, true);
                    } else {
                        gm.drawTransformedGraphic(gc, it2->sx, it2->sy, *it2->gr, new_width, new_height);
                    }
                }
            }

            // Draw texts (this does the names above the knights' heads)
            const int txt_yofs = - phy_pixels_per_square/2 - txt_font.getTextHeight();
            for (vector<TextElement>::const_iterator it = txt_buffer.begin(); it != txt_buffer.end(); ++it) {
                const int x = it->sx - txt_font.getTextWidth(it->text)/2;
                const int y = it->sy + txt_yofs;
                gc.drawText(x+1, y+1, txt_font, it->text, Coercri::Color(0,0,0));
                gc.drawText(x, y, txt_font, it->text, Coercri::Color(255,255,255));
            }
        }
    } else {
        // Dead - clear out the msg queue as it looks stupid when msgs from your previous life come up!
        messages.clear();
        cts_messages.clear();
    }

    // The rest should not be drawn when screen is flashing
    if (!screen_flash) {

        // Determine whether we should draw a message
        if (messages.empty() && cts_messages.empty()) {
            last_mtime_ms = 0;
        } else {
            if (last_mtime_ms == 0) {
                if (!messages.empty()) last_mtime_ms = messages.front().start_time_ms;
                else last_mtime_ms = time_ms;
            }
            int mtime_ms = time_ms - last_mtime_ms;
                
            const int msg_on_time_ms = config_map.getInt("message_on_time");
            const int msg_off_time_ms = config_map.getInt("message_off_time");
            const int nmsgs = messages.size() + cts_messages.size();
            int mphase = (mtime_ms / (msg_on_time_ms + msg_off_time_ms)) % nmsgs;
            const UTF8String &the_msg(mphase < messages.size() ? messages[mphase].message
                                       : cts_messages[mphase - messages.size()]);
            if (mtime_ms % (msg_on_time_ms + msg_off_time_ms) <= msg_on_time_ms) {
                std::map<int,RoomData>::iterator ri = rooms.find(current_room);
                if (ri != rooms.end()) {
                    DrawUI::drawMessage(config_map, gc,
                                        phy_dungeon_left, phy_dungeon_top, phy_dungeon_width, phy_dungeon_height,
                                        gm,
                                        last_known_x, last_known_y, ri->second.width,
                                        ri->second.height, phy_pixels_per_square, the_msg);
                }
            }
        }
    }
}

bool LocalDungeonView::aliveRecently() const
{
    int time_ms = time_us / 1000;
    return time_ms < last_known_time_ms + config_map.getInt("death_draw_map_time");
}

void LocalDungeonView::setCurrentRoom(int r, int w, int h)
{
    typedef std::map<int, RoomData>::iterator Iter;

    // Clear the entity_map when moving to a new room
    entity_map.clear();

    // Create the new map (if necessary) and reset current_room & current_dmap
    Iter it = rooms.find(r);
    if (it == rooms.end()) {
        rooms.insert(std::make_pair(r, RoomData(w, h)));
    }
    current_room = r;
}

void LocalDungeonView::addEntity(unsigned short int id, int x, int y, MapHeight ht, MapDirection facing,
                                 const Anim * anim, const Overlay *ovr, int af, int atz_diff_ms,
                                 bool ainvis, bool ainvuln,
                                 bool approached,
                                 int cur_ofs, MotionType motion_type, int motion_time_remaining_ms,
                                 const PlayerID &player_id)
{
    int64_t atz_diff_us = int64_t(atz_diff_ms) * 1000;
    int64_t motion_time_remaining_us = motion_time_remaining_ms * 1000;
    entity_map.addEntity(time_us, id, x, y, ht, facing, anim, ovr, af,
                         atz_diff_us ? atz_diff_us + time_us : 0,
                         ainvis, ainvuln,
                         cur_ofs, motion_type, motion_time_remaining_us, player_id);
    if (id == 0) {
        // my knight
        my_x = x;
        my_y = y;
        if (my_x >= 0) {
            last_known_x = my_x;
            last_known_y = my_y;
        }
        my_approached = approached;
        if (!my_colour_change && anim) {
            my_colour_change.reset(new ColourChange(anim->getColourChange(false)));
        }
    }
}

void LocalDungeonView::rmEntity(unsigned short int id)
{
    entity_map.rmEntity(id);
    if (id==0) {
        my_x = MY_X_OFFSCREEN_VALUE;
    }
}

void LocalDungeonView::repositionEntity(unsigned short int id, int new_x, int new_y)
{
    entity_map.repositionEntity(id, new_x, new_y);
    if (id == 0) {
        my_x = new_x;
        my_y = new_y;
        if (my_x >= 0) {
            last_known_x = my_x;
            last_known_y = my_y;
        }
        my_approached = false;
    }
}

void LocalDungeonView::moveEntity(unsigned short int id, MotionType mt, int motion_dur_ms, bool missile_mode)
{
    int64_t motion_dur_us = int64_t(motion_dur_ms) * 1000;
    entity_map.moveEntity(time_us, id, mt, motion_dur_us, missile_mode);
    if (id==0) {
        my_approached = (mt == MT_APPROACH);
    }
}

void LocalDungeonView::flipEntityMotion(unsigned short int id, int initial_delay_ms, int motion_dur_ms)
{
    int64_t initial_delay_us = int64_t(initial_delay_ms) * 1000;
    int64_t motion_dur_us = int64_t(motion_dur_ms) * 1000;
    entity_map.flipEntityMotion(time_us, id, initial_delay_us, motion_dur_us);
}

void LocalDungeonView::setAnimData(unsigned short int id, const Anim *a, const Overlay *o,
                                   int af, int atz_diff_ms, bool ainvis, bool ainvuln, bool currently_moving)
{
    int64_t atz_diff_us = int64_t(atz_diff_ms) * 1000;
    entity_map.setAnimData(id, a, o, af,
                           atz_diff_us ? atz_diff_us + time_us : 0,
                           ainvis, ainvuln, currently_moving);
}

void LocalDungeonView::setFacing(unsigned short int id, MapDirection new_facing)
{
    entity_map.setFacing(id, new_facing);
}

void LocalDungeonView::setSpeechBubble(unsigned short int id, bool show)
{
    entity_map.setSpeechBubble(id, show);
}

void LocalDungeonView::clearTiles(int x, int y, bool)
{
    std::map<int,RoomData>::iterator ri = rooms.find(current_room);
    if (ri == rooms.end()) return;
    if (!ri->second.valid(x,y)) return;
    std::list<RoomData::GfxEntry> &gfx(ri->second.lookupGfx(x, y));
    // We know that the tiles come before items or other stuff like that.
    while (!gfx.empty() && gfx.front().depth != item_depth) gfx.pop_front();
}

void LocalDungeonView::setTile(int x, int y, int depth, const Graphic * gfx,
                               boost::shared_ptr<const ColourChange> cc, bool)
{
    std::map<int,RoomData>::iterator ri = rooms.find(current_room);
    if (ri == rooms.end()) return;
    if (!ri->second.valid(x,y)) return;
    std::list<RoomData::GfxEntry> &glst(ri->second.lookupGfx(x, y));
    std::list<RoomData::GfxEntry>::iterator it = glst.begin();
    while (1) {
        // The list is in sorted order, highest (ie deepest) depth first.
        if (it == glst.end() || depth > it->depth) {
            if (gfx) {
                RoomData::GfxEntry ge;
                ge.gfx = gfx;
                ge.cc = cc;
                ge.depth = depth;
                glst.insert(it, ge);
            }
            break;
        } else if (it->depth == depth) {
            if (gfx) {
                it->gfx = gfx;
                it->cc = cc;
            } else {
                glst.erase(it);
            }
            break;
        } else {
            ++it;
        }
    }
}

void LocalDungeonView::setItem(int x, int y, const Graphic * graphic, bool)
{
    // We can implement this using setTile.
    setTile(x, y, item_depth, graphic, boost::shared_ptr<const ColourChange>(), true);
}

void LocalDungeonView::placeIcon(int x, int y, const Graphic *g, int dur_ms)
{
    if (current_room == -1) return;

    int time_ms = time_us / 1000;

    // Add the 'icon' to the gmap. Also store in 'icons' list.
    setTile(x, y, icon_depth, g, boost::shared_ptr<const ColourChange>(), true);
    LocalIcon ic;
    ic.room_no = current_room;
    ic.x = x;
    ic.y = y;
    ic.expiry_ms = dur_ms + time_ms;
    icons.push(ic);
}

void LocalDungeonView::flashMessage(const std::string &s_latin1, int ntimes)
{
    int time_ms = time_us / 1000;

    Message m;
    m.message = UTF8String::fromLatin1(s_latin1);
    m.start_time_ms = time_ms;
    m.stop_time_ms = time_ms + (config_map.getInt("message_on_time") + config_map.getInt("message_off_time"))*ntimes;
    messages.push_back(m);
}

void LocalDungeonView::cancelContinuousMessages()
{
    cts_messages.clear();
}

void LocalDungeonView::addContinuousMessage(const std::string &s_latin1)
{
    cts_messages.push_back(UTF8String::fromLatin1(s_latin1));
}

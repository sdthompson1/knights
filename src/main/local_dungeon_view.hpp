/*
 * local_dungeon_view.hpp
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
 * Implementation of DungeonView for on-screen drawing.
 * 
 */

#ifndef LOCAL_DUNGEON_VIEW_HPP
#define LOCAL_DUNGEON_VIEW_HPP

#include "client_callbacks.hpp"
#include "dungeon_view.hpp"
#include "entity_map.hpp"
#include "utf8string.hpp"

#include "gfx/gfx_context.hpp" // coercri

#include <cstdint>
#include <functional>
#include <list>
#include <queue>
#include <vector>

class ConfigMap;
class GfxManager;

class LocalDungeonView : public DungeonView {
public:

    //
    // functions specific to this class
    //

    LocalDungeonView(const ConfigMap &config_map, int approach_offset, const Graphic *speech_bubble_);
    void addMicrosecondsToTime(int64_t delta_us) { time_us += delta_us; }
    void draw(Coercri::GfxContext &gc, GfxManager &gm, bool screen_flash,
              int phy_dungeon_left, int phy_dungeon_top,
              int phy_dungeon_width, int phy_dungeon_height,
              int phy_pixels_per_square, float dungeon_scale_factor,
              const Coercri::Font & txt_font, bool show_own_name,
              std::function<ClientState(const UTF8String&)> player_state_lookup,
              int &room_tl_x, int &room_tl_y);
    boost::shared_ptr<ColourChange> getMyColourChange() const { return my_colour_change; }
    bool isApproached() const { return my_approached; }
    bool aliveRecently() const;

    //
    // functions from DungeonView
    //
    
    void setCurrentRoom(int r, int width, int height);

    void addEntity(unsigned short int id, int x, int y, MapHeight ht, MapDirection facing,
                   const Anim * anim, const Overlay *ovr, int af, int atz_diff_ms,
                   bool ainvis, bool ainvuln, // (anim data)
                   bool approached,
                   int cur_ofs, MotionType motion_type, int motion_time_remaining,
                   const UTF8String &name);
    void rmEntity(unsigned short int id);
    void repositionEntity(unsigned short int id, int new_x, int new_y);
    void moveEntity(unsigned short int id, MotionType motion_type, int motion_duration_ms, bool missile_mode);
    void flipEntityMotion(unsigned short int id, int initial_delay_ms, int motion_duration_ms);
    void setAnimData(unsigned short int id, const Anim *, const Overlay *, int af, int atz_diff_ms, bool ainvis, bool ainvuln, bool currently_moving);
    void setFacing(unsigned short int id, MapDirection new_facing);
    void setSpeechBubble(unsigned short int id, bool show);

    void clearTiles(int x, int y, bool);
    void setTile(int x, int y, int depth, const Graphic *gfx, boost::shared_ptr<const ColourChange> cc, bool);

    void setItem(int x, int y, const Graphic *gfx, bool);

    void placeIcon(int x, int y, const Graphic *gfx, int dur_ms);

    void flashMessage(const std::string &msg_latin1, int ntimes);
    void cancelContinuousMessages();
    void addContinuousMessage(const std::string &msg);

private:
    // config map
    const ConfigMap &config_map;

    // time (updated by addMicrosecondsToTime() method)
    int64_t time_us;
    
    // icons
    struct LocalIcon {
        int room_no;
        int x, y;
        int expiry_ms;
        bool operator<(const LocalIcon &other) const { return expiry_ms < other.expiry_ms; }
    };
    std::priority_queue<LocalIcon> icons;

    // room maps
    struct RoomData {
        // Origin is set to the top-left corner if the room is mapped,
        // or (-1,-1) if it is unmapped.
        // Width, height correspond to the size of gmap.
        int origin_x, origin_y;
        int width, height;

        // Items, Tiles
        struct GfxEntry {
            const Graphic *gfx;
            boost::shared_ptr<const ColourChange> cc;
            int depth;
        };
        std::vector<std::list<GfxEntry> > gmap;
        std::list<GfxEntry> & lookupGfx(int x, int y) { return gmap[y*width + x]; }
        bool valid(int x, int y) const { return x>=0 && y>=0 && x<width && y<height; }

        // ctor
        RoomData(int w, int h)
            : origin_x(-1), origin_y(-1), width(w), height(h), gmap(w*h)
        { }
    };
    std::map<int, RoomData> rooms;
    int current_room;

    EntityMap entity_map;

    struct CompareDepth {
        bool operator()(const RoomData::GfxEntry &lhs, const RoomData::GfxEntry &rhs) const {
            return lhs.depth > rhs.depth;  // reverse order (highest depth first)
        }
    };

    // my knight
    int my_x, my_y;
    int last_known_time_ms;
    int last_known_x, last_known_y;
    boost::shared_ptr<ColourChange> my_colour_change;
    bool my_approached;

    // messages
    struct Message {
        UTF8String message;
        int start_time_ms;
        int stop_time_ms;
    };
    std::deque<Message> messages;  // sorted by start_time
    std::vector<UTF8String> cts_messages;
    int last_mtime_ms;

    // gfx buffer
    std::map<int, std::vector<GraphicElement> > gfx_buffer;
    std::vector<TextElement> txt_buffer;

    const Graphic *speech_bubble;
};

#endif

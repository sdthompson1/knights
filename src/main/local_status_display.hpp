/*
 * local_status_display.hpp
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

#ifndef LOCAL_STATUS_DISPLAY_HPP
#define LOCAL_STATUS_DISPLAY_HPP

#include "map_support.hpp"
#include "status_display.hpp"

#include "gfx/gfx_context.hpp" // coercri

#include <map>

class GfxManager;
class Graphic;
class LocalMiniMap;
class PotionRenderer;
class SkullRenderer;

class LocalStatusDisplay : public StatusDisplay {
public:

    //
    // functions specific to this class
    //

    LocalStatusDisplay(const ConfigMap &cfg,
                       const PotionRenderer *potion_renderer_,
                       const SkullRenderer *skull_renderer_,
                       const Graphic *menu_gfx_centre_,
                       const Graphic *menu_gfx_empty_,
                       const Graphic *menu_gfx_highlight_);
    void draw(Coercri::GfxContext &gc, GfxManager &gm, 
              int time, float scale,
              int x, int y,
              bool show_potion_bottle,
              const LocalMiniMap &mini_map,
              const std::string &time_limit_string);
    void getSize(float scale, int &width, int &height) const;
    void setMenuOpen(bool m) { menu_open = m; }
    bool isMenuOpen() const { return menu_open; }
    void clearMenuGraphics();
    void setMenuGraphic(MapDirection d, const Graphic *g);
    void setMenuHighlight(MapDirection d) { menu_highlight = true; menu_highlight_dir = d; }
    void clearMenuHighlight() { menu_highlight = false; }

    // get quest info
    const std::vector<std::string> & getQuestHints() const { return quest_hints; }
    bool needQuestHintUpdate() { const bool result = need_gui_update; need_gui_update = false; return result; }
    
    //
    // functions from StatusDisplay
    //

    void setBackpack(int slot, const Graphic *gfx, const Graphic *overdraw,
                     int no_carried, int no_max);
    void addSkull();
    void setHealth(int h);
    void setPotionMagic(PotionMagic pm, bool poison_immunity);
    void setQuestHints(const std::vector<std::string> &);
    
private:
    const ConfigMap &config_map;

    // cached config variables
    const int ref_inventory_left, ref_inventory_top, ref_inventory_width, ref_inventory_height;
    const int num_inventory_slots, ref_inventory_spacing, ref_inventory_gem_height;
    const int ref_mini_map_left, ref_mini_map_top, ref_mini_map_width, ref_mini_map_height;
    const int ref_skulls_left, ref_skulls_top;
    const int ref_potion_left, ref_potion_top, ref_potion_top_with_time;
    const int ref_time_x, ref_time_y;

    // skulls and potion bottle
    const SkullRenderer *skull_renderer;
    const PotionRenderer *potion_renderer;
    int nskulls;
    int health;
    PotionMagic magic;
    bool poison_immun;

    // backpack
    struct BackpackEntry {
        const Graphic *gfx;
        const Graphic *overdraw;
        int no_carried;
        int no_max;
    };
    std::map<int, BackpackEntry> backpack;

    // menu
    const Graphic * menu_gfx_centre;    // 'arrows' graphic
    const Graphic * menu_gfx_empty;     // 'empty square' graphic
    const Graphic * menu_gfx_highlight; // 'highlight' graphic
    const Graphic * menu_gfx[4];        // graphics for the 4 directions
    bool menu_open;
    bool menu_highlight;
    MapDirection menu_highlight_dir;

    // quest requirements from server
    std::vector<std::string> quest_hints;
    bool need_gui_update;
};

#endif

    

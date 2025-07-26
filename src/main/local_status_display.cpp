/*
 * local_status_display.cpp
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

#include "config_map.hpp"
#include "draw.hpp"
#include "gfx_manager.hpp"
#include "local_mini_map.hpp"
#include "local_status_display.hpp"
#include "potion_renderer.hpp"
#include "round.hpp"
#include "skull_renderer.hpp"

LocalStatusDisplay::LocalStatusDisplay(const ConfigMap &cfg,
                                       const PotionRenderer *potion_r,
                                       const SkullRenderer *skull_r,
                                       const Graphic *menu_gfx_centre_,
                                       const Graphic *menu_gfx_empty_,
                                       const Graphic *menu_gfx_highlight_)                                       
    : config_map(cfg),

      ref_inventory_left(cfg.getInt("dpy_inventory_left")),
      ref_inventory_top(cfg.getInt("dpy_inventory_top")),
      ref_inventory_width(cfg.getInt("dpy_inventory_width")),
      ref_inventory_height(cfg.getInt("dpy_inventory_height")),
      
      num_inventory_slots(cfg.getInt("dpy_inventory_slots")),
      ref_inventory_spacing(cfg.getInt("dpy_inventory_spacing")),
      ref_inventory_gem_height(cfg.getInt("dpy_inventory_gem_height")),

      ref_mini_map_left(cfg.getInt("dpy_map_left")),
      ref_mini_map_top(cfg.getInt("dpy_map_top")),
      ref_mini_map_width(cfg.getInt("dpy_map_width")),
      ref_mini_map_height(cfg.getInt("dpy_map_height")),

      ref_skulls_left(cfg.getInt("dpy_skulls_left")),
      ref_skulls_top(cfg.getInt("dpy_skulls_top")),

      ref_potion_left(cfg.getInt("dpy_potion_left")),
      ref_potion_top(cfg.getInt("dpy_potion_top")),
      ref_potion_top_with_time(cfg.getInt("dpy_potion_top_with_time")),
      ref_time_x(cfg.getInt("dpy_time_x")),
      ref_time_y(cfg.getInt("dpy_time_y")),

      skull_renderer(skull_r),
      potion_renderer(potion_r),
      nskulls(0),
      health(0),
      magic(NO_POTION),
      poison_immun(false),
      menu_gfx_centre(menu_gfx_centre_),
      menu_gfx_empty(menu_gfx_empty_),
      menu_gfx_highlight(menu_gfx_highlight_),
      menu_open(false),
      menu_highlight(false),
      menu_highlight_dir(D_NORTH),
      need_gui_update(false)
{
    for (int i = 0; i < 4; ++i) { menu_gfx[i] = menu_gfx_empty_; }
}

void LocalStatusDisplay::draw(Coercri::GfxContext &gc, GfxManager &gm,
                              int time, float scale,
                              int x, int y,
                              bool show_potion_bottle,
                              LocalMiniMap &mini_map,
                              const std::string &time_limit_string_latin1)
{
    // Work out where to draw the inventory
    const int inv_slot_width = int(ref_inventory_width / num_inventory_slots * scale);
    const int inv_width = inv_slot_width * num_inventory_slots;
    const int inv_height = int(ref_inventory_height * scale);
    const int inv_x = x + Round(ref_inventory_left * scale) + (inv_width - Round(ref_inventory_width * scale)) / 2;
    const int inv_y = y + Round(ref_inventory_top * scale);
    
    // Draw backpack icons. (This is handled by DrawUI)
    for (std::map<int, BackpackEntry>::iterator it = backpack.begin(); it != backpack.end(); ++it) {
        DrawUI::drawBackpackEntry(gc,
                                  inv_x, inv_y, inv_width, inv_height,
                                  gm, num_inventory_slots, ref_inventory_spacing, ref_inventory_gem_height,
                                  it->first, it->second.gfx, it->second.overdraw, it->second.no_carried,
                                  it->second.no_max);
    }

    if (menu_open) {
        // Get "raw" size of the menu graphics.
        int mg_width=0, mg_height=0;
        if (menu_gfx_centre) gm.getGraphicSize(*menu_gfx_centre, mg_width, mg_height);

        // We centre the menu within the "ref_mini_map" area.
        int menu_x = Round(ref_mini_map_left * scale) + x;
        int menu_y = Round(ref_mini_map_top * scale) + y;
        const float block_size_f = mg_width * scale;  // we assume mg_width == mg_height
        const int block_size = int(block_size_f);  // round down
        const float leftover = block_size_f - block_size;
        menu_x += int(leftover*3) / 2;
        menu_y += int(leftover*3) / 2;

        // Draw the menu
        DrawUI::drawMenu(config_map, gc,
                         menu_x, menu_y, block_size*3, block_size*3,
                         gm,
                         menu_gfx_centre, menu_gfx_empty, menu_gfx_highlight, time,
                         menu_gfx[D_NORTH], menu_gfx[D_EAST], menu_gfx[D_SOUTH], menu_gfx[D_WEST],
                         menu_highlight, menu_highlight_dir);
    } else {
        // Resize the mini map. We assume that each square on the minimap takes up one "reference pixel".
        // This is then re-centred within the "ref_mini_map" rectangle if necessary.
        int map_x = Round(ref_mini_map_left * scale) + x;
        int map_y = Round(ref_mini_map_top * scale) + y;
        const int total_map_width = Round(ref_mini_map_width * scale);
        const int total_map_height = Round(ref_mini_map_height * scale);
        const int map_width = Round(mini_map.getWidth() * scale);
        const int map_height = Round(mini_map.getHeight() * scale);
        map_x += (total_map_width - map_width) / 2;
        map_y += (total_map_height - map_height) / 2;

        // Draw the map
        mini_map.draw(gc, map_x, map_y, map_width, map_height, time);
    }

    // Draw Skulls
    if (skull_renderer) {
        const int phy_skulls_left = Round(float(ref_skulls_left) * scale) + x;
        const int phy_skulls_top = Round(float(ref_skulls_top) * scale) + y;
        skull_renderer->draw(gc, gm, nskulls, phy_skulls_left, phy_skulls_top, scale);
    }

    // Draw Potion Bottle
    if (potion_renderer && show_potion_bottle) {
        const bool show_time = !time_limit_string_latin1.empty();
        const int ref_top = show_time ? ref_potion_top_with_time : ref_potion_top;
        const int phy_potion_left = Round(float(ref_potion_left) * scale) + x;
        const int phy_potion_top = Round(float(ref_top) * scale) + y;
        potion_renderer->draw(time, gc, gm, health, magic, poison_immun,
                              phy_potion_left, phy_potion_top, scale);
        if (show_time) {
            const int phy_x = Round(float(ref_time_x) * scale) + x 
                    - gm.getFont()->getTextWidth(UTF8String::fromLatin1(time_limit_string_latin1))/2;
            const int phy_y = Round(float(ref_time_y) * scale) + y;
            gc.drawText(phy_x, phy_y, *gm.getFont(), UTF8String::fromLatin1(time_limit_string_latin1),
                    Coercri::Color(255,255,255));
        }
    }
}

void LocalStatusDisplay::getSize(float scale, int &width, int &height) const
{
    width = Round((ref_mini_map_left + ref_mini_map_width) * scale);
    height = Round((ref_inventory_top + ref_inventory_height) * scale);
}


void LocalStatusDisplay::clearMenuGraphics()
{
    for (int d = 0; d < 4; ++d) {
        menu_gfx[d] = 0;
    }
}

void LocalStatusDisplay::setMenuGraphic(MapDirection d, const Graphic *g)
{
    if (g) {
        menu_gfx[d] = g;
    } else {
        menu_gfx[d] = menu_gfx_empty;
    }
}

void LocalStatusDisplay::setBackpack(int slot, const Graphic *gfx, const Graphic *overdraw,
                                     int no_carried, int no_max)
{
    BackpackEntry be;
    be.gfx = gfx;
    be.overdraw = overdraw;
    be.no_carried = no_carried;
    be.no_max = no_max;
    backpack.erase(slot);
    backpack.insert(std::make_pair(slot, be));
}

void LocalStatusDisplay::addSkull()
{
    ++nskulls;
}

void LocalStatusDisplay::setHealth(int h)
{
    health = h;
}

void LocalStatusDisplay::setPotionMagic(PotionMagic pm, bool pi)
{
    magic = pm;
    poison_immun = pi;
}

void LocalStatusDisplay::setQuestHints(const std::vector<std::string> &rqmts)
{
    quest_hints = rqmts;
    need_gui_update = true;
}

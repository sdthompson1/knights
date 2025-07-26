/*
 * draw.cpp
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
 * The various in-game drawing routines
 *
 */

#include "misc.hpp"

#include "colour_change.hpp"
#include "config_map.hpp"
#include "draw.hpp"
#include "gfx_manager.hpp"
#include "round.hpp"

// coercri
#include "gfx/rectangle.hpp"

namespace {
    const int NCOLS = 4;
    unsigned char COLS[NCOLS][3] = {
        { 255,0,0 },
        { 255,255,255 },
        { 255,0,0 },
        { 0,0,0 }
    };
}

//
// drawBackpackEntry
//

void DrawUI::drawBackpackEntry(Coercri::GfxContext &gc,
                               int left, int top, int width, int height,
                               GfxManager &gm, int num_slots, int spacing, int gem_height,
                               int slot, const Graphic *gfx, const Graphic *overdraw_,
                               int no_carried, int no_max)
{
    if (!gfx) return;
    if (no_carried == 0) return;
    if (spacing < 1) spacing = 1;

    const Graphic *overdraw = overdraw_ ? overdraw_ : gfx;
    
    // assumption: all item gfx, including gems, are the same width (although
    // not necessarily the same height).
    int gfx_width, gfx_height, overdraw_width, overdraw_height;
    gm.getGraphicSize(*gfx, gfx_width, gfx_height);
    gm.getGraphicSize(*overdraw, overdraw_width, overdraw_height);
    
    const int width_of_each = width / num_slots;
    const float scale_factor = float(width_of_each) / float(gfx_width);
    const int rescaled_height = Round(float(gfx_height) * scale_factor);
    const int rescaled_overdraw_height = Round(float(overdraw_height) * scale_factor);
    const int rescaled_spacing = std::max(1, Round(float(spacing) * scale_factor));
    const int rescaled_gem_height = Round(float(gem_height) * scale_factor);
    
    // Slots: 11, 12, ... 19 are Normal Items
    // 20 = Lock Picks
    // 21, 22, 23 = Keys.
    // 30 = Gems

    if (slot > 10 && slot < 20) {
        // Normal items are drawn in a column format, starting from the 2nd slot
        // (the 1st being reserved for keys).
        // Vertical spacing is chosen such that no_max items fit exactly into the height
        // of the inventory area. (If no_max==0, then space them by the supplied spacing value.)

        const int x = left + width_of_each*(slot-10);

        if (no_max != 0) {
            const int num = height - rescaled_gem_height - rescaled_height;
            if (no_carried > no_max) no_max = no_carried;
            for (int j = 0; j < no_carried; ++j) {
                gm.drawTransformedGraphic(gc, x, top + num*j/no_max, *gfx, width_of_each, rescaled_height);
            }
        } else {
            // use supplied spacing; extra ones are just dropped.
            const int jmax = (height - rescaled_gem_height - rescaled_height) / rescaled_spacing;
            for (int j = std::min(no_carried-1, jmax); j >= 0; --j) {
                gm.drawTransformedGraphic(gc, x, top + j*rescaled_spacing, j ? *overdraw : *gfx, width_of_each,
                                          j ? rescaled_overdraw_height : rescaled_height);
            }
        }
        
    } else if (slot >= 20 && slot <= 29) {
        // Keys are drawn downwards from the top, in the leftmost slot.
        // NB no_carried and no_max are ignored for keys and lockpicks.
        const int x = left;
        const int y = top + rescaled_height*(slot-20);
        gm.drawTransformedGraphic(gc, x, y, *gfx, width_of_each, rescaled_height);

    } else if (slot == 30) {
        // Gems are drawn UNDERNEATH the normal inventory slots, in a right to left order.
        int x = left + width_of_each*(num_slots-1);
        const int y = top + height - rescaled_gem_height;
        for (int i = 0; i < no_carried; ++i) {
            gm.drawTransformedGraphic(gc, x, y, *gfx, width_of_each, rescaled_height);
            x -= width_of_each;
        }
    }
}


//
// drawMessage
//

void DrawUI::drawMessage(const ConfigMap &config_map, Coercri::GfxContext &gc,
                         int d_left, int d_top, int d_width, int d_height,
                         GfxManager &gm, int sq_rel_x, int sq_rel_y,
                         int w, int h, int pixels_per_square, const UTF8String &message)
{
    const int room_bl_x = d_left + (d_width - w*pixels_per_square)/2;
    const int room_bl_y = d_top + (d_height - h*pixels_per_square)/2;
    const int sq_bl_x = sq_rel_x*pixels_per_square + room_bl_x;
    const int sq_bl_y = sq_rel_y*pixels_per_square + room_bl_y;
    boost::shared_ptr<Coercri::Font> font = gm.getFont();
    int text_width, text_height;
    font->getTextSize(message, text_width, text_height);
    const int tx = sq_bl_x + (pixels_per_square/2) - (text_width/2);
    const int ty = sq_bl_y + pixels_per_square*11/10;

    const int text_margin_x = text_height / 2;
    const int text_margin_y = text_height / 10;

    gc.fillRectangle(Coercri::Rectangle(tx - text_margin_x,
                                        ty - text_margin_y,
                                        text_width + text_margin_x*2,
                                        text_height + text_margin_y*2),
                     Coercri::Color(0, 0, 0, config_map.getInt("msg_alpha")));
    gc.drawText(tx, ty, *font, message, Coercri::Color(255,255,255));
}


//
// drawMenu
// NOTE: we assume that graphics are square and all the same size.
//

void DrawUI::drawMenu(const ConfigMap &config_map, Coercri::GfxContext &gc,
                      int left, int top, int width, int height,
                      GfxManager &gm, 
                      const Graphic * gfx_centre, const Graphic * gfx_empty,
                      const Graphic * gfx_highlight, int time,
                      const Graphic * gfx_north, const Graphic * gfx_east,
                      const Graphic * gfx_south, const Graphic * gfx_west,
                      bool highlight, MapDirection highlight_dir)
{
    const int size = width / 3;
    if (gfx_centre) {
        const int w = size;
        const int h = size;
        gm.drawTransformedGraphic(gc, left + w, top + h, *gfx_centre, w, h);
        drawMenuItem(gc, gm, gfx_north, gfx_empty, left + w,   top,       w, h);
        drawMenuItem(gc, gm, gfx_west,  gfx_empty, left,       top + h,   w, h);
        drawMenuItem(gc, gm, gfx_east,  gfx_empty, left + 2*w, top + h,   w, h);
        drawMenuItem(gc, gm, gfx_south, gfx_empty, left + w,   top + 2*h, w, h);
        if (gfx_highlight) {
            int xofs = 0, yofs = 0;
            if (highlight) {
                switch (highlight_dir) {
                case D_NORTH:
                    yofs = -h;
                    break;
                case D_EAST:
                    xofs = w;
                    break;
                case D_SOUTH:
                    yofs = h;
                    break;
                case D_WEST:
                    xofs = -w;
                    break;
                }
            }
            ColourChange cc;
            const int n = (time / config_map.getInt("mini_map_flash_time")) % NCOLS;
            cc.add(Colour(255, 0, 0), Colour(COLS[n][0], COLS[n][1], COLS[n][2]));
            gm.drawTransformedGraphic(gc, left + xofs + w, top + yofs + h, *gfx_highlight, w, h, cc);
        }
    }
}

void DrawUI::drawMenuItem(Coercri::GfxContext &gc, GfxManager &gm,
                          const Graphic *g, const Graphic *e,
                          int x, int y, int w, int h)
{
    if (g) gm.drawTransformedGraphic(gc, x, y, *g, w, h);
    else if (e) gm.drawTransformedGraphic(gc, x, y, *e, w, h);
}




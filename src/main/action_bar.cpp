/*
 * action_bar.cpp
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

#include "misc.hpp"

#include "action_bar.hpp"
#include "config_map.hpp"
#include "gfx_manager.hpp"
#include "round.hpp"
#include "user_control.hpp"

ActionBar::ActionBar(const ConfigMap &cfg,
                     const Graphic *gfx_empty_slot_,
                     const Graphic *gfx_highlight_)
    : ref_slot_size(cfg.getInt("dpy_action_slot_size")),
      gfx_empty_slot(gfx_empty_slot_),
      gfx_highlight(gfx_highlight_),
      slot_gfx(NUM_ACTION_BAR_SLOTS),
      base_x(0), base_y(0), icon_size(0)
{
}

void ActionBar::draw(Coercri::GfxContext &gc, GfxManager &gm,
                     int time, float scale,
                     int x, int y,
                     int highlight_slot, bool strong_highlight)
{
    base_x = x;
    base_y = y;
    icon_size = int(ref_slot_size * scale);

    // draw each icon
    for (int i = 0; i < NUM_ACTION_BAR_SLOTS; ++i) {
        const Graphic *g = slot_gfx[i];
        if (g) {

            // slot 0 (suicide) should be on extreme right.
            const int gx = base_x + icon_size * ((i==0 ? NUM_ACTION_BAR_SLOTS : i) - 1);
            const int gy = base_y;
            
            gm.drawTransformedGraphic(gc, gx, gy, *g, icon_size, icon_size);
            
            if (highlight_slot == i && gfx_highlight) {
                
                if (strong_highlight) {
                    ColourChange cc;
                    cc.add(Colour(255,0,0), Colour(255,255,255));
                    gm.drawTransformedGraphic(gc, gx, gy, *gfx_highlight, icon_size, icon_size, cc);
                } else {
                    gm.drawTransformedGraphic(gc, gx, gy, *gfx_highlight, icon_size, icon_size);
                }
            }
        }
    }
}

void ActionBar::setGraphics(const Graphic ** gfx)
{
    for (int i = 0; i < NUM_ACTION_BAR_SLOTS; ++i) {
        if (gfx[i]) {
            slot_gfx[i] = gfx[i];
        } else {
            slot_gfx[i] = gfx_empty_slot;
        }
    }
}

int ActionBar::mouseHit(int mx, int my) const
{
    if (my >= base_y && my < base_y + icon_size) {
        const int dx = (mx - base_x);
        if (dx < 0) return NO_SLOT;
        int slot = dx / icon_size + 1;
        if (slot == NUM_ACTION_BAR_SLOTS) slot = 0;  // suicide
        if (slot < NUM_ACTION_BAR_SLOTS) return slot;
    }
    return NO_SLOT;
}

/*
 * potion_renderer.cpp
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

#include "config_map.hpp"
#include "gfx_manager.hpp"
#include "potion_renderer.hpp"
#include "round.hpp"

PotionRenderer::PotionRenderer(const ConfigMap &cfg)
    : config_map(cfg), def_set(false), def_col(0,0,0,0)
{ }

void PotionRenderer::addGraphic(const Graphic *g)
{
    gfx.push_back(g);
}

void PotionRenderer::addColour(Colour col)
{
    if (!def_set) {
        // First colour gets set as "default".
        def_set = true;
        def_col = col;
    } else {
        // Second and subsequent colours get added as ColourChanges.
        ColourChange cc;
        cc.add(def_col, col);
        colours.push_back(cc);
    }

}

void PotionRenderer::draw(int time, Coercri::GfxContext &gc, GfxManager &gm, int health, PotionMagic magic,
                          bool poison_immunity, int left, int top, float scale) const
{
    // Find out which graphic to draw
    if (health < 0) health = 0;
    if (health >= gfx.size()) health = gfx.size() - 1;
    const Graphic *gr = gfx[health];

    int width, height;
    gm.getGraphicSize(*gr, width, height);
    width = Round(float(width) * scale);
    height = Round(float(height) * scale);
    
    // Check if poison immunity colour change should be used.
    if (poison_immunity) {
        const int s = (time / config_map.getInt("pot_flash_time")) % config_map.getInt("pi_flash_delay");
        if (s == 0) {
            // poison immunity: Use a white colour change
            ColourChange cc;
            cc.add(def_col, Colour(255, 255, 255));
            gm.drawTransformedGraphic(gc, left, top, *gr, width, height, cc);
            return;
        }
    }

    // Otherwise, work out colour to use based on "magic" value.
    if (magic < NO_POTION || magic > SUPER) return;  // magic out of range
    
    if (magic == NO_POTION) {
        // no potion: draw potion w/o colour change
        gm.drawTransformedGraphic(gc, left, top, *gr, width, height);
    } else if (magic == SUPER) {
        // super: use a random colour change
        // Use time as a random seed (we should not disturb g_rng during rendering).
        int xx = (time/config_map.getInt("pot_flash_time"))*1103515245;
        int r = (xx >> 16) & 0xff;
        int g = (xx >> 8) & 0xff;
        int b = (xx & 0xff);
        ColourChange cc;
        cc.add(def_col, Colour(r, g, b));
        gm.drawTransformedGraphic(gc, left, top, *gr, width, height, cc);
    } else {
        // draw with colour change appropriate to current magic
        if (magic-1 < 0 || magic-1 >= colours.size()) return;
        gm.drawTransformedGraphic(gc, left, top, *gr, width, height, colours[int(magic-1)]);
    }
}

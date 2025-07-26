/*
 * action_bar.hpp
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

#ifndef ACTION_BAR_HPP
#define ACTION_BAR_HPP

#include "gfx/gfx_context.hpp" // coercri

#include <vector>

class ConfigMap;
class GfxManager;
class Graphic;
class UserControl;

// special slot numbers
enum {
    NO_SLOT = -1,
    NUM_ACTION_BAR_SLOTS = 10,  // Numbered 0 to NUM_ACTION_BAR_SLOTS-1
    TAP_SLOT = NUM_ACTION_BAR_SLOTS,
    TOTAL_NUM_SLOTS = NUM_ACTION_BAR_SLOTS + 1
};
    

class ActionBar {
public:
    ActionBar(const ConfigMap &cfg,
              const Graphic *gfx_empty_slot_,
              const Graphic *gfx_highlight_);

    void draw(Coercri::GfxContext &gc, GfxManager &gm,
              int time, float scale,
              int x, int y,
              int highlight_slot, bool strong_highlight);

    void setGraphics(const Graphic **gfx);
    int mouseHit(int mx, int my) const;    // returns a slot number, or NO_SLOT
    
private:
    int ref_slot_size;
    const Graphic *gfx_empty_slot;
    const Graphic *gfx_highlight;

    // set by setAvailableControls()
    std::vector<const Graphic *> slot_gfx;

    // set by draw() and/or getSize()
    int base_x, base_y, icon_size;
};

#endif

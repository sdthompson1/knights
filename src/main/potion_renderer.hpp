/*
 * potion_renderer.hpp
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

/*
 * This draws the potion bottle, showing Knight's health and current magic.
 * 
 */

#ifndef POTION_RENDERER_HPP
#define POTION_RENDERER_HPP

#include "colour_change.hpp"
#include "potion_magic.hpp"

#include "gfx/gfx_context.hpp"

#include <vector>
using namespace std;

class ConfigMap;
class GfxManager;
class Graphic;

class PotionRenderer {
public:
    //
    // setup
    //
    explicit PotionRenderer(const ConfigMap &cfg);
    
    // Graphics, to be added in order of increasing health
    // (first for 0 health, second for 1 hitpoint, 3rd for 2 hitpoints etc)
    void addGraphic(const Graphic *);

    // Colours, to be added in order of PotionMagics.
    void addColour(Colour col);

    //
    // drawing
    // (clip rectangle should be unset, will be taken care of by this func)
    //
    void draw(int time, Coercri::GfxContext &, GfxManager &, int health, PotionMagic magic, bool poison_immunity,
              int left, int top, float scale) const;
    
private:
    const ConfigMap &config_map;
    bool def_set;
    Colour def_col;
    vector<const Graphic *> gfx;
    vector<ColourChange> colours;
};

#endif

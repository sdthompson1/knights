/*
 * draw.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2012.
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
 * Contains code for drawing various bits of the in-game displays.
 *
 */

#ifndef DRAW_HPP
#define DRAW_HPP

#include "graphic_element.hpp"
#include "map_support.hpp"

// coercri
#include "gfx/font.hpp"
#include "gfx/gfx_context.hpp"
#include "gfx/graphic.hpp"

#include "boost/shared_ptr.hpp"

#include <map>
#include <vector>
using namespace std;

class ColourChange;
class ConfigMap;
class Creature;
class DungeonMap;
class Entity;
class GfxManager;
class MapCoord;


class DrawUI {
public:
    // Draw backpack entries (items).
    // This is called once per backpack 'slot' (i.e. item type).
    static void drawBackpackEntry(Coercri::GfxContext &gc, int x, int y, int width, int height,
                                  GfxManager &gm, int num_slots, int spacing, int gem_height,
                                  int slot, const Graphic *gfx, const Graphic *overdraw,
                                  int no_carried, int no_max);
    
    // Draw a text message over a certain square
    // d_left, d_top, d_width, d_height give the dungeon rectangle
    // Sq_rel_x and Sq_rel_y give the square position
    // W and H give the room dimensions (in squares)
    // Message is the text to be displayed.
    static void drawMessage(const ConfigMap &config_map,
                            Coercri::GfxContext &gc, int d_left, int d_top, int d_width, int d_height,
                            GfxManager &gm, int sq_rel_x, int sq_rel_y,
                            int w, int h, int pixels_per_square, const string &message);

    
    // Draw the menu
    static void drawMenu(const ConfigMap &config_map, 
                         Coercri::GfxContext &gc, int left, int top, int width, int height,
                         GfxManager &gm, 
                         const Graphic * gfx_centre, const Graphic * gfx_empty,
                         const Graphic * gfx_highlight, int time,
                         const Graphic * gfx_north, const Graphic * gfx_east,
                         const Graphic * gfx_south, const Graphic * gfx_west,
                         bool highlight, MapDirection highlight_dir);

private:
    static void drawMenuItem(Coercri::GfxContext &, GfxManager &gm,
                             const Graphic *, const Graphic *,
                             int, int, int, int);
};

#endif

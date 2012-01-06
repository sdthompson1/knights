/*
 * local_mini_map.cpp
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

#include "misc.hpp"

#include "config_map.hpp"
#include "local_mini_map.hpp"

namespace {
    const Coercri::Color col[] = {
        Coercri::Color(255,0,0),     // highlight
        Coercri::Color(255,255,255), // highlight
        Coercri::Color(255,0,0),     // highlight
        Coercri::Color(0,0,0),       // highlight
        Coercri::Color(68,68,34),    // COL_WALL
        Coercri::Color(136,136,85),  // COL_FLOOR
        Coercri::Color(0,0,0),       // COL_UNMAPPED
    };

    const int FIRST_HIGHLIGHT = 0;
    const int NUM_HIGHLIGHTS = 4;
}

void LocalMiniMap::draw(Coercri::GfxContext &gc, int left, int top, int npx, int npy, int t) const
{
    if (width==0 || height==0) return;
    if (npx == 0 || npy == 0) return;  // will cause division by zero otherwise

    // Make a copy of the map with the correct highlights (flashing red dots) in it.
    std::vector<MiniMapColour> new_data;
    makeHighlightedMap(t, new_data);

    const int scale = min(npx/width, npy/height);

    if (scale == 0) {

        // Down-scaling

        const int recip_scale = max(width/npx, height/npy);
        const int xinc = width % npx;
        const int yinc = height % npy;

        int xrem = 0, yrem = 0;
        int xbase = 0, ybase = 0;

        for (int y = top; y < top + npy; ++y) {

            xbase = 0;
            xrem = 0;

            bool extend_up = false;
            if (yrem >= npy) {
                yrem -= npy;
                extend_up = true;
            }

            for (int x = left; x < left + npx; ++x) {

                bool extend_right = false;
                if (xrem >= npx) {
                    xrem -= npx;
                    extend_right = true;
                }

                MiniMapColour csel = COL_UNMAPPED;
                for (int j = 0; j < (extend_up ? recip_scale+1 : recip_scale); ++j) {
                    for (int i = 0; i < (extend_right ? recip_scale+1 : recip_scale); ++i) {
                        ASSERT(xbase + i < width);
                        ASSERT(ybase + j < height);
                        csel = min(csel, new_data[(xbase+i) + (ybase+j)*width]);
                    }
                }

                gc.plotPixel(x, y, col[csel]);

                xrem += xinc;
                xbase += extend_right ? recip_scale+1 : recip_scale;
            }

            yrem += yinc;
            ybase += extend_up ? recip_scale+1 : recip_scale;
        }
        

    } else {

        // Up-scaling

        const int xinc = npx % width;
        const int yinc = npy % height;
    
        int xrem = 0, yrem = 0;
        int xbase = left, ybase = top;

        for (int y = 0; y < height; ++y) {

            xbase = left;
            xrem = 0;
        
            bool extend_up = false;
            if (yrem >= height) {
                yrem -= height;
                extend_up = true;
            }

            const MiniMapColour *pright = &new_data[y*width];
            const MiniMapColour *paboveright = (y+1==height) ? 0 : &new_data[(y+1)*width];
        
            MiniMapColour chere, cabove;
            MiniMapColour cright = *pright++;
            MiniMapColour caboveright = paboveright ? *paboveright++ : COL_UNMAPPED;
        
            for (int x = 0; x < width; ++x) {

                bool extend_right = false;
                if (xrem >= width) {
                    xrem -= width;
                    extend_right = true;
                }

                chere = cright;
                cabove = caboveright;
                if (x != width-1) {
                    cright = *pright++;
                    caboveright = paboveright ? *paboveright++ : COL_UNMAPPED;
                } else {
                    cright = caboveright = COL_UNMAPPED;
                }

                for (int j=0; j<scale; ++j) {
                    for (int i=0; i<scale; ++i) {
                        gc.plotPixel(xbase+i, ybase+j, col[chere]);
                    }
                }

                if (extend_right) {
                    MiniMapColour csel = max(chere, cright);
                    for (int j=0; j<scale; ++j) {
                        gc.plotPixel(xbase+scale, ybase+j, col[csel]);
                    }
                    if (extend_up) {
                        csel = max(csel, cabove);
                        csel = max(csel, caboveright);
                        gc.plotPixel(xbase+scale, ybase+scale, col[csel]);
                    }
                }
                if (extend_up) {
                    const MiniMapColour csel = max(chere, cabove);
                    for (int i=0; i<scale; ++i) {
                        gc.plotPixel(xbase+i, ybase+scale, col[csel]);
                    }
                }

                xrem += xinc;
                xbase += extend_right ? scale+1 : scale;
            }

            yrem += yinc;
            ybase += extend_up ? scale+1 : scale;
        }
    }
}

void LocalMiniMap::setSize(int w, int h)
{
    if (width != 0 || w <= 0 || h <= 0) return;
    width = w;
    height = h;
    data.resize(width*height);
    wipeMap();
}

void LocalMiniMap::setColour(int x, int y, MiniMapColour col)
{
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    data[y*width + x] = col;
}
    
void LocalMiniMap::wipeMap()
{
    std::fill(data.begin(), data.end(), COL_UNMAPPED);
}

void LocalMiniMap::mapKnightLocation(int n, int x, int y)
{
    setHighlight(x, y, n);
}

void LocalMiniMap::mapItemLocation(int x, int y, bool on)
{
    const int id = y * height + x + 1000;  // add 1000 to prevent clashes with other IDs
    if (on) {
        setHighlight(x, y, id);
    } else {
        setHighlight(-1, -1, id);
    }
}

void LocalMiniMap::setHighlight(int x, int y, int id)
{
    // The strategy is to store highlights into a separate map, rather than
    // simply setting colours directly in the "data" array. This simplifies
    // setHighlight and setColour, but it does complicate the draw routines
    // slightly.
    
    // Erase the old highlight first (if any)
    map<int,Highlight>::iterator it = highlights.find(id);
    if (it != highlights.end()) {
        highlights.erase(it);
    }

    if (x >= 0 && x < width && y >= 0 && y < height) {
        // Set the new highlight.
        Highlight h;
        h.x = x;
        h.y = y;
        highlights.insert(make_pair(id, h));
    }
}

void LocalMiniMap::makeHighlightedMap(int t, std::vector<MiniMapColour> &new_data) const
{
    const MiniMapColour col_highlight = MiniMapColour((t / config_map.getInt("mini_map_flash_time")) % NUM_HIGHLIGHTS + FIRST_HIGHLIGHT);

    new_data = data;

    for (std::map<int,LocalMiniMap::Highlight>::const_iterator it = highlights.begin(); it != highlights.end(); ++it) {
        const int idx = it->second.y * width + it->second.x;
        new_data[idx] = col_highlight;
    }
}

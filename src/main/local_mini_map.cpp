/*
 * local_mini_map.cpp
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
#include "local_mini_map.hpp"

#include <algorithm>

namespace {
    // Note: This file assumes that all MiniMapColour enum values are greater than zero.

    const Coercri::Color WALL_COL(68,68,34);
    const Coercri::Color FLOOR_COL(136,136,85);

    const Coercri::Color HIGHLIGHT_COLS[] = {
        Coercri::Color(255,0,0),
        Coercri::Color(255,255,255),
        Coercri::Color(255,0,0),
        Coercri::Color(0,0,0),
    };
    const int NUM_HIGHLIGHTS = 4;
}


// This algorithm turns a mini-map grid (post-scaling) into a
// (reasonably) minimal set of filled rectangles for drawing.
//
// The grid is specified as a 2-D array of colours. The grid cells are not
// uniformly sized; their x and y boundaries are specified in the x_coords
// and y_coords arrays.
//
// (The idea is that a small set of GfxContext::fillRectangle calls is
// more efficient than plotting individual pixels using GfxContext::plotPixel
// or even GfxContext::plotPixelBatch. Plus, we only need to recalculate the
// set of rectangles when the map changes or the window resizes; we can then
// cache the results and re-use them on subsequent frames.)
//
// Input:
//   colours = A 2-D grid of colours (with uneven grid spacing)
//   x_coords = X pixel coords of the left edge of each grid cell,
//       plus one extra entry for X-coord of right edge of final cell
//   y_coords = Y pixel coords of the top edge of each grid cell,
//       plus one extra entry for Y-coord of bottom edge of final cell
// Output:
//   The wall_rects, floor_rects and highlight_rects arrays are filled
//   with appropriate rectangles to draw.
//
void LocalMiniMap::rectangleAlgorithm(const std::vector<int> &x_coords,
                                      const std::vector<int> &y_coords,
                                      const std::vector<MiniMapColour> &colours)
{
    int w = x_coords.size() - 1;
    int h = y_coords.size() - 1;

    std::vector<char> covered(colours.size());

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {

            // If this is unmapped, we don't have to draw it.
            if (colours[y*w + x] == COL_UNMAPPED) continue;

            // If this is already covered, we don't have to draw it again.
            if (covered[y*w + x]) continue;

            // Get colour of this cell
            MiniMapColour col = colours[y*w + x];

            // Extend the rectangle as far to the right as possible.
            int num_cells_x = 1;
            for (int x2 = x + 1; x2 < w; ++x2) {
                if (colours[y*w + x2] == col) {
                    ++num_cells_x;
                    covered[y*w + x2] = true;
                } else {
                    break;
                }
            }

            // Extend the rectangle as far downward as possible.
            int num_cells_y = 1;
            for (int y2 = y + 1; y2 < h; ++y2) {
                // The whole row must be covered
                bool ok = true;
                for (int x2 = x; x2 < x + num_cells_x; ++x2) {
                    if (colours[y2*w + x2] != col) {
                        ok = false;
                        break;
                    }
                }

                // If it wasn't a match then stop here
                if (!ok) break;

                // It was a match. Extend the height, marking the entire new row as covered.
                ++num_cells_y;
                for (int x2 = x; x2 < x + num_cells_x; ++x2) {
                    covered[y2*w + x2] = true;
                }
            }

            // Compute the rectangle's coordinates on-screen
            int left = x_coords[x];
            int right = x_coords[x + num_cells_x];
            int top = y_coords[y];
            int bottom = y_coords[y + num_cells_y];
            Coercri::Rectangle rect(left, top, right - left, bottom - top);

            // Now add our rectangle to the appropriate list.

            switch (col) {
            case COL_WALL:
                wall_rects.push_back(rect);
                break;

            case COL_FLOOR:
                floor_rects.push_back(rect);
                break;

            default:
                highlight_rects.push_back(rect);
                break;
            }
        }
    }
}

// This down-scales a mini-map to fit a rectangle that is smaller (in pixels)
// than the actual map size (in squares). Each on-screen pixel represents multiple
// dungeon squares.
// Input:
//   left, top, npx, npy give the area on screen that we want to fill.
//   new_data is the mini-map grid with highlights added.
// Output:
//   grid, x_coords, y_coords are filled with a "colour grid" suitable for use
//   with rectangleAlgorithm. (These vectors are assumed empty initially.)
void LocalMiniMap::downScale(int left, int top, int npx, int npy,
                             const std::vector<MiniMapColour> &new_data,
                             std::vector<MiniMapColour> &grid,
                             std::vector<int> &x_coords,
                             std::vector<int> &y_coords)
{
    const int recip_scale = std::max(width/npx, height/npy);
    const int xinc = width % npx;
    const int yinc = height % npy;

    int xrem = 0, yrem = 0;
    int xbase = 0, ybase = 0;

    for (int x = left; x <= left + npx; ++x) {
        x_coords.push_back(x);
    }
    for (int y = top; y <= top + npy; ++y) {
        y_coords.push_back(y);
    }

    for (int y = top; y < top + npy; ++y) {

        xbase = 0;
        xrem = 0;

        bool extend_down = false;
        if (yrem >= npy) {
            yrem -= npy;
            extend_down = true;
        }

        for (int x = left; x < left + npx; ++x) {

            bool extend_right = false;
            if (xrem >= npx) {
                xrem -= npx;
                extend_right = true;
            }

            MiniMapColour csel = COL_UNMAPPED;

            const int jmin = ybase;
            const int jmax = std::min(height, ybase + recip_scale + (extend_down ? 1 : 0));

            const int imin = xbase;
            const int imax = std::min(width, xbase + recip_scale + (extend_right ? 1 : 0));

            for (int j = jmin; j < jmax; ++j) {
                for (int i = imin; i < imax; ++i) {
                    // Pick min colour of all map squares covered by this pixel
                    // (i.e. prioritize highlights, then walls, then floors, then unmapped)
                    csel = std::min(csel, new_data[j * width + i]);
                }
            }

            grid.push_back(csel);

            xrem += xinc;
            xbase += recip_scale + (extend_right ? 1 : 0);
        }

        yrem += yinc;
        ybase += recip_scale + (extend_down ? 1 : 0);
    }
}

// This up-scales a mini-map to fit a rectangle that is larger (in pixels)
// than the actual map size (in squares). Each map square expands to several
// on-screen pixels.
// This supports non-uniform scaling, i.e. npx,npy do not necessarily have to
// be an exact multiple of width,height. In the non-uniform case, extra one-
// pixel-thick "border zones" are inserted between squares, as needed.
// Input:
//   left, top, npx, npy give the area on screen that we want to fill.
//   scale equals min(npx/width, npy/height).
//   new_data is the mini-map grid with highlights added.
// Output:
//   grid, x_coords, y_coords are filled with a "colour grid" suitable for use
//   with rectangleAlgorithm. (These vectors are assumed empty initially.)
void LocalMiniMap::upScale(int left, int top, int npx, int npy,
                           int scale,
                           const std::vector<MiniMapColour> &new_data,
                           std::vector<MiniMapColour> &grid,
                           std::vector<int> &x_coords,
                           std::vector<int> &y_coords)
{
    const int xinc = npx % width;
    const int yinc = npy % height;

    int xrem = 0, yrem = 0;
    int xbase = left, ybase = top;

    std::vector<MiniMapColour> next_row;

    for (int y = 0; y < height; ++y) {

        xbase = left;
        xrem = 0;

        bool extend_down = false;
        if (yrem >= height) {
            yrem -= height;
            extend_down = true;
        }

        const MiniMapColour *pright = &new_data[y * width];
        const MiniMapColour *pbelowright = (y+1==height) ? 0 : &new_data[(y+1)*width];

        MiniMapColour chere, cbelow;
        MiniMapColour cright = *pright++;
        MiniMapColour cbelowright = pbelowright ? *pbelowright++ : COL_UNMAPPED;

        for (int x = 0; x < width; ++x) {

            bool extend_right = false;
            if (xrem >= width) {
                xrem -= width;
                extend_right = true;
            }

            chere = cright;
            cbelow = cbelowright;
            if (x != width - 1) {
                cright = *pright++;
                cbelowright = pbelowright ? *pbelowright++ : COL_UNMAPPED;
            } else {
                cright = cbelowright = COL_UNMAPPED;
            }

            // Draw colour "chere" in a rectangle occupying pixel ranges
            // [xbase, xbase+scale) and [ybase, ybase+scale).
            grid.push_back(chere);
            if (y == 0) {
                x_coords.push_back(xbase);
            }
            if (x == 0) {
                y_coords.push_back(ybase);
            }

            if (extend_right) {
                MiniMapColour csel = std::max(chere, cright);
                // Draw colour "csel" in a one-pixel-wide rectangle,
                // at x-coordinate (xbase+scale), in the y-range
                // [ybase, ybase+scale).
                grid.push_back(csel);
                if (y == 0) {
                    x_coords.push_back(xbase+scale);
                }
            }

            if (extend_down) {
                MiniMapColour csel = std::max(chere, cbelow);
                // Draw colour "csel" in a one-pixel-tall rectangle,
                // at y-coordinate (ybase+scale), in the x-range
                // [xbase, xbase+scale).
                next_row.push_back(csel);
                if (x == 0) {
                    y_coords.push_back(ybase+scale);
                }

                if (extend_right) {
                    csel = std::max(csel, cright);
                    csel = std::max(csel, cbelowright);
                    // Draw a single pixel of colour "csel" at co-ordinates
                    // (xbase+scale, ybase+scale).
                    next_row.push_back(csel);
                }
            }

            xrem += xinc;
            xbase += (extend_right ? scale+1 : scale);
        }

        if (y == 0) {
            // final x_coord
            x_coords.push_back(xbase);
        }

        grid.insert(grid.end(), next_row.begin(), next_row.end());
        next_row.clear();

        yrem += yinc;
        ybase += (extend_down ? scale+1 : scale);
    }

    // final y_coord
    y_coords.push_back(ybase);
}

// This rebuilds the wall_rects, floor_rects and highlight_rects arrays.
// It first calls downScale or upScale as appropriate, then calls rectangleAlgorithm.
void LocalMiniMap::rebuildRects(int left, int top, int npx, int npy)
{
    wall_rects.clear();
    floor_rects.clear();
    highlight_rects.clear();

    // Make a copy of the map with the correct highlights (flashing red dots) in it.
    // (The highlights will be set to colour 0.)
    std::vector<MiniMapColour> new_data(data);
    writeHighlightsToMap(new_data);

    // Now scale the mini-map up or down to fit inside the desired area on-screen.
    std::vector<MiniMapColour> grid;
    std::vector<int> x_coords;
    std::vector<int> y_coords;

    const int scale = std::min(npx/width, npy/height);

    if (scale == 0) {
        downScale(left, top, npx, npy, new_data,
                  grid, x_coords, y_coords);

    } else {
        upScale(left, top, npx, npy, scale, new_data,
                grid, x_coords, y_coords);
    }

    // Now that we have our grid and co-ords, run the rectangle algorithm!
    rectangleAlgorithm(x_coords, y_coords, grid);
}

// This is the main "draw mini-map" function.
//  - "left", "top", "npx" and "npy" define the on-screen rectangle in which the mini-map
//    is to be drawn (note "np" stands for "number of pixels", so npx and npy are the
//    width and height of the on-screen rectangle). The mini-map will be scaled up or down
//    as needed to fit this rectangle.
//  - "t" gives the current time in milliseconds. This is used for the flashing effect
//    on the mini-map highlights.
void LocalMiniMap::draw(Coercri::GfxContext &gc, int left, int top, int npx, int npy, int t)
{
    if (width==0 || height==0) return;  // mini-map itself is empty
    if (npx == 0 || npy == 0) return;   // on-screen rectangle is empty

    if (left != prev_left || top != prev_top || npx != prev_npx || npy != prev_npy) {
        // Our cached rectangles are out of date; recompute them.
        rebuildRects(left, top, npx, npy);
        prev_left = left;
        prev_top = top;
        prev_npx = npx;
        prev_npy = npy;
    }

    // Figure out the colour of the highlight (this cycles through different
    // colours over time).
    int idx_highlight = (t / config_map.getInt("mini_map_flash_time")) % NUM_HIGHLIGHTS;
    const Coercri::Color col_highlight = HIGHLIGHT_COLS[idx_highlight];

    // Now just draw all the cached rectangles using GfxContext::fillRectangle.
    for (auto const& rect : wall_rects) {
        gc.fillRectangle(rect, WALL_COL);
    }
    for (auto const& rect : floor_rects) {
        gc.fillRectangle(rect, FLOOR_COL);
    }
    for (auto const& rect : highlight_rects) {
        gc.fillRectangle(rect, col_highlight);
    }
}

// Resizes the mini-map. w and h are the new dungeon size in squares.
void LocalMiniMap::setSize(int w, int h)
{
    if (width != 0 || w <= 0 || h <= 0) return;
    width = w;
    height = h;
    data.resize(width*height);
    wipeMap();
}

// Set the base colour of a particular dungeon square.
void LocalMiniMap::setColour(int x, int y, MiniMapColour col)
{
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    data[y*width + x] = col;
    prev_npx = 0;  // force rect update
}

// Fill the entire map with COL_UNMAPPED.
void LocalMiniMap::wipeMap()
{
    std::fill(data.begin(), data.end(), COL_UNMAPPED);
    prev_npx = 0;  // force rect update
}

// Set a highlight for knight number "n" at position x,y. This
// temporarily overrides the base colour of that square.
// If a highlight for knight "n" already exists, then it is
// moved to the new position.
// If x == y == -1, the highlight for knight "n" is removed
// entirely.
void LocalMiniMap::mapKnightLocation(int n, int x, int y)
{
    setHighlight(x, y, n);
}

// Set an item highlight at position x,y (if on=true), or
// remove it (if on=false).
void LocalMiniMap::mapItemLocation(int x, int y, bool on)
{
    const int id = y * width + x + 1000;  // add 1000 to prevent clashes with other IDs
    if (on) {
        setHighlight(x, y, id);
    } else {
        setHighlight(-1, -1, id);
    }
}

// Helper function used internally to set highlights.
void LocalMiniMap::setHighlight(int x, int y, int id)
{
    // The strategy is to store highlights into a separate map, rather than
    // simply setting colours directly in the "data" array. This simplifies
    // setHighlight and setColour, but it does complicate the draw routines
    // slightly.

    // Erase the old highlight first (if any)
    highlights.erase(id);

    if (x >= 0 && x < width && y >= 0 && y < height) {
        // Set the new highlight.
        Highlight h;
        h.x = x;
        h.y = y;
        highlights.insert(std::make_pair(id, h));
    }

    prev_npx = 0;  // force rect update
}

// Helper function. For each highlight, this overwrites the value in "new_data"
// at the position of that highlight with a zero value.
void LocalMiniMap::writeHighlightsToMap(std::vector<MiniMapColour> &new_data) const
{
    for (std::map<int,LocalMiniMap::Highlight>::const_iterator it = highlights.begin(); it != highlights.end(); ++it) {
        const int idx = it->second.y * width + it->second.x;
        new_data[idx] = MiniMapColour(0); // Highlight marker
    }
}

/*
 * local_mini_map.hpp
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
 * Implementation of MiniMap for on-screen drawing.
 *
 */

#ifndef LOCAL_MINI_MAP_HPP
#define LOCAL_MINI_MAP_HPP

#include "mini_map.hpp"

#include "gfx/gfx_context.hpp"  // coercri
#include "gfx/rectangle.hpp"

#include <map>
#include <vector>

class ConfigMap;

class LocalMiniMap : public MiniMap {
public:

    //
    // functions specific to this class
    //

    explicit LocalMiniMap(const ConfigMap &cfg)
        : config_map(cfg), width(0), height(0),
          prev_left(0), prev_top(0), prev_npx(0), prev_npy(0)
    {}

    void draw(Coercri::GfxContext &gc, int left, int top, int width, int height, int time);
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
    //
    // functions overridden from MiniMap
    //

    void setSize(int width, int height);
    void setColour(int x, int y, MiniMapColour col);
    void wipeMap();
    void mapKnightLocation(int n, int x, int y);
    void mapItemLocation(int x, int y, bool on);

private:
    void rectangleAlgorithm(const std::vector<int>&, const std::vector<int>&, const std::vector<MiniMapColour>&);
    void downScale(int left, int top, int npx, int npy,
                   const std::vector<MiniMapColour> &new_data,
                   std::vector<MiniMapColour> &grid,
                   std::vector<int> &x_coords,
                   std::vector<int> &y_coords);
    void upScale(int left, int top, int npx, int npy,
                 int scale,
                 const std::vector<MiniMapColour> &new_data,
                 std::vector<MiniMapColour> &grid,
                 std::vector<int> &x_coords,
                 std::vector<int> &y_coords);
    void rebuildRects(int left, int top, int npx, int npy);
    void setHighlight(int x, int y, int id);
    void writeHighlightsToMap(std::vector<MiniMapColour> &new_data) const;
    
    const ConfigMap &config_map;

    int width, height;
    std::vector<MiniMapColour> data;

    struct Highlight {
        int x, y;
    };
    std::map<int, Highlight> highlights;

    int prev_left, prev_top, prev_npx, prev_npy;
    std::vector<Coercri::Rectangle> wall_rects;
    std::vector<Coercri::Rectangle> floor_rects;
    std::vector<Coercri::Rectangle> highlight_rects;
};

#endif

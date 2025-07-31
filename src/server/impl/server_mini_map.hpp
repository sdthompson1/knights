/*
 * server_mini_map.hpp
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

#ifndef SERVER_MINI_MAP_HPP
#define SERVER_MINI_MAP_HPP

#include "mini_map.hpp"

#include <map>
#include <vector>

class ServerMiniMap : public MiniMap {
public:
    typedef unsigned char ubyte;
    explicit ServerMiniMap(std::vector<ubyte> &out_) : out(out_) { }

    void appendMiniMapCmds(std::vector<ubyte> &vec) const;
    void clearMiniMapCmds() { mini_map_runs.clear(); }
    
    virtual void setSize(int width, int height) override;
    virtual void setColour(int x, int y, MiniMapColour col) override;
    virtual void wipeMap() override;
    virtual void mapKnightLocation(int n, int x, int y) override;
    virtual void mapItemLocation(int x, int y, bool on) override;

    void prepareForCatchUp() { prev_kt_locn.clear(); }

private:
    std::vector<ubyte> &out;

    struct MiniMapRun {
        int start_x;
        int y;
        std::vector<MiniMapColour> cols;
    };
    std::vector<MiniMapRun> mini_map_runs;

    // caching
    struct KtLocn {
        int x;
        int y;
    };
    std::map<int, KtLocn> prev_kt_locn;
};

#endif

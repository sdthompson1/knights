/*
 * load_segments.cpp
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

#include "knights_config_impl.hpp"
#include "load_segments.hpp"
#include "lua_userdata.hpp"
#include "rstream.hpp"
#include "rstream_find.hpp"
#include "segment.hpp"
#include "trim.hpp"

#include "lua.hpp"

using std::auto_ptr;

void LoadSegments(lua_State *lua, KnightsConfigImpl *kc,
                  const char *filename, const boost::filesystem::path &cwd)
{
    // [tiletbl]
    
    boost::filesystem::path to_load = RStreamFind(filename, cwd);
    RStream str(to_load);

    lua_newtable(lua);    // [tiletbl result]
    lua_insert(lua, -2);  // [result tiletbl]

    int idx = 1;
    
    while (1) {
        std::string x;
        std::getline(str, x);
        if (!str) luaL_error(lua, "Problem loading segments: read error");

        x = Trim(x);

        if (x == "segment") {
            auto_ptr<Segment> segment(new Segment(str, lua)); // reads tiletbl (doesn't pop)
            Segment *result = kc->addLuaSegment(segment); // hands over 'segment'
            NewLuaPtr<Segment>(lua, result);  // [result tiletbl newseg]
            lua_rawseti(lua, -3, idx++);      // [result tiletbl]

        } else if (x == "eof") {
            lua_pop(lua, 1);   // [result]
            return;

        } else if (x.size() > 0 && x[0] == '#'
                   || x.size() == 0) {
            // Comment or blank line
            // (Do nothing)

        } else {
            luaL_error(lua, "Problem loading segments: incorrect file format");
        }
    }
}

/*
 * dungeon_layout.cpp
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

#include "dungeon_layout.hpp"
#include "lua_check.hpp"
#include "my_exceptions.hpp"

#include "lua.hpp"

namespace {
    void MakeLowerCase(std::string &s) {
        for (int i=0; i<s.size(); ++i) {
            s[i] = tolower(s[i]);
        }
    }

    int GetPositiveInteger(lua_State *lua, const char *fieldname)
    {
        // [table]
        lua_pushstring(lua, fieldname);  // [t fieldname]
        lua_gettable(lua, -2);           // [t value]
        if (!lua_isnumber(lua, -1)) {
            lua_pop(lua, 1);  // [t]
            throw LuaError(std::string("Problem with dungeon layout: '") + fieldname + "' missing");
        }
        const int result = lua_tointeger(lua, -1);
        if (result <= 0) {
            throw LuaError(std::string("Problem with dungeon layout: '") + fieldname + "' is negative");
        }
        lua_pop(lua, 1);
        return result;
    }

    BlockType GetBlockType(lua_State *lua)
    {
        // [t datatable blocktable]
        lua_pushstring(lua, "type");   // [t dt bt "type"]
        lua_gettable(lua, -2);         // [t dt bt typestring]
        const char * str = lua_tostring(lua, -1);

        BlockType result;
        bool found = false;
        
        if (str) {
            std::string s(str);
            MakeLowerCase(s);
            found = true;
            if (s == "block") result = BT_BLOCK;
            else if (s == "edge") result = BT_EDGE;
            else if (s == "special") result = BT_SPECIAL;
            else if (s == "none") result = BT_NONE;
            else found = false;
        }                
        
        if (!found) {
            throw LuaError("Problem with dungeon layout: invalid or missing 'type'; must be 'block', 'edge', 'special' or 'none'");
        }

        lua_pop(lua, 1);  // [t dt bt]
        
        return result;
    }

    void GetExits(lua_State *lua, bool &n, bool &e, bool &s, bool &w, bool &def)
    {
        // [t datatable blocktable]
        lua_pushstring(lua, "exits");  // [t dt bt "exits"]
        lua_gettable(lua, -2);    // [t dt bt exitstring]

        const char *str = lua_tostring(lua, -1);
        if (!str) {
            def = true;
        } else {
            std::string st(str);
            n = st.find_first_of("nN") != string::npos;
            e = st.find_first_of("eE") != string::npos;
            s = st.find_first_of("sS") != string::npos;
            w = st.find_first_of("wW") != string::npos;
            def = false;
        }

        lua_pop(lua, 1);   // [t dt bt]
    }
}

DungeonLayout::DungeonLayout(lua_State *lua)
{
    // [t]
    LuaCheckIndexable(lua, -1, "return value from a DungeonLayout function");

    width = GetPositiveInteger(lua, "width");
    height = GetPositiveInteger(lua, "height");
    data.resize(width * height);
    horiz_exits.resize((width-1) * height);
    vert_exits.resize(width * (height-1));

    lua_pushstring(lua, "data");   // [t "data"]
    lua_gettable(lua, -2);         // [t datatable]
    lua_len(lua, -1);              // [t dt len]
    if (lua_tointeger(lua, -1) != width * height) {
        lua_pop(lua, 2);  // [t]
        throw LuaError("Problem with dungeon layout: 'data' table has wrong number of entries");
    }
    lua_pop(lua, 1);  // [t dt]

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            lua_pushinteger(lua, y*width + x + 1);   // [t dt index]
            lua_gettable(lua, -2);                   // [t dt bt]
            
            // set block type
            const BlockType bt = GetBlockType(lua);
            data[y*width + x] = bt;

            lua_pop(lua, 1);   // [t dt]
        }
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // read exits from the table

            lua_pushinteger(lua, y*width + x + 1);   // [t dt index]
            lua_gettable(lua, -2);                   // [t dt bt]

            bool n, e, s, w, def;
            GetExits(lua, n, e, s, w, def);

            if (def) {

                // Figure it out ourselves

                if (data[y*width + x] == BT_NONE) {
                    n = e = s = w = false;

                } else {
                
                    if (x == 0) {
                        w = false;
                    } else {
                        w = data[y*width + x-1] != BT_NONE;
                    }

                    if (x == width-1) {
                        e = false;
                    } else {
                        e = data[y*width + x+1] != BT_NONE;
                    }

                    if (y == 0) {
                        n = false;
                    } else {
                        n = data[(y-1)*width + x] != BT_NONE;
                    }

                    if (y == height-1) {
                        s = false;
                    } else {
                        s = data[(y+1)*width + x] != BT_NONE;
                    }
                }
            }


            // Error checking
            const char *err = 0;

            if (n && y == 0
                    || e && x == width-1
                    || s && y == height-1
                    || w && x == 0) {
                err = "Illegal exit from map edge";
            }

            if (x > 0) {
                // Check 'w' exit matches previously installed 'e' exit
                if (w != horiz_exits[y*(width-1) + (x-1)]) {
                    err = "Exits do not match";
                }
            }

            if (y > 0) {
                // Check 'n' exit matches previously installed 's' exit
                if (n != vert_exits[(y-1)*width + x]) {
                    err = "Exits do not match";
                }
            }

            if (err) {
                throw LuaError(err);
            }

            // Install exits
            if (y < height-1) vert_exits[y*width + x] = s;
            if (x < width-1) horiz_exits[y*(width-1) + x] = e;

            lua_pop(lua, 1);   // [t dt]
        }
    }

    lua_pop(lua, 2);
}

BlockType DungeonLayout::getBlockType(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= height) return BT_BLOCK;
    return data[y*width+x];
}

bool DungeonLayout::hasVertExit(int x, int y) const
{
    if (x < 0 || x >= width || y < 0 || y >= (height-1)) return false;
    return vert_exits[y*width + x];
}

bool DungeonLayout::hasHorizExit(int x, int y) const
{
    if (x < 0 || x >= (width-1) || y < 0 || y >= height) return false;
    return horiz_exits[y*(width-1) + x];
}

/*
 * lua_traceback.cpp
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

#include "lua_traceback.hpp"

#include "lua.hpp"

#include <sstream>

std::string LuaTraceback(lua_State *lua)
{
    const int MAXLEVEL = 15;

    bool found_lua_yet = false;

    std::ostringstream str;
    
    for (int level = 0; level <= MAXLEVEL; ++level) {
        lua_Debug ar;
        const int result = lua_getstack(lua, level, &ar);
        if (result == 0) {
            // We have reached top of stack
            break;
        }

        lua_getinfo(lua, "Sl", &ar);

        if (*ar.what == 'C') {
            if (found_lua_yet) {
                // We have reached an enclosing C function. Stop traceback.
                break;
            } else {
                // Skip lowest level C functions (they are usually uninteresting e.g. error handler functions)
                continue;
            }
        }

        if (found_lua_yet) str << ',';
        else str << "\nTraceback:";
        str << "   " << ar.short_src << ':' << ar.currentline;
        found_lua_yet = true;
    }

    return str.str();
}

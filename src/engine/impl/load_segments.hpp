/*
 * load_segments.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2024.
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

#ifndef LOAD_SEGMENTS_HPP
#define LOAD_SEGMENTS_HPP

class KnightsConfigImpl;

struct lua_State;

//
// Function to load a list of segments from a text file
// On entry: there is a tiletable on top of lua stack
// On exit: tiletable is popped, and a table of segments is pushed.
// Note: This can raise lua errors.
//

void LoadSegments(lua_State *lua, KnightsConfigImpl *kc,
                  const char *filename,
                  const std::string &cwd);

#endif

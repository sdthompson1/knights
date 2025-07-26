/*
 * trim.cpp
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

#include "my_ctype.hpp"
#include "trim.hpp"

std::string Trim(const std::string &s)
{
    size_t beg = 0;
    while (beg < s.size()) {
        if (!IsSpace(s[beg])) break;
        ++beg;
    }
    // 'beg' is now the first non-space character
    // (or size() if none such exists)
    
    size_t end = s.size();
    while (end != beg) {
        if (!IsSpace(s[end-1])) break;
        --end;
    }
    // 'end' is now one plus the last non-space character,
    // or equal to beg, whichever is greater
    
    return s.substr(beg, end - beg);
}        

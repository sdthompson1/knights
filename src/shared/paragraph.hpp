/*
 * paragraph.hpp
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

#ifndef PARAGRAPH_HPP
#define PARAGRAPH_HPP

#include "localization.hpp"

// This struct is used for quest descriptions
struct Paragraph {
    LocalKey key;
    int plural;  // negative if doesn't need to be pluralized
    std::vector<LocalParam> params;

    bool operator==(const Paragraph &other) const {
        return key == other.key && plural == other.plural && params == other.params;
    }
    bool operator!=(const Paragraph &other) const {
        return !(*this == other);
    }
};

#endif

/*
 * colour_change.hpp
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

/*
 * Defines a mapping from old colours to new colours. Used for
 * dynamically changing colours of Graphics, eg for Knight House
 * Colours, or for changing the colour of your potion bottle when you
 * have strength, super etc.
 *
 */

#ifndef COLOUR_CHANGE_HPP
#define COLOUR_CHANGE_HPP

#include "network/byte_buf.hpp" // coercri

#include <vector>

struct Colour {
    unsigned char r, g, b, a;
    Colour() { }
    Colour(unsigned char rr, unsigned char gg, unsigned char bb, unsigned char aa=255) : r(rr), g(gg), b(bb), a(aa) { }
    bool operator<(const Colour &other) const {
        return r < other.r ? true
            : r > other.r ? false
            : g < other.g ? true
            : g > other.g ? false
            : b < other.b ? true
            : b > other.b ? false
            : a < other.a;
    };
    bool operator==(const Colour &other) const {
        return r==other.r && g==other.g && b==other.b && a == other.a;
    }
    bool operator!=(const Colour &other) const {
        return !(*this == other);
    }

    // serialization
    explicit Colour(Coercri::InputByteBuf &buf);
    void serialize(Coercri::OutputByteBuf &buf) const;
};

class ColourChange {
public:
    // Constructor: creates an 'empty' ColourChange object (where all colours are passed
    // through unmodified).
    ColourChange() { }
    
    // empty(): true if this is the 'identity' colour change
    bool empty() const { return mappings.empty(); }
    
    // add: adds a mapping from old_col to new_col
    void add(Colour old_col, Colour new_col);

    // lookup: returns false if the src colour is to be left unchanged
    // returns true (and sets new_col) if the src colour is to be
    // changed to new_col.
    bool lookup(Colour old_col, Colour &new_col) const;

    // comparison functions.
    bool operator<(const ColourChange &other) const { return mappings < other.mappings; }
    bool operator==(const ColourChange &other) const { return mappings == other.mappings; }

    // serialization
    explicit ColourChange(Coercri::InputByteBuf &buf);
    void serialize(Coercri::OutputByteBuf &buf) const;
    
private:
    // Vector of (old,new) pairs. This is kept sorted (on "old" values) at all times.
    std::vector<std::pair<Colour,Colour> > mappings;
};

#endif

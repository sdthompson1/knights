/*
 * item_generator.hpp
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

#ifndef ITEM_GENERATOR_HPP
#define ITEM_GENERATOR_HPP

#include "kconfig_fwd.hpp"
using namespace KConfig;

#include <vector>
using namespace std;

class ItemType;

//
// ItemGenerator generates an ItemType at random from a predetermined list.
// (It's very similar to RandomAction. Perhaps there should be some sort of template 
// class RandomList<T>.)
//
class ItemGenerator {
public:
    ItemGenerator() : total_weight(0), fixed_item_type(0), amount(0) { }

    // add "fixed item"
    // amount==NULL is treated as amount==RIConstant(1).
    void setFixedItemType(const ItemType *i, const RandomInt * n)
        { fixed_item_type = i; amount = n; }

    // add "child generators"
    void reserve(int n) { data.reserve(n); }
    void add(const ItemGenerator *ig, int wt);

    // If a fixed item type is set, then get() will return that.
    // Otherwise, one of the child item generators will be invoked.    
    pair<const ItemType *, int> get() const;
    
private:
    vector<pair<const ItemGenerator *, int> > data;
    int total_weight;
    const ItemType *fixed_item_type;
    const RandomInt * amount;
};

#endif

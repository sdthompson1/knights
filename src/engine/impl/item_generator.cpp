/*
 * item_generator.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
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

#include "item_generator.hpp"
#include "rng.hpp"

#include "random_int.hpp"
using namespace KConfig;

void ItemGenerator::add(const ItemGenerator *ig, int wt)
{
    if (ig && wt > 0) {
        data.push_back(make_pair(ig, wt));
        total_weight += wt;
    }
}

pair<const ItemType *, int> ItemGenerator::get() const
{
    if (fixed_item_type) {
        int n = 1;
        if (amount) n = amount->get();
        return make_pair(fixed_item_type,n);
    } else if (total_weight > 0) {
        int r = g_rng.getInt(0, total_weight);
        for (int i=0; i<data.size(); ++i) {
            r -= data[i].second;
            if (r < 0) return data[i].first->get();
        }
        ASSERT(0);
    }
    return pair<const ItemType *, int>(0,0);
}



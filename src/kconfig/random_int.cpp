/*
 * random_int.cpp
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

#include "random_int.hpp"

int KConfig::RIDice::get() const
{
	int x = 0;
	for (int i=0; i<n; ++i) {
		x += GetRandom(d) + 1;
	}
	return x;
}

int KConfig::RIList::get() const
{
	using std::vector;
	using std::pair;
	int w = GetRandom(total_weight);
	for (vector<pair<const RandomInt*, int> >::const_iterator it = contents.begin(); it != contents.end(); ++it) {
		w -= it->second;
		if (w < 0) {
			return it->first->get();
		}
	}
	ASSERT(0);
	return 0;
}


/*
 * segment_set.cpp
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

#include "rng.hpp"
#include "segment.hpp"
#include "segment_set.hpp"

SegmentSet::~SegmentSet()
{
    // delete all contained segments
    for (vector<vector<const Segment *> >::iterator it1 = segments.begin(); it1 != segments.end(); ++it1) {
        for (vector<const Segment *>::iterator it2 = it1->begin(); it2 != it1->end(); ++it2) {
            delete *it2;
        }
    }
    for (vector<vector<const Segment *> >::iterator it1 = special_segments.begin(); it1 != special_segments.end(); ++it1) {
        for (vector<const Segment *>::iterator it2 = it1->begin(); it2 != it1->end(); ++it2) {
            delete *it2;
        }
    }
}

void SegmentSet::addSegment(const Segment *r, int nhomes, int category)
{
    if (nhomes < 0) return;
    if (category < 0) {
        if (segments.size() < nhomes+1) segments.resize(nhomes+1);
        segments[nhomes].push_back(r);
    } else {
        if (special_segments.size() < category+1) special_segments.resize(category+1);
        special_segments[category].push_back(r);
    }
}

const Segment * SegmentSet::getHomeSegment(int minhomes) const
{
    int nsets = segments.size();
    if (minhomes >= nsets) return 0;
    if (minhomes < 0) minhomes = 0;

    int nsegments = 0;
    for (int i=minhomes; i<nsets; ++i) nsegments += segments[i].size();

    int r = g_rng.getInt(0, nsegments);
    for (int i=minhomes; i<nsets; ++i) {
        if (r < segments[i].size()) {
            return segments[i][r];
        } else {
            r -= segments[i].size();
        }
    }
    
    ASSERT(0);
    return 0;
}

const Segment * SegmentSet::getSpecialSegment(int category) const
{
    if (category < 0 || category >= special_segments.size()) return 0;
    int r = g_rng.getInt(0, special_segments[category].size());
    return special_segments[category][r];
}

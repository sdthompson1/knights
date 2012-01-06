/*
 * segment_set.hpp
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

/*
 * SegmentSet: collection of Segments.
 *
 */

#ifndef SEGMENT_SET_HPP
#define SEGMENT_SET_HPP

class Segment;

#include "boost/noncopyable.hpp"

#include <vector>
using namespace std;

class SegmentSet : boost::noncopyable {
public:
    ~SegmentSet();  // deletes all contained segments.
    
    void addSegment(const Segment *, int nhomes, int category = -1);
    
    const Segment * getHomeSegment(int minhomes) const;
    const Segment * getSpecialSegment(int category) const;
    
private:
    vector<vector<const Segment *> > segments;    // segments[nhomes][segmentno].
    vector<vector<const Segment *> > special_segments;  // special_segments[category][segmentno].
};

#endif

/*
 * FILE:
 *   region.hpp
 *
 * PURPOSE:
 *   A Region is a list of non-overlapping rectangles
 *
 * AUTHOR:
 *   Stephen Thompson <stephen@solarflare.org.uk>
 *
 * COPYRIGHT:
 *   Copyright (C) Stephen Thompson, 2008 - 2024.
 *
 *   This file is part of the "Coercri" software library. Usage of "Coercri"
 *   is permitted under the terms of the Boost Software License, Version 1.0, 
 *   the text of which is displayed below.
 *
 *   Boost Software License - Version 1.0 - August 17th, 2003
 *
 *   Permission is hereby granted, free of charge, to any person or organization
 *   obtaining a copy of the software and accompanying documentation covered by
 *   this license (the "Software") to use, reproduce, display, distribute,
 *   execute, and transmit the Software, and to prepare derivative works of the
 *   Software, and to permit third-parties to whom the Software is furnished to
 *   do so, all subject to the following:
 *
 *   The copyright notices in the Software and this entire statement, including
 *   the above license grant, this restriction and the following disclaimer,
 *   must be included in all copies of the Software, in whole or in part, and
 *   all derivative works of the Software, unless such copies or derivative
 *   works are solely in the form of machine-executable object code generated by
 *   a source language processor.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 *   SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 *   FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *   DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef COERCRI_REGION_HPP
#define COERCRI_REGION_HPP

#include <vector>

namespace Coercri {

    class Rectangle;
    
    class Region {
    public:
        // Add rectangle. If an overlapping rectangle is added then it
        // will automatically be split up into multiple non-overlapping
        // rectangles.
        void addRectangle(const Rectangle &rect);

        // Add all rectangles from another region
        void addRegion(const Region &other);

        // Remove all rectangles
        void clear() { rectangles.clear(); }
        
        // Get smallest rectangle containing the whole region
        Rectangle getBoundingBox() const;

        // Iterator interface to loop over the constituent rectangles.
        // NOTE: All contained rectangles will be non-degenerate and
        // non-overlapping. No other guarantees are given.
        typedef std::vector<Rectangle>::const_iterator const_iterator;
        const_iterator begin() const { return rectangles.begin(); }
        const_iterator end() const { return rectangles.end(); }

        // Check whether the region is empty
        bool isEmpty() const { return rectangles.empty(); }
        
    private:
        // TODO: Should use a more efficient implementation, cf. X11
        // which represents a region as a list of horizontal "spans".
        std::vector<Rectangle> rectangles;
    };

}

#endif

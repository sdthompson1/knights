/*
 * FILE: 
 *   region.cpp
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

#include "rectangle.hpp"
#include "region.hpp"

#include <list>

using Coercri::Rectangle;
using std::list;
using std::vector;

namespace {
    void AddRect(int l, int t, int r, int b, list<Rectangle> &lst, list<Rectangle>::iterator it)
    {
        Rectangle rect = Rectangle(l, t, r-l, b-t);
        if (!rect.isDegenerate()) {
            lst.insert(it, rect);
        }
    }
    
    void RectMinus(const Rectangle &lhs, const Rectangle &rhs, list<Rectangle> &lst, list<Rectangle>::iterator it)
    {
        const Rectangle intsct = IntersectRects(lhs, rhs);
        AddRect(lhs.getLeft(), lhs.getTop(), lhs.getRight(), intsct.getTop(), lst, it);
        AddRect(lhs.getLeft(), intsct.getTop(), intsct.getLeft(), intsct.getBottom(), lst, it);
        AddRect(intsct.getRight(), intsct.getTop(), lhs.getRight(), intsct.getBottom(), lst, it);
        AddRect(lhs.getLeft(), intsct.getBottom(), lhs.getRight(), lhs.getBottom(), lst, it);
    }
}

namespace Coercri {

    void Region::addRectangle(const Rectangle &rect)
    {
        if (rect.isDegenerate()) return;
        
        // Break into several rectangles if necessary.
        // Note: we don't bother trying to optimize by merging together adjacent
        // rectangles that share a side.
        list<Rectangle> new_rects;
        new_rects.push_back(rect);
        
        for (vector<Rectangle>::const_iterator exist = rectangles.begin(); exist != rectangles.end(); ++exist) {
            for (list<Rectangle>::iterator add = new_rects.begin(); add != new_rects.end(); ) {
                RectMinus(*add, *exist, new_rects, add);  // inserts Add\Exist into new_rects.
                add = new_rects.erase(add);               // removes Add from new_rects
            }
        }
        rectangles.reserve(rectangles.size() + new_rects.size());
        for (list<Rectangle>::const_iterator it = new_rects.begin(); it != new_rects.end(); ++it) {
            rectangles.push_back(*it);
        }
    }

    void Region::addRegion(const Region &other)
    {
        for (const_iterator it = other.begin(); it != other.end(); ++it) {
            addRectangle(*it);
        }
    }

    Rectangle Region::getBoundingBox() const
    {
        Rectangle bb;
        for (vector<Rectangle>::const_iterator it = rectangles.begin(); it != rectangles.end(); ++it) {
            if (bb.isDegenerate()) {
                bb = *it;
            } else {
                const int new_left = std::min(bb.getLeft(), it->getLeft());
                const int new_right = std::max(bb.getRight(), it->getRight());
                const int new_top = std::min(bb.getTop(), it->getTop());
                const int new_bottom = std::max(bb.getBottom(), it->getBottom());
                bb = Rectangle(new_left, new_top, new_right-new_left, new_bottom-new_top);
            }
        }
        return bb;
    }
}

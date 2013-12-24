/*
 * title_block.hpp
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
 * Widget for title boxes, as drawn above a ListBox.
 *
 */

#ifndef TITLE_BLOCK_HPP
#define TITLE_BLOCK_HPP

#include "guichan.hpp"

#include <string>
#include <vector>
using namespace std;

class TitleBlock : public gcn::Widget {
public:
    // ctor copies the two input vectors.
    explicit TitleBlock(const std::vector<std::string> &titles_, const std::vector<int> &widths_);

    void draw(gcn::Graphics *graphics);
    void drawFrame(gcn::Graphics *) { }
    
private:
    std::vector<std::string> titles;
    std::vector<int> widths;
};

#endif

/*
 * title_block.cpp
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

#include "title_block.hpp"

#include <numeric>

TitleBlock::TitleBlock(const std::vector<std::string> &t, const std::vector<int> &w)
    : titles(t), widths(w)
{
    setSize(std::accumulate(widths.begin(), widths.end(), 0), getFont()->getHeight() + 4);
}

namespace {
    void DrawTitleBox(gcn::Graphics *gfx, const gcn::Color &base_color, int x1, int y1, int x2, int y2)
    {
        gfx->setColor(gcn::Color(255, 255, 255));
        gfx->drawLine(x1, y1, x2-2, y1);
        gfx->drawLine(x1, y1, x1, y2-1);
        gfx->setColor(gcn::Color(0,0,0));
        gfx->drawLine(x2-1, y1, x2-1, y2-1);
        gfx->drawLine(x1+1, y2-1, x2-1, y2-1);
        gfx->setColor(base_color);
        gfx->fillRectangle(gcn::Rectangle(x1+1, y1+1, x2-x1-2, y2-y1-2));
    }
}

void TitleBlock::draw(gcn::Graphics *graphics)
{
    graphics->setFont(getFont());
    int x = 0;
    for (int i = 0; i < int(titles.size()); ++i) {
        DrawTitleBox(graphics, getBaseColor(), x, 0, x + widths[i], getHeight());
        graphics->setColor(getForegroundColor());
        graphics->drawText(titles[i], x + 2, 2, gcn::Graphics::LEFT);
        x += widths[i];
    }
}

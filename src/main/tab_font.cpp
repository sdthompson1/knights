/*
 * tab_font.cpp
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

#include "tab_font.hpp"

TabFont::TabFont(boost::shared_ptr<gcn::Font> b, const std::vector<int> &t)
    : base_font(b), field_widths(t)
{ }

namespace {
    // precondition: !s.empty()
    std::string GetNext(std::string &s)
    {
        std::string result;
        size_t pos = s.find('\t');
        if (pos == std::string::npos) {
            result = s;
            s.clear();
        } else {
            result = s.substr(0, pos);
            s = s.substr(pos+1);
        }
        return result;
    }

    void ClipString(std::string &s, const gcn::Font &font, int max_width)
    {
        while (font.getWidth(s) > max_width && !s.empty()) {
            s.erase(s.length()-1);
        }
    }
}

void TabFont::drawString(gcn::Graphics *graphics, const std::string &text, int x, int y)
{
    int i = 0;
    std::string s = text;
    while (!s.empty() && i < field_widths.size()) {
        std::string to_draw = GetNext(s);
        ClipString(to_draw, *base_font, field_widths[i]);
        base_font->drawString(graphics, to_draw, x, y);
        x += field_widths[i];
        ++i;
    }
}

int TabFont::getWidth(const std::string &text) const
{
    int i = 0, x = 0, w = 0;
    std::string s = text;
    std::string last_str;
    while (!s.empty() && i < field_widths.size()) {
        std::string to_draw = GetNext(s);
        ClipString(to_draw, *base_font, field_widths[i]);
        w = x + base_font->getWidth(to_draw);
        x += field_widths[i];
        ++i;
    }
    return w;
}

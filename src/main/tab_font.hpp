/*
 * tab_font.hpp
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
 * Modifies a Font to draw with tab stops. Used to implement a sort of
 * poor man's multi column list box (Guichan doesn't include a multi
 * column list widget).
 *
 */

#ifndef TAB_FONT_HPP
#define TAB_FONT_HPP

#include "guichan.hpp"
#include "boost/shared_ptr.hpp"
#include <vector>

class TabFont : public gcn::Font {
public:
    TabFont(boost::shared_ptr<gcn::Font> base_font, const std::vector<int> & field_widths_);
    virtual void drawString(gcn::Graphics *graphics, const std::string &text, int x, int y);
    virtual int getHeight() const { return base_font->getHeight(); }
    virtual int getWidth(const std::string &text) const;
private:
    boost::shared_ptr<gcn::Font> base_font;
    std::vector<int> field_widths;
};

#endif

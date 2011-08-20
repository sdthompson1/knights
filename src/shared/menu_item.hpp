/*
 * menu_item.hpp
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

/*
 * Represents a single setting in the quest menu (e.g. "Amount of Stuff")
 *
 */

#ifndef MENU_ITEM_HPP
#define MENU_ITEM_HPP

#include "network/byte_buf.hpp"  // coercri

#include <string>
#include <vector>

class MenuItem {
public:
    // menu items have a string key, and a numeric value between min_value
    // and min_value + num_values - 1 (inclusive).
    const std::string & getKey() const { return key; }
    int getMinValue() const;
    int getNumValues() const;
    bool hasNumValues() const;

    // presentation
    const std::string & getTitleString() const { return title_str; }
    std::string getValueString(int i) const;
    bool getSpaceAfter() const { return space_after; }
    
    // construction
    MenuItem(const std::string &key_, int min_value_, int nvalues,
             const std::string &title)
        : key(key_), min_value(min_value_),
          title_str(title), value_str(nvalues), space_after(false)  { }
    void setValueString(int val, const std::string &str) { value_str.at(val - min_value) = str; }
    void setSpaceAfter() { space_after = true; }
    
    // serialization
    explicit MenuItem(Coercri::InputByteBuf &buf);
    void serialize(Coercri::OutputByteBuf &buf) const;

private:
    std::string key;
    int min_value;
    std::string title_str;
    std::vector<std::string> value_str;
    bool space_after;
};

#endif

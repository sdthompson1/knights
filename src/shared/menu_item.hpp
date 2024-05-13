/*
 * menu_item.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2024.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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
 * Represents a single option in the quest menu (e.g. "Amount of Stuff")
 *
 */

#ifndef MENU_ITEM_HPP
#define MENU_ITEM_HPP

#include "network/byte_buf.hpp"  // coercri

#include <stdexcept>
#include <string>
#include <vector>

class MenuItem {
public:
    // general properties
    const std::string & getTitleString() const { return title; }
    bool getSpaceAfter() const { return space_after; }
    
    // type (numeric or dropdown)
    bool isNumeric() const { return numeric; }

    // properties for numeric fields
    int getNumDigits() const { return num_digits; }
    const std::string &getSuffix() const { return suffix; }

    // properties for dropdown fields
    int getNumChoices() const { return value_str.size(); }
    const std::string & getChoiceString(int i) const { return value_str.at(i); }

    
    // construction - dropdown
    MenuItem(const std::string &title_,
             std::vector<std::string> &settings)  // swapped into place
        : title(title_), numeric(false), space_after(false)
    {
        if (settings.empty()) throw std::invalid_argument("MenuItem with no choices!");
        value_str.swap(settings);
    }

    // construction - numeric
    MenuItem(const std::string &title_,
             int num_digits_,
             const std::string &suffix_)
        : title(title_), numeric(true),
          num_digits(num_digits_), suffix(suffix_),
          space_after(false)
    { }

    // space_after is set after the fact
    void addSpaceAfter() { space_after = true; }
    
    // serialization
    explicit MenuItem(Coercri::InputByteBuf &buf);
    void serialize(Coercri::OutputByteBuf &buf) const;

private:
    // general properties
    std::string title;
    bool numeric;

    // numeric properties
    int num_digits;
    std::string suffix;

    // dropdown properties
    std::vector<std::string> value_str;

    // spacing
    bool space_after;
};

#endif

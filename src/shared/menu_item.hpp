/*
 * menu_item.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2025.
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

#include "localization.hpp"

#include "network/byte_buf.hpp"  // coercri

#include <stdexcept>
#include <string>
#include <vector>

struct LocalKeyOrInteger {
    bool is_integer;
    int integer;
    LocalKey local_key;
};

class MenuItem {
public:
    // general properties
    const LocalKey & getTitleKey() const { return title; }
    bool getSpaceAfter() const { return space_after; }
    
    // type (numeric or dropdown)
    // (Note: "numeric" means a field like "Time Limit" where the user enters a number)
    bool isNumeric() const { return numeric; }

    // properties for numeric fields
    int getNumDigits() const { return num_digits; }
    const LocalKey &getSuffix() const { return suffix; }  // e.g. "mins" for Time Limit field

    // properties for dropdown fields
    int getNumChoices() const { return dropdown_entries.size(); }
    const LocalKeyOrInteger & getChoice(int i) const { return dropdown_entries.at(i); }

    
    // construction - dropdown
    MenuItem(const LocalKey &title, const std::vector<LocalKeyOrInteger> &settings)
        : title(title), numeric(false), space_after(false)
    {
        if (settings.empty()) throw std::invalid_argument("MenuItem with no choices!");
        dropdown_entries = settings;
    }

    // construction - numeric
    MenuItem(const LocalKey &title, int num_digits, const LocalKey &suffix)
        : title(title), numeric(true),
          num_digits(num_digits), suffix(suffix),
          space_after(false)
    { }

    // space_after is set after the fact
    void addSpaceAfter() { space_after = true; }

    // serialization
    explicit MenuItem(Coercri::InputByteBuf &buf);
    void serialize(Coercri::OutputByteBuf &buf) const;

private:
    // general properties
    LocalKey title;
    bool numeric;

    // numeric properties
    int num_digits;
    LocalKey suffix;

    // dropdown properties
    std::vector<LocalKeyOrInteger> dropdown_entries;

    // spacing
    bool space_after;
};

#endif

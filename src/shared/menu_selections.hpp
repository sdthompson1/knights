/*
 * menu_selections.hpp
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
 * Represents the currently selected menu settings
 *
 */

#ifndef MENU_SELECTIONS_HPP
#define MENU_SELECTIONS_HPP

#include <map>
#include <string>
#include <vector>

class MenuSelections {
public:
    // Allow direct access to the settings map
    struct Sel {
        int value;
        std::vector<int> allowed_values;
    };
    std::map<std::string, Sel> selections;

    // Also provide functions for more convenient access
    int getValue(const std::string &key) const;
    void setValue(const std::string &key, int new_value);
    void setAllowedValues(const std::string &key, const std::vector<int> &allowed_values);
    std::vector<int> getAllowedValues(const std::string &key) const;
};

#endif

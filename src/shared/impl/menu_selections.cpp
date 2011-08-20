/*
 * menu_selections.cpp
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

#include "misc.hpp"

#include "menu_selections.hpp"

int MenuSelections::getValue(const std::string &key) const
{
    std::map<std::string, Sel>::const_iterator it = selections.find(key);
    if (it == selections.end()) return 0;
    else return it->second.value;
}

void MenuSelections::setValue(const std::string &key, int new_value)
{
    selections[key].value = new_value;
}

void MenuSelections::setAllowedValues(const std::string &key, const std::vector<int> &allowed_values)
{
    selections[key].allowed_values = allowed_values;
}

std::vector<int> MenuSelections::getAllowedValues(const std::string &key) const
{
    std::map<std::string, Sel>::const_iterator it = selections.find(key);
    if (it == selections.end()) return std::vector<int>();
    return it->second.allowed_values;
}

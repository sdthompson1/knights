/*
 * menu_listener.hpp
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
 * This class interfaces to the Lua menu system.
 *
 */

#ifndef MENU_LISTENER_HPP
#define MENU_LISTENER_HPP

#include <string>
#include <vector>

class MenuListener {
public:
    virtual void settingChanged(int item_num,
                                const char *item_key,
                                int choice_num,
                                const char *choice_string,
                                const std::vector<int> &allowed_choices) = 0;

    virtual void questDescriptionChanged(const std::string &new_description) { }
};

#endif

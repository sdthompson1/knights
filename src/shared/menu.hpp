/*
 * menu.hpp
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
 * Quest Selection menu.
 *
 */

#ifndef MENU_HPP
#define MENU_HPP

#include "menu_item.hpp"
#include "localization.hpp"

#include "network/byte_buf.hpp" // coercri

#include <vector>

class Menu {
public:
    Menu() { }

    // get
    int getNumItems() const { return int(items.size()); }
    const MenuItem & getItem(int i) const { return items.at(i); }
    const LocalKey & getTitle() const { return title; }

    // set
    void addItem(const MenuItem &mi) { items.push_back(mi); }
    MenuItem & getItem(int i) { return items.at(i); }
    void setTitle(const LocalKey &t) { title = t; }
    
    // serialization
    explicit Menu(Coercri::InputByteBuf &buf);
    void serialize(Coercri::OutputByteBuf &buf) const;
    
private:
    std::vector<MenuItem> items;
    LocalKey title;
};

#endif

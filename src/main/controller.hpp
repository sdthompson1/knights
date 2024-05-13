/*
 * controller.hpp
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
 * "Controllers" act as an interface between the LocalDungeonView (in
 * particular, LocalDungeonView::getControl) and the input device
 * itself.
 *
 */

#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "map_support.hpp"

#include "gfx/key_code.hpp"

struct ControllerState {
    // Direction
    MapDirection dir;
    bool centred;

    // Old "fire" button
    bool fire;

    // Old Suicide keys
    bool suicide;
};

class Controller {
public:
    virtual ~Controller() { }

    // Read the current state of the controller
    virtual void get(ControllerState &state) const = 0;

    // Should this controller be used in conjunction with mouse + action bar?
    virtual bool usingActionBar() const = 0;
};

#endif

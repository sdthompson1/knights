/*
 * client_config.hpp
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
 * Holds the graphics, anims etc used during a game. These would have
 * been sent to us by the server.
 *
 */

#ifndef CLIENT_CONFIG_HPP
#define CLIENT_CONFIG_HPP

#include "menu.hpp"

#include "boost/noncopyable.hpp"
#include "boost/scoped_ptr.hpp"
#include <vector>

class Anim;
class Graphic;
class Overlay;
class Sound;
class UserControl;

class ClientConfig : boost::noncopyable {
public:
    ClientConfig() : approach_offset(0) { }
    ~ClientConfig();   // calls delete on all the stored ptrs.
    
    std::vector<const Graphic*> graphics;
    std::vector<const Anim*> anims;
    std::vector<const Overlay*> overlays;
    std::vector<const Sound*> sounds;
    std::vector<const UserControl*> standard_controls;
    std::vector<const UserControl*> other_controls;
    boost::scoped_ptr<Menu> menu;
    int approach_offset;
};

#endif

/*
 * client_config.cpp
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

#include "client_config.hpp"

#include "anim.hpp"
#include "user_control.hpp"
#include "graphic.hpp"
#include "overlay.hpp"
#include "sound.hpp"

namespace {
    template<class T>
    void DeleteAll(std::vector<const T*> &x)
    {
        for (typename std::vector<const T*>::iterator it = x.begin(); it != x.end(); ++it) {
            delete *it;
        }
    }
}

ClientConfig::~ClientConfig()
{
    DeleteAll(graphics);
    DeleteAll(anims);
    DeleteAll(overlays);
    DeleteAll(sounds);
    DeleteAll(standard_controls);
    DeleteAll(other_controls);
}

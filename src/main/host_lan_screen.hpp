/*
 * host_lan_screen.hpp
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

#ifndef HOST_LAN_SCREEN_HPP
#define HOST_LAN_SCREEN_HPP

#include "screen.hpp"

class HostLanScreenImpl;

class HostLanScreen : public Screen {
public:
    virtual bool start(KnightsApp &knights_app, boost::shared_ptr<Coercri::Window> window, gcn::Gui &gui);

private:
    boost::shared_ptr<HostLanScreenImpl> pimpl;
};

#endif

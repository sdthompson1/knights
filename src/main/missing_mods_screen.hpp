/*
 * missing_mods_screen.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2026.
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
 * Missing Mods Screen -- shown (instead of the ErrorScreen) when we
 * try to join a game that uses mods we don't have installed. Lists
 * the missing mods (by title, if the online platform can tell us),
 * explains how to install them, and offers a Retry button.
 *
 */

#ifndef MISSING_MODS_SCREEN_HPP
#define MISSING_MODS_SCREEN_HPP

#include "screen.hpp"

#include <string>
#include <vector>

class MissingModsScreenImpl;

class MissingModsScreen : public Screen {
public:
    explicit MissingModsScreen(std::vector<std::string> mod_vfs_names)
        : mod_vfs_names(std::move(mod_vfs_names)) { }
    virtual bool start(KnightsApp &knights_app, boost::shared_ptr<Coercri::Window> window, gcn::Gui &gui);
    virtual void update();

private:
    boost::shared_ptr<MissingModsScreenImpl> pimpl;
    std::vector<std::string> mod_vfs_names;
};

#endif

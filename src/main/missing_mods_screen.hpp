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
 * try to join a game that uses mods we don't have installed.
 *
 * Also used (in UPDATES mode) to download updates for installed
 * Workshop mods (see CheckUpdatesScreen).
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
    // MISSING: mods are missing, so we can't join the game. Exit returns to the
    //   previous menu; once all downloads are complete, we try to join again.
    // UPDATES: Workshop mods need updating. Exit returns to the Mods screen.
    enum Mode { MISSING, UPDATES };

    explicit MissingModsScreen(std::vector<std::string> mod_vfs_names, Mode mode = MISSING)
        : mod_vfs_names(std::move(mod_vfs_names)), mode(mode) { }
    virtual bool start(KnightsApp &knights_app, boost::shared_ptr<Coercri::Window> window, gcn::Gui &gui);
    virtual void update();

private:
    boost::shared_ptr<MissingModsScreenImpl> pimpl;
    std::vector<std::string> mod_vfs_names;
    Mode mode;
};

#endif

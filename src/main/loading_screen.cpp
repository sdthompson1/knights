/*
 * loading_screen.cpp
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

#include "misc.hpp"

#include "knights_app.hpp"
#include "loading_screen.hpp"
#include "x_centre.hpp"

LoadingScreen::LoadingScreen()
    : knights_app(nullptr)
{ }

bool LoadingScreen::start(KnightsApp &ka, boost::shared_ptr<Coercri::Window>, gcn::Gui &)
{
    knights_app = &ka;
    return false;  // GUI not required
}

void LoadingScreen::draw(uint64_t timestamp_us, Coercri::GfxContext &gc)
{
    if (!knights_app) return;
    UTF8String message = knights_app->getLocalization().get(LocalKey("loading"));
    XCentre(gc, *knights_app->getFont(), knights_app->getFont()->getTextHeight() + 15, message);
}

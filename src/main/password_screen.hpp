/*
 * password_screen.hpp
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

#ifndef PASSWORD_SCREEN_HPP
#define PASSWORD_SCREEN_HPP

#include "screen.hpp"

class KnightsClient;
class PasswordScreenImpl;

class PasswordScreen : public Screen {
public:
    explicit PasswordScreen(boost::shared_ptr<KnightsClient> cli, bool first_attempt);
    virtual bool start(KnightsApp &knights_app, boost::shared_ptr<Coercri::Window> window, gcn::Gui &gui);
    
private:
    boost::shared_ptr<PasswordScreenImpl> pimpl;
};

#endif

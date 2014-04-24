/*
 * menu_screen.hpp
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

#ifndef MENU_SCREEN_HPP
#define MENU_SCREEN_HPP

#include "screen.hpp"

#include <string>
#include <vector>

class KnightsClient;
class MenuScreenImpl;

class MenuScreen : public Screen {
public:
    explicit MenuScreen(boost::shared_ptr<KnightsClient> knights_client, bool extended,
                        std::string saved_chat = std::string());
    virtual bool start(KnightsApp &app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui);
    virtual unsigned int getUpdateInterval() { return 50; }
    virtual void update();
    
private:
    boost::shared_ptr<MenuScreenImpl> pimpl;
    boost::shared_ptr<KnightsClient> knights_client;
    bool extended;
    std::string saved_chat;
};

#endif

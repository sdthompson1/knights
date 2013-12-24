/*
 * legacy_action.cpp
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

#include "misc.hpp"

#include "legacy_action.hpp"
#include "my_ctype.hpp"

#include "boost/thread/locks.hpp"


//
// ActionPars
//

MapDirection ActionPars::getMapDirection(int index)
{
    // This is the default if getMapDirection is not separately implemented
    // (It should probably be removed now, and made pure virtual in the base class)
    std::string d = getString(index);
    for (std::string::iterator it = d.begin(); it != d.end(); ++it) { 
        *it = ToUpper(*it);
    }
    if (d == "NORTH") return D_NORTH;
    else if (d == "EAST") return D_EAST;
    else if (d == "SOUTH") return D_SOUTH;
    else if (d == "WEST") return D_WEST;
    error();
    return D_NORTH;
}


//
// ActionMaker
//

boost::mutex g_makers_mutex;

std::map<std::string, const ActionMaker*> & MakersMap()
{
    static boost::shared_ptr<std::map<std::string, const ActionMaker*> > p(
        new std::map<std::string, const ActionMaker*>);
    return *p;
}

ActionMaker::ActionMaker(const std::string &name)
{
    // this is called before main() starts (in a single threaded way)
    // so don't need to lock the mutex. Indeed 'g_makers_mutex' may not
    // have been constructed yet so it would not be safe to do so.
    MakersMap()[name] = this;
}

LegacyAction * ActionMaker::createAction(const std::string &name, ActionPars &pars)
{
    const ActionMaker * maker = 0;
    {
        // We lock 'g_makers_mutex' in case std::map does not support
        // multiple concurrent readers.
        boost::lock_guard<boost::mutex> lock(g_makers_mutex);
        std::map<std::string, const ActionMaker *> & makers = MakersMap();
        std::map<std::string, const ActionMaker *>::iterator it = makers.find(name);
        if (it != makers.end()) maker = it->second;
    }
    if (maker) return maker->make(pars);
    else return 0;
}


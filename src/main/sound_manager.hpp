/*
 * sound_manager.hpp
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

/*
 * Keeps track of all loaded sounds
 * 
 * NOTE: This class is not protected by locks so it should only be
 * accessed by one thread at a time. Currently the only access from
 * outside the main thread comes from the "loader" thread in
 * GameManager. (The game does not start until this thread has
 * exited so this should be OK.)
 *
 */

#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

// coercri
#include "sound/sound_driver.hpp"

#include "boost/shared_ptr.hpp"

#include <map>
#include <string>

class Sound;

class SoundManager {
public:
    explicit SoundManager(boost::shared_ptr<Coercri::SoundDriver> d) : sound_driver(d) { }
    
    void loadSound(const Sound &sound);
    void playSound(const Sound &sound, int frequency);
    void clear();  // unloads all sounds.
    
private:
    boost::shared_ptr<Coercri::SoundDriver> sound_driver;
    std::map<const Sound*, boost::shared_ptr<Coercri::Sound> > sound_map;
};

#endif

/*
 * sound_manager.cpp
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

#include "misc.hpp"

#include "rstream.hpp"
#include "sound.hpp"
#include "sound_manager.hpp"

void SoundManager::loadSound(const Sound &sound)
{
    if (!sound_driver) return;

    if (sound_map.find(&sound) == sound_map.end()) {
        boost::shared_ptr<RStream> str(new RStream(sound.getFilename()));
        sound_map.insert(make_pair(&sound, sound_driver->loadSound(str)));
    }
}

void SoundManager::playSound(const Sound &sound, int frequency)
{
    if (!sound_driver) return;
    
    map<const Sound *, boost::shared_ptr<Coercri::Sound> >::iterator it = sound_map.find(&sound);
    if (it == sound_map.end()) {
        loadSound(sound);
        it = sound_map.find(&sound);
        ASSERT(it != sound_map.end());
    }
    sound_driver->playSound(it->second, frequency);
}

void SoundManager::clear()
{
    sound_map.clear();
}

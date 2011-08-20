/*
 * sound.hpp
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

/*
 * Stores filename for a Sound.
 *
 */

#ifndef SOUND_HPP
#define SOUND_HPP

#include "network/byte_buf.hpp"

#include <string>

class Sound {
public:
    Sound(int id_, const std::string &filename_)
        : id(id_), filename(filename_) { }

    int getID() const { return id; }
    const std::string & getFilename() const { return filename; }

    explicit Sound(int id_, Coercri::InputByteBuf &buf);
    void serialize(Coercri::OutputByteBuf &buf) const;
    
private:
    int id;
    std::string filename;
};

#endif

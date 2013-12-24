/*
 * graphic_element.hpp
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

/*
 * This is used internally by DrawUI. It is also used by EntityMap::draw.
 *
 */

#ifndef GRAPHIC_ELEMENT_HPP
#define GRAPHIC_ELEMENT_HPP

class ColourChange;
class Graphic;

class GraphicElement {
public:
    int sx, sy;
    const Graphic *gr;
    const ColourChange *cc;
    bool semitransparent;
};

class TextElement {
public:
    int sx, sy;  // Centre point of the entity (not necessarily where text should go!)
    std::string text;
};

#endif

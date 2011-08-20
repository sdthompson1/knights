/*
 * load_font.hpp
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

#ifndef LOAD_FONT_HPP
#define LOAD_FONT_HPP

#include "gfx/font.hpp"        // coercri
#include "gfx/ttf_loader.hpp"  // coercri
#include "boost/shared_ptr.hpp"
#include <string>
#include <vector>

boost::shared_ptr<Coercri::Font> LoadFont(Coercri::TTFLoader &loader,
                                          const std::vector<std::string> &ttf_font_names,
                                          const std::vector<std::string> &bmp_font_names, int size);

#endif

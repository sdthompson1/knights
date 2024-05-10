/*
 * load_font.cpp
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

#include "load_font.hpp"
#include "my_exceptions.hpp"  // misc
#include "rstream.hpp"        // rstream

#include "gfx/bitmap_font.hpp"      // coercri
#include "gfx/load_bmp.hpp"         // coercri
#include "gfx/load_system_ttf.hpp"  // coercri

boost::shared_ptr<Coercri::Font> LoadFont(boost::shared_ptr<Coercri::GfxDriver> driver,
                                          Coercri::TTFLoader &ttf_loader,
                                          const std::vector<std::string> &ttf_font_names,
                                          const std::vector<std::string> &bitmap_font_names, int size)
{
    try {
        if (!ttf_font_names.empty()) {
            return Coercri::LoadSystemTTF(ttf_loader, ttf_font_names, size);
        }
    } catch (...) { }
    
    for (std::vector<std::string>::const_iterator it = bitmap_font_names.begin(); it != bitmap_font_names.end(); ++it) {
        try {
            RStream str(*it);
            boost::shared_ptr<Coercri::PixelArray> pix = Coercri::LoadBMP(str);
            boost::shared_ptr<Coercri::Font> result(new Coercri::BitmapFont(driver, pix));
            return result;
        } catch (...) { }
    }

    throw GraphicLoadFailed("Could not load font! Check fonts.txt");
}

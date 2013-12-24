/*
 * gfx_manager.cpp
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

#include "file_cache.hpp"
#include "gfx_manager.hpp"
#include "graphic.hpp"
#include "graphic_transform.hpp"
#include "load_font.hpp"
#include "my_exceptions.hpp"

#include "gfx/load_bmp.hpp"  // coercri

GfxManager::GfxManager(boost::shared_ptr<Coercri::GfxDriver> gfx_driver_,
                       boost::shared_ptr<Coercri::TTFLoader> ttf_loader_,
                       const std::vector<std::string> &ttf_font_names_,
                       const std::vector<std::string> &bmp_font_names_,
                       unsigned char invis_alpha_,
                       FileCache &fc)
    : gfx_driver(gfx_driver_),
      ttf_loader(ttf_loader_),
      file_cache(fc),
      ttf_font_names(ttf_font_names_),
      bmp_font_names(bmp_font_names_),
      font_size(0),
      npix(0),
      invis_alpha(invis_alpha_)
{
}

GfxManager::~GfxManager()
{
    deleteAllGraphics();
}

boost::shared_ptr<Coercri::Font> GfxManager::getFont()
{
    return font;
}

void GfxManager::setFontSize(int new_size)
{
    if (font_size == new_size) return;
    font = LoadFont(*ttf_loader, ttf_font_names, bmp_font_names, new_size);
    font_size = new_size;
}  



bool GfxManager::loadGraphic(const Graphic &gfx, bool permanent)
{
    // Note: We don't (yet) bother to optimize the case where the same filename,
    // but a different hx/hy/r/g/b is being loaded. The BMP file is loaded twice in that case.
    GfxMap::const_iterator it = gfx_map.find(&gfx);
    if (it == gfx_map.end()) {
        // Open the file. Only BMP format supported for now.
        boost::shared_ptr<std::istream> str = file_cache.openFile(gfx.getFileInfo());
        if (!str) return false; // Not in cache!
        
        boost::shared_ptr<const Coercri::PixelArray> pixels = Coercri::LoadBMP(*str);

        // Apply colour key if necessary
        if (gfx.getR() != -1) {
            ColourChange cc;
            cc.add(Colour(gfx.getR(), gfx.getG(), gfx.getB(), 255), 
                   Colour(gfx.getR(), gfx.getG(), gfx.getB(), 0));   // make it transparent
            pixels = CreateGraphicWithCC(pixels, cc);
        }
        
        // Create the graphic
        boost::shared_ptr<Coercri::Graphic> new_graphic
            = gfx_driver->createGraphic(pixels, gfx.getHX(), gfx.getHY());
        
        // Add to the map
        GfxPtrWithFlag g;
        g.gfx = new_graphic;
        g.permanent = permanent;
        gfx_map.insert(GfxMap::value_type(&gfx, g));
    }

    return true;
}

void GfxManager::deleteAllGraphics()
{
    // Removing entries from gfx_map will release the shared_ptr
    // references to the graphics and therefore delete them
    for (GfxMap::iterator it = gfx_map.begin(); it != gfx_map.end(); ) {
        GfxMap::iterator to_be_deleted = it++;
        if (!to_be_deleted->second.permanent) {
            gfx_map.erase(to_be_deleted);
        }
    }

    // We also need to delete any cached transformed-graphics
    cached_gfx_map.clear();
    recent_list.clear();
    npix = 0;
}

void GfxManager::getGraphicSize(const Graphic &gfx, int &width, int &height) const
{
    const Coercri::Graphic &cg = getCoercriGraphic(gfx);
    width = cg.getWidth();
    height = cg.getHeight();
}

void GfxManager::setGfxResizer(boost::shared_ptr<GfxResizer> resizer)
{
    gfx_resizer = resizer;
    // We need to delete any cached graphics, as they may not be valid with the new resizer
    cached_gfx_map.clear();
    recent_list.clear();
    npix = 0;
}

boost::shared_ptr<GfxResizer> GfxManager::getGfxResizer()
{
    return gfx_resizer;
}

boost::shared_ptr<Coercri::Graphic> GfxManager::createGraphic(const GraphicKey &key)
{
    // NOTE: This routine should be called with either a resize or a colour-change,
    // but not both at the same time.
    // (Semitransparency counts as part of the colour-change.)
    boost::shared_ptr<const Coercri::PixelArray> new_pixels;
    int new_hx, new_hy;
    key.original->getHandle(new_hx, new_hy);
    
    if (key.cc.empty() && !key.semitransparent) {
        // Resize only
        ASSERT(key.new_width != key.original->getWidth() || key.new_height != key.original->getHeight());

        // Call the resizing routine
        new_pixels = CreateResizedGraphic(*gfx_resizer, key.original->getPixels(), key.new_width, key.new_height,
                                          new_hx, new_hy, new_hx, new_hy);

    } else {
        // Colour change only
        ASSERT(key.new_width == key.original->getWidth() && key.new_height == key.original->getHeight());

        // Call the colour-changing routine
        new_pixels = CreateGraphicWithCC(key.original->getPixels(), key.cc, 
            key.semitransparent ? invis_alpha : 255);
    }
    
    boost::shared_ptr<Coercri::Graphic> new_graphic(gfx_driver->createGraphic(new_pixels, new_hx, new_hy));
    return new_graphic;
}

const Coercri::Graphic & GfxManager::getGraphic(const GraphicKey &key)
{
    CachedGfxMap::iterator it = cached_gfx_map.find(key);

    if (it != cached_gfx_map.end()) {
        // It's in the cache. Bump it to the front of the MRU list
        recent_list.splice(recent_list.begin(), recent_list, it->second.list_position);
        return *it->second.new_graphic;

    } else {
        // Not in cache. Need to create a new graphic
        boost::shared_ptr<Coercri::Graphic> new_graphic = createGraphic(key);

        // Add to the RecentList (at the front)
        recent_list.push_front(key);

        // Add to the main cache
        GraphicData gd;
        gd.new_graphic = new_graphic;
        gd.list_position = recent_list.begin();
        cached_gfx_map.insert(make_pair(key, gd));

        // Increase npix
        npix += key.new_width * key.new_height;

        // Delete old graphics if necessary
        deleteOld();

        // Return the new graphic
        return *new_graphic;
    }
}

bool GfxManager::tooBig() const
{
    // NOTE: minimum_number should be at least 2, otherwise getResizedGraphicWithCC
    // may fail (because its intermediate "gfx_cc" may get deleted before we can use it!).
    
    const int minimum_number = 5;
    const int maximum_number = 1000;
    const int maximum_pixels = 1000000;

    const int current_number = cached_gfx_map.size();

    if (current_number <= minimum_number) return false;
    if (current_number > maximum_number) return true;
    if (npix > maximum_pixels) return true;
    return false;
}

void GfxManager::deleteOld()
{
    // Delete old graphic(s) if the cache is too big.
    while (tooBig()) {
        ASSERT(!recent_list.empty());

        // Find the least recently accessed graphic in the CachedGfxMap. We are going to remove it.
        CachedGfxMap::iterator it = cached_gfx_map.find(recent_list.back());
        ASSERT(it != cached_gfx_map.end());
        
        // Reduce npix accordingly
        npix -= it->first.new_width * it->first.new_height;

        // Remove it from CachedGfxMap and RecentList
        // (This will also delete the graphic, since the shared_ptr will be released)
        cached_gfx_map.erase(it);
        recent_list.pop_back();
    }
}

const Coercri::Graphic & GfxManager::getGraphicWithCC(const Coercri::Graphic &original, const ColourChange &cc, bool semitransparent)
{
    // Short cut:
    if (cc.empty() && !semitransparent) return original;

    // Look it up in cache (creating if necessary):
    GraphicKey key;
    key.original = &original;
    key.cc = cc;
    key.semitransparent = semitransparent;
    key.new_width = original.getWidth();
    key.new_height = original.getHeight();
    return getGraphic(key);
}

const Coercri::Graphic & GfxManager::getResizedGraphic(const Coercri::Graphic &original, int new_width, int new_height)
{
    // Short cut:
    if (new_width == original.getWidth() && new_height == original.getHeight()) return original;

    // Look it up in cache (creating if necessary):
    GraphicKey key;
    key.original = &original;
    key.cc = ColourChange();
    key.semitransparent = false;
    key.new_width = new_width;
    key.new_height = new_height;
    return getGraphic(key);
}

const Coercri::Graphic & GfxManager::getCoercriGraphic(const Graphic &original) const
{
    GfxMap::const_iterator it = gfx_map.find(&original);
    if (it == gfx_map.end()) throw UnexpectedError("Graphic has not been loaded");
    else return *(it->second.gfx);
}

void GfxManager::drawGraphic(Coercri::GfxContext &gc, int x, int y, const Graphic &gfx)
{
    const Coercri::Graphic & gfx_original = getCoercriGraphic(gfx);

    if (gfx.getColourChange()) {
        const Coercri::Graphic & gfx_cc = getGraphicWithCC(gfx_original, *gfx.getColourChange(), false);
        gc.drawGraphic(x, y, gfx_cc);
    } else {
        gc.drawGraphic(x, y, gfx_original);
    }
}

void GfxManager::drawTransformedGraphic(Coercri::GfxContext &gc, int x, int y,
                                        const Graphic &gfx, int new_width, int new_height)
{
    if (new_width == 0 || new_height == 0) return;   // Drawing zero-sized graphic; effectively a no-op

    const Coercri::Graphic & gfx_original = getCoercriGraphic(gfx);

    const Coercri::Graphic * gfx_cc = &gfx_original;
    if (gfx.getColourChange()) {
        gfx_cc = &getGraphicWithCC(gfx_original, *gfx.getColourChange(), false);
    }

    const Coercri::Graphic & gfx_cc_resized = getResizedGraphic(*gfx_cc, new_width, new_height);
    gc.drawGraphic(x, y, gfx_cc_resized);
}

void GfxManager::drawTransformedGraphic(Coercri::GfxContext &gc, int x, int y,
                                        const Graphic &gfx, int new_width, int new_height,
                                        const ColourChange &cc, bool semitransparent)
{
    if (new_width == 0 || new_height == 0) return;   // Drawing zero-sized graphic; effectively a no-op

    const Coercri::Graphic & gfx_original = getCoercriGraphic(gfx);
    const Coercri::Graphic & gfx_cc = getGraphicWithCC(gfx_original, cc, semitransparent);
    const Coercri::Graphic & gfx_cc_resized = getResizedGraphic(gfx_cc, new_width, new_height);
    gc.drawGraphic(x, y, gfx_cc_resized);
}

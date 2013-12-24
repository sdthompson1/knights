/*
 * gfx_manager.hpp
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
 * Provides interface for rendering a Graphic onto the screen
 * (internally, converts Graphics into Coercri::Graphic). Also
 * provides additional functionality including resizing and
 * recolouring.
 *
 * NOTE: This class is not protected by locks so it should only be
 * accessed by one thread at a time. Currently the only access from
 * outside the main thread comes from the "loader" thread in
 * GameManager. (The game does not start until this thread has
 * exited so this should be OK.)
 * 
 */

#ifndef GFX_MANAGER_HPP
#define GFX_MANAGER_HPP

#include "colour_change.hpp"

// coercri
#include "gfx/font.hpp"
#include "gfx/gfx_context.hpp"
#include "gfx/gfx_driver.hpp"
#include "gfx/graphic.hpp"
#include "gfx/ttf_loader.hpp"

#include "boost/shared_ptr.hpp"

#include <list>
#include <map>
using namespace std;

class ColourChange;
class FileCache;
class GfxResizer;
class Graphic;

class GfxManager {
public:
    GfxManager(boost::shared_ptr<Coercri::GfxDriver> gfx_driver_,
               boost::shared_ptr<Coercri::TTFLoader> ttf_loader_,
               const vector<string> &ttf_font_names_,
               const vector<string> &bmp_font_names_,
               unsigned char invis_alpha_,
               FileCache &fc);
    ~GfxManager();
    
    // Fonts and text

    boost::shared_ptr<Coercri::Font> getFont();
    void setFontSize(int new_size);

    // Graphics

    bool loadGraphic(const Graphic &gfx, bool permanent = false);
    void deleteAllGraphics();  // doesn't delete the "permanent" ones.
    
    void getGraphicSize(const Graphic &gfx, int &width, int &height) const;
    
    void setGfxResizer(boost::shared_ptr<GfxResizer> resizer);
    boost::shared_ptr<GfxResizer> getGfxResizer();
    void drawGraphic(Coercri::GfxContext &gc, int x, int y, const Graphic &gfx);
    void drawTransformedGraphic(Coercri::GfxContext &gc, int x, int y,
                                const Graphic &gfx, int new_width, int new_height);
    void drawTransformedGraphic(Coercri::GfxContext &gc, int x, int y,
                                const Graphic &gfx, int new_width, int new_height,
                                const ColourChange &cc, 
                                bool semitransparent = false); // used for drawing invisible teammates

private:
    // typedefs, structs

    struct GfxPtrWithFlag {
        boost::shared_ptr<Coercri::Graphic> gfx;
        bool permanent;
    };
    typedef map<const Graphic *, GfxPtrWithFlag> GfxMap;

    struct GraphicKey {
        const Coercri::Graphic * original;
        ColourChange cc;
        bool semitransparent;
        int new_width, new_height;

        bool operator<(const GraphicKey &rhs) const
        {
            return
                original < rhs.original ? true :
                rhs.original < original ? false :
                cc < rhs.cc ? true :
                rhs.cc < cc ? false :
                semitransparent < rhs.semitransparent ? true :
                semitransparent > rhs.semitransparent ? false :
                new_width < rhs.new_width ? true :
                rhs.new_width < new_width ? false :
                new_height < rhs.new_height;
        }
    };

    typedef list<GraphicKey> RecentList;  // MRU at front, LRU at back

    struct GraphicData {
        boost::shared_ptr<Coercri::Graphic> new_graphic;
        RecentList::iterator list_position;
    };

    typedef map<GraphicKey, GraphicData> CachedGfxMap;
        
private:
    // Private methods
    boost::shared_ptr<Coercri::Graphic> createGraphic(const GraphicKey &key);
    const Coercri::Graphic & getGraphic(const GraphicKey &key);
    bool tooBig() const;
    void deleteOld();
    const Coercri::Graphic & getGraphicWithCC(const Coercri::Graphic &original, const ColourChange &cc, bool semitransparent);
    const Coercri::Graphic & getResizedGraphic(const Coercri::Graphic &original, int new_width, int new_height);
    const Coercri::Graphic & getCoercriGraphic(const Graphic &gfx) const;
    
private:
    // Data
    boost::shared_ptr<Coercri::GfxDriver> gfx_driver;
    boost::shared_ptr<GfxResizer> gfx_resizer;
    FileCache &file_cache;
    
    // Font data
    boost::shared_ptr<Coercri::TTFLoader> ttf_loader;
    vector<string> ttf_font_names, bmp_font_names;
    int font_size;
    boost::shared_ptr<Coercri::Font> font;

    // Loaded Graphics
    GfxMap gfx_map;

    // Caches for transformed graphics
    CachedGfxMap cached_gfx_map;
    RecentList recent_list;
    int npix;

    // Cached config value
    unsigned char invis_alpha;
};

#endif

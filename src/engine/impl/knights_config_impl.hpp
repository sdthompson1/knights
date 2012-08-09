/*
 * knights_config_impl.hpp
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

#ifndef KNIGHTS_CONFIG_IMPL_HPP
#define KNIGHTS_CONFIG_IMPL_HPP

#include "colour_change.hpp"
#include "item_type.hpp"  // needed for ItemSize
#include "lua_func.hpp"
#include "map_support.hpp"
#include "overlay.hpp"       // needed for N_OVERLAY_FRAME

// coercri
#include "gfx/color.hpp"

#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"
#include <map>
#include <string>
#include <vector>

class Anim;
class ConfigMap;
class Control;
class CoordTransform;
class DungeonGenerator;
class DungeonMap;
class EventManager;
class FileInfo;
class GoreManager;
class Graphic;
class HomeManager;
class ItemType;
class Menu;
class MenuWrapper;
class MonsterManager;
class MonsterType;
class Player;
class Segment;
class Sound;
class StuffManager;
class TaskManager;
class Tile;
class TutorialManager;
class UserControl;

class KnightsConfigImpl {
public:

    //
    // ctor, dtor
    //

    KnightsConfigImpl(const std::string &config_filename, bool menu_strict);
    ~KnightsConfigImpl();


    //
    // stuff forwarded from KnightsConfig
    //

    void getAnims(std::vector<const Anim*> &anims) const;
    void getGraphics(std::vector<const Graphic*> &graphics) const;
    void getOverlays(std::vector<const Overlay*> &overlays) const;
    void getSounds(std::vector<const Sound*> &sounds) const;
    void getStandardControls(std::vector<const UserControl*> &controls) const;
    void getOtherControls(std::vector<const UserControl*> &controls) const;
    void initializeGame(HomeManager &home_manager,
                        std::vector<boost::shared_ptr<Player> > &players,
                        StuffManager &stuff_manager,
                        GoreManager &gore_manager,
                        MonsterManager &monster_manager,
                        EventManager &event_manager,
                        TaskManager &task_manager,
                        const std::vector<int> &hse_cols,
                        const std::vector<std::string> &player_names,
                        TutorialManager *tutorial_manager);
    const Menu & getMenu() const;
    MenuWrapper & getMenuWrapper() { return *menu_wrapper; }
    void resetMenu();
    int getApproachOffset() const;
    void getHouseColours(std::vector<Coercri::Color> &result) const;
    boost::shared_ptr<const ConfigMap> getConfigMap() const { return config_map; }
    boost::shared_ptr<lua_State> getLuaState() { return lua_state; }

    // This returns true when we are in the "config" stage (as opposed to "in-game")
    bool doingConfig() const { return doing_config; }

    
    //
    // Add objects (graphics, sounds etc) that have been created by Lua.
    // (These are stored on the Lua side as raw pointers but the KnightConfig must also be
    // informed, both so that it can delete the objects at the end, and also so that it
    // can send the objects to network players when they join the game.)
    //
    // NOTE: These can only be called when doingConfig() is true.
    //

    Anim * addLuaAnim(lua_State *lua, int idx);
    Control * addLuaControl(auto_ptr<Control> p);  // first NUM_STANDARD_CONTROLS assumed to be the std ctrls
    void addLuaGraphic(auto_ptr<Graphic> p);
    ItemType * addLuaItemType(auto_ptr<ItemType> p);
    MonsterType * addLuaMonsterType(auto_ptr<MonsterType> p,
                                    std::vector<boost::shared_ptr<Tile> > &corpse_tiles);
    Overlay * addLuaOverlay(auto_ptr<Overlay> p);
    Segment * addLuaSegment(auto_ptr<Segment> p);
    Sound * addLuaSound(const FileInfo &fi);  // creates the sound and adds it.

    void setOverlayOffsets(lua_State *lua); // reads args from lua indices 1,2,3...
    
private:
    //
    // helper functions
    //

    void freeMemory();
    shared_ptr<Tile> makeDeadKnightTile(boost::shared_ptr<Tile>, const ColourChange &);

    void popHouseColours(lua_State *lua);
    void popHouseColoursMenu(lua_State *lua);
    Colour popRGB(lua_State *lua);
    void popTutorial(lua_State *lua);
    
private:
    // The lua state. This stays alive between games (it is only destroyed when ~KnightsConfigImpl is called).
    // This should be the last thing destroyed, so is listed first in the class.
    boost::shared_ptr<lua_State> lua_state;

    // True during ctor, false otherwise
    bool doing_config;

    // Storage for lua-created game objects. Will be deleted by ~KnightsConfigImpl.
    std::vector<Anim *> lua_anims;
    std::vector<Control *> lua_controls;
    std::vector<Graphic *> lua_graphics;
    std::vector<ItemType *> lua_item_types;
    std::vector<MonsterType *> lua_monster_types;
    std::vector<Overlay *> lua_overlays;
    std::vector<Segment *> lua_segments;
    std::vector<Sound *> lua_sounds;
    std::vector<ItemType *> special_item_types;  // used for loaded crossbows
    
    // Menu
    std::auto_ptr<MenuWrapper> menu_wrapper;

    // Various things
    std::vector<ColourChange> house_colours_normal, house_colours_invulnerable;  // One entry per House
    std::vector<Coercri::Color> house_col_vector;
    std::vector<boost::shared_ptr<ColourChange> > secured_cc;
    Anim * knight_anim;   // In colours of house 0, ie house_colours[0] is always 'empty'.
    std::vector<Anim *> knight_anims;   // copies of 'knight_anim' with appropriate colour changes
    ItemType * default_item;
    std::vector<const Control *> control_set;
    const Graphic *stuff_bag_graphic;

    // Gore (blood & knight/monster corpses)
    std::map<MonsterType *, std::vector<shared_ptr<Tile> > > monster_corpse_tiles;
    const Graphic *blood_icon;
    std::vector<boost::shared_ptr<Tile> > blood_tiles, dead_knight_tiles;
    
    // The generic hook system
    std::map<std::string, LuaFunc> hooks;

    // Mapping from tile category names to numbers
    std::map<std::string, int> tile_categories;
    
    // Config Map.
    boost::shared_ptr<ConfigMap> config_map;

    // Overlay offsets
    struct OverlayData {
        int ofsx, ofsy;
        MapDirection dir;
    };
    OverlayData overlay_offsets[Overlay::N_OVERLAY_FRAME][4];    

    // extra graphics for the dead knight tiles. added 30-May-2009
    std::vector<boost::shared_ptr<Graphic> > dead_knight_graphics;

    // Tutorial
    std::map<int, std::pair<std::string,std::string> > tutorial_data;
};

#endif

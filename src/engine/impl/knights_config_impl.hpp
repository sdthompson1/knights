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
#include "config_map.hpp"
#include "item_type.hpp"
#include "map_support.hpp"
#include "menu.hpp"
#include "menu_constraints.hpp"
#include "overlay.hpp"
#include "segment_set.hpp"

// kfile
#include "kfile.hpp"

// coercri
#include "gfx/color.hpp"

#include "boost/scoped_ptr.hpp"
#include "boost/shared_ptr.hpp"
#include <string>
#include <vector>

class Action;
class Anim;
class ColourChange;
class Control;
class CoordTransform;
class DungeonDirective;
class DungeonGenerator;
class EventManager;
class GoreManager;
class Graphic;
class HomeManager;
class ItemGenerator;
class ItemType;
class MenuInt;
class MenuItem;
class MenuSelections;
class MonsterManager;
class MonsterType;
class Player;
class Quest;
class RandomDungeonLayout;
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

    explicit KnightsConfigImpl(const std::string &config_filename);
    ~KnightsConfigImpl();


    //
    // types used below
    //

    struct SegmentTileData {
        std::vector<boost::shared_ptr<Tile> > tiles;
        std::vector<const ItemType*> items;
        std::vector<const MonsterType*> monsters;
    };


    //
    // stuff forwarded from KnightsConfig
    //

    void getAnims(std::vector<const Anim*> &anims) const;
    void getGraphics(std::vector<const Graphic*> &graphics) const;
    void getOverlays(std::vector<const Overlay*> &overlays) const;
    void getSounds(std::vector<const Sound*> &sounds) const;
    void getStandardControls(std::vector<const UserControl*> &controls) const;
    void getOtherControls(std::vector<const UserControl*> &controls) const;
    std::string initializeGame(const MenuSelections &msel,
                               boost::shared_ptr<DungeonMap> &dungeon_map,
                               boost::shared_ptr<CoordTransform> &coord_transform,
                               std::vector<boost::shared_ptr<Quest> > &quests,
                               HomeManager &home_manager,
                               std::vector<boost::shared_ptr<Player> > &players,
                               StuffManager &stuff_manager,
                               GoreManager &gore_manager,
                               MonsterManager &monster_manager,
                               EventManager &event_manager,
                               bool &premapped,
                               std::vector<std::pair<const ItemType *, std::vector<int> > > &starting_gears,
                               TaskManager &task_manager,
                               const std::vector<int> &hse_cols,
                               const std::vector<std::string> &player_names,
                               TutorialManager *tutorial_manager,
                               int &final_gvt) const;
    const Menu & getMenu() const { return menu; }
    const MenuConstraints & getMenuConstraints() const { return menu_constraints; }
    int getApproachOffset() const;
    void getHouseColours(std::vector<Coercri::Color> &result) const;
    std::string getQuestDescription(int quest_num, const std::string &exit_point_string) const;
    boost::shared_ptr<const ConfigMap> getConfigMap() const { return config_map; }
    boost::shared_ptr<lua_State> getLuaState() { return lua_state; }


    // This returns true when we are in the "config" stage (as opposed to "in-game")
    bool doingConfig() const { return kf; }

    
    //
    // Add additional objects (graphics, sounds etc) that have been created by Lua.
    // (These are stored on the Lua side as raw pointers but the KnightConfig must also be
    // informed, both so that it can delete the objects at the end, and also so that it
    // can send the objects to network players when they join the game.)
    //
    // NOTE: These can only be called when doingConfig() is true.
    //

    Action * addLuaAction(auto_ptr<Action> p);
    Anim * addLuaAnim(lua_State *lua, int idx);
    Control * addLuaControl(auto_ptr<Control> p);
    RandomDungeonLayout * addLuaDungeonLayout(lua_State *lua); // reads from args 1 and 2.
    void addLuaGraphic(auto_ptr<Graphic> p);
    ItemGenerator * addLuaItemGenerator(lua_State *lua);
    ItemType * addLuaItemType(auto_ptr<ItemType> p);
    Overlay * addLuaOverlay(auto_ptr<Overlay> p);
    Sound * addLuaSound(const char *name);  // creates the sound and adds it.

    void setOverlayOffsets(lua_State *lua); // reads args from lua indices 1,2,3...
    
    
    //
    // KConfig references -- each function pushes a userdata to the Lua stack.
    //

    void kconfigAnim(const char *name);
    void kconfigItemType(const char *name);
    void kconfigTile(const char *name);
    void kconfigControl(const char *name);

    
    //
    // Interface to KFile
    //

    KConfig::KFile * getKFile() { return kf.get(); }  // NULL if file not opened

    MapAccess popAccessCode(MapAccess dflt);
    void popAccessTable(MapAccess out[], MapAccess dflt);
    Action * popAction();
    Action * popAction(Action *dflt);
    Anim * popAnim();
    Anim * popAnim(Anim *dflt);
    bool popBool();
    bool popBool(bool dflt);
    Control * popControl();
    Control * popControl(Control *dflt);
    void popControlSet(std::vector<const Control*> &which_control_set);
    DungeonDirective * popDungeonDirective();
    Graphic * popGraphic();
    Graphic * popGraphic(Graphic *dflt);
    void popHouseColours(const ConfigMap &config_map);
    void popHouseColoursMenu();
    ItemGenerator * popItemGenerator();
    ItemSize popItemSize();
    ItemSize popItemSize(ItemSize dflt);
    const ItemType * popItemType();
    const ItemType * popItemType(const ItemType *dflt);
    MapDirection popMapDirection();
    MapDirection popMapDirection(MapDirection dflt);
    void popMenu();
    void popMenuSpace();
    MenuItem popMenuItem();
    MenuInt * popMenuInt();
    void popMenuValue(int val, MenuItem &menu_item);
    string popMenuValueName(int val);
    void popMenuValueDirective(const string &key, int value);
    bool isMonsterType();
    MonsterType * popMonsterType();
    Overlay * popOverlay();
    Overlay * popOverlay(Overlay *dflt);
    void popPotionRenderer();
    float popProbability();  // Read as integer, then divided by 100. Error if result <0 or >1.
    float popProbability(int dflt);  // The 'dflt' is out of 100.
    void popQuestDescriptions();
    RandomDungeonLayout * popRandomDungeonLayout();
    Colour popRGB();
    Segment * popSegment(const std::vector<SegmentTileData> &tile_map, int &category);
    Segment * popSegmentData(const std::vector<SegmentTileData> &tile_map, int w, int h);
    void popSegmentList(const std::vector<SegmentTileData> &tile_map);
    void popSegmentRooms(Segment &);
    void popSegmentSet();
    void popSegmentSwitches(Segment &);
    void popSegmentTiles(std::vector<SegmentTileData> &tile_map);
    void popSegmentTile(SegmentTileData &seg_tile, bool &items_yet, bool &monsters_yet);
    void popSkullRenderer();
    Sound * popSound();
    boost::shared_ptr<Tile> popTile();
    boost::shared_ptr<Tile> popTile(boost::shared_ptr<Tile> dflt);
    void popTileList(std::vector<boost::shared_ptr<Tile> > &output);
    void popTutorial();
    void popZombieActivityTable();

    int getSegmentCategory(const std::string &); // returns -1 if empty string given
    int getTileCategory(const std::string &);    // ditto
    
    KConfig::RandomIntContainer & getRandomIntContainer() { return random_ints; }

private:
    //
    // helper functions
    //

    void processTile(const std::vector<SegmentTileData> &tmap, Segment &seg, int x, int y);
    void readMenu(const Menu &menu, const MenuSelections &msel, DungeonGenerator &dgen,
                  std::vector<boost::shared_ptr<Quest> > &quests) const;
    const SegmentTileData & tileFromInt(const std::vector<SegmentTileData> &tile_map, int t);
    shared_ptr<Tile> makeDeadKnightTile(boost::shared_ptr<Tile>, const ColourChange &);

    void * doLuaCast(unsigned int type_tag, void *ud) const;
    

    //
    // helper classes
    //

    class Sentry {
    public:
        Sentry(KnightsConfigImpl &c) : cfg(c) { }
        ~Sentry();
        KnightsConfigImpl &cfg;
    };
    friend class Sentry;

    struct Popper {
        Popper(KConfig::KFile &k) : kf(k) { }
        ~Popper();
        KConfig::KFile &kf;
    };

    
private:
    // The lua state. This stays alive between games (it is only destroyed when ~KnightsConfigImpl is called).
    // This should be the last thing destroyed, so is listed first in the class.
    boost::shared_ptr<lua_State> lua_state;


    // Storage for all 'basic' game objects, i.e. stuff loaded directly from the config file.
    // (Deleted by ~KnightsConfigImpl.)

    std::map<const Value *, Action *> actions;
    std::map<const Value *, Control *> controls;
    std::map<const Value *, DungeonDirective *> dungeon_directives;
    std::map<const Value *, const ItemType *> item_types;
    std::vector<ItemType *> special_item_types;
    std::map<const Value *, MenuInt *> menu_ints;
    std::map<const Value *, MonsterType *> monster_types;
    std::map<const Value *, Overlay *> overlays;
    std::map<const Value *, Segment *> segments;
    std::map<const Value *, boost::shared_ptr<Tile> > tiles;
    SegmentSet segment_set;
    KConfig::RandomIntContainer random_ints;

    // Storage for lua-created game objects. Will be deleted by ~KnightsConfigImpl.
    std::vector<Action *> lua_actions;
    std::vector<Anim *> lua_anims;
    std::vector<Control *> lua_controls;
    std::vector<RandomDungeonLayout *> lua_dungeon_layouts;
    std::vector<Graphic *> lua_graphics;
    std::vector<ItemGenerator *> lua_item_generators;
    std::vector<ItemType *> lua_item_types;
    std::vector<Overlay *> lua_overlays;
    std::vector<Sound *> lua_sounds;
    
    // Menu
    Menu menu;
    MenuConstraints menu_constraints;
    std::vector<DungeonDirective*> global_dungeon_directives;
    typedef std::map<int, std::vector<DungeonDirective*> > val_directives_map;
    typedef std::map<std::string, val_directives_map> key_val_directives_map;
    key_val_directives_map menu_dungeon_directives;
    typedef std::map<int, std::vector<boost::shared_ptr<Quest> > > val_quests_map;
    typedef std::map<std::string, val_quests_map> key_val_quests_map;
    key_val_quests_map menu_quests;

    // Various things
    boost::shared_ptr<Tile> wall_tile, horiz_door_tile[2], vert_door_tile[2];
    std::vector<ColourChange> house_colours_normal, house_colours_invulnerable;  // One entry per House
    std::vector<Coercri::Color> house_col_vector;
    std::vector<boost::shared_ptr<ColourChange> > secured_cc;
    Anim * knight_anim;   // In colours of house 0, ie house_colours[0] is always 'empty'.
    std::vector<Anim *> knight_anims;   // copies of 'knight_anim' with appropriate colour changes
    const ItemType * default_item;
    std::vector<const Control *> control_set;
    const Graphic *stuff_bag_graphic;
    std::vector<std::string> standard_quest_descriptions;

    // Monsters, gore

    std::map<MonsterType *, std::vector<shared_ptr<Tile> > > monster_corpse_tiles;
    std::map<MonsterType *, std::vector<shared_ptr<Tile> > > monster_generator_tiles;

    struct ZombieActivityEntry {
        shared_ptr<Tile> from;
        shared_ptr<Tile> to_tile;
        const MonsterType * to_monster_type;
    };
    std::vector<ZombieActivityEntry> zombie_activity;
    static void addZombieActivity(MonsterManager &mm, shared_ptr<Tile> from, const ZombieActivityEntry &ze);
    
    const Graphic *blood_icon;
    std::vector<boost::shared_ptr<Tile> > blood_tiles, dead_knight_tiles;
    
    MonsterType *vampire_bat_type; // Will eventually get rid of these (hopefully...)


    // The generic hook system
    std::map<std::string, const Action *> hooks;

    // Some maps from names (in the config system) to integer id codes.
    // These are non-empty only during loading of the config file.
    std::map<std::string, int> menu_item_names;
    std::map<std::string, int> segment_categories;
    std::map<std::string, int> tile_categories;
    
    // Config Map.
    boost::shared_ptr<ConfigMap> config_map;

    // Overlay offsets
    struct OverlayData {
        int ofsx, ofsy;
        MapDirection dir;
    };
    OverlayData overlay_offsets[Overlay::N_OVERLAY_FRAME][4];
    
    // kf is non-null only during the constructor.
    boost::shared_ptr<KConfig::KFile> kf;
    

    // extra graphics for the dead knight tiles. added 30-May-2009
    std::vector<boost::shared_ptr<Graphic> > dead_knight_graphics;

    // Tutorial
    std::map<int, std::pair<std::string,std::string> > tutorial_data;
};

#endif

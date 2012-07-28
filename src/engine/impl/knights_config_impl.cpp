/*
 * knights_config_impl.cpp
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

#include "anim.hpp"
#include "config_map.hpp"
#include "control.hpp"
#include "control_actions.hpp"     // for AddStandardControls
#include "coord_transform.hpp"
#include "dungeon_map.hpp"
#include "event_manager.hpp"
#include "gore_manager.hpp"
#include "graphic.hpp"
#include "home_manager.hpp"
#include "item_check_task.hpp"
#include "knights_config_impl.hpp"
#include "lua_ingame.hpp"
#include "lua_exec.hpp"
#include "lua_load_from_rstream.hpp"
#include "lua_sandbox.hpp"
#include "lua_setup.hpp"
#include "lua_userdata.hpp"
#include "menu_wrapper.hpp"
#include "monster_manager.hpp"
#include "monster_task.hpp"
#include "monster_type.hpp"
#include "my_exceptions.hpp"
#include "player.hpp"
#include "rng.hpp"
#include "rstream.hpp"
#include "segment.hpp"
#include "sound.hpp"
#include "stuff_bag.hpp"
#include "task_manager.hpp"
#include "tile.hpp"
#include "tutorial_manager.hpp"

#include "lua.hpp"

#include <sstream>

#ifdef __LP64__
#include <stdint.h>
#define int_pointer_type intptr_t
#else
#define int_pointer_type unsigned long
#endif

#ifdef lst2
#undef lst2
#endif

namespace {
    template<class T> struct Delete {
        void operator()(pair<const Value *const,T*> &x) { delete x.second; }
        void operator()(T*p) { delete p; }
    };

    void MakeLowerCase(string &s) {
        for (int i=0; i<s.size(); ++i) {
            s[i] = tolower(s[i]);
        }
    }

    // Implementation of KConfigSource to use RStreams
    struct MyFileLoader : KConfigSource {
        virtual boost::shared_ptr<std::istream> openFile(const std::string &filename) {
            boost::shared_ptr<std::istream> result(new RStream(filename));
            return result;
        }
    };
}

KnightsConfigImpl::KnightsConfigImpl(const std::string &config_file_name, bool menu_strict)
    : knight_anim(0), default_item(0),
      stuff_bag_graphic(0),
      blood_icon(0)
{
    try {

        // Create the lua context
        lua_state = MakeLuaSandbox();
        lua_State *lua = lua_state.get();

        // Add our config functions to it
        AddLuaConfigFunctions(lua, this);
        AddLuaIngameFunctions(lua);

        // Add the standard controls
        AddStandardControls(lua, this);
        
        // Open the file
        MyFileLoader my_file_loader;
        kf.reset(new KFile(config_file_name, my_file_loader, random_ints, lua));

        // Load some lua code into the context
        // TODO: The lua file name should probably be an input to the ctor, like the kconfig name is ?
        LuaExecRStream(lua, "main.lua", 0, 0, 
                       false);   // look in top level rsrc directory only

        // Get the "kts" table
        lua_rawgeti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);  // [env]
        luaL_getsubtable(lua, -1, "kts");                       // [env kts]

        // Load the Config Map
        lua_getfield(lua, -1, "MISC_CONFIG");
        config_map.reset(new ConfigMap);
        PopConfigMap(lua, *config_map);

        // Load the Dungeon Environment Customization Menu
        lua_getfield(lua, -1, "MENU");  // [env kts menu]
        menu_wrapper.reset(new MenuWrapper(lua, menu_strict)); // [env kts]

        // Load misc other stuff.
        kf->pushSymbol("KNIGHT_ANIM");
        knight_anim = popAnim();
        kf->pushSymbol("KNIGHT_HOUSE_COLOURS");
        popHouseColours(*config_map);
        kf->pushSymbol("KNIGHT_HOUSE_COLOURS_MENU");
        popHouseColoursMenu();
        kf->pushSymbol("DEFAULT_ITEM");
        default_item = popItemType(0);

        // controls (read from kts.CONTROLS)
        // [env kts]
        lua_getfield(lua, -1, "CONTROLS");  // [env kts ctrls]
        lua_len(lua, -1);                   // [env kts ctrls len]
        const int ctrls_len = lua_tointeger(lua, -1);
        lua_pop(lua, 1);      // [env kts ctrls]
        control_set.reserve(ctrls_len);
        for (int i = 1; i <= ctrls_len; ++i) {
            lua_pushinteger(lua, i);  // [env kts ctrls i]
            lua_gettable(lua, -2);    // [env kts ctrls ctrl]
            control_set.push_back(ReadLuaPtr<Control>(lua, -1));
            lua_pop(lua, 1);          // [env kts ctrls]
        }
        lua_pop(lua, 1);   // [env kts]
        
        // monsters, gore
        kf->pushSymbol("BLOOD_ICON");
        blood_icon = popGraphic();
        kf->pushSymbol("BLOOD_TILES");
        popTileList(blood_tiles);
        kf->pushSymbol("DEAD_KNIGHT_TILES");
        popTileList(dead_knight_tiles);

        // Zombie activity table (kts.ZOMBIE_ACTIVITY)
        luaL_getsubtable(lua, -1, "ZOMBIE_ACTIVITY");  // [env kts zombie_table]
        popZombieActivityTable(lua);                   // [env kts]

        // misc other stuff
        kf->pushSymbol("STUFF_BAG_GRAPHIC");
        stuff_bag_graphic = popGraphic();

        // Hooks -- These are used for various sound effects that can't
        // be fitted in more directly.
        const char * hook_names[] = {
            "HOOK_WEAPON_DOWNSWING",
            "HOOK_WEAPON_PARRY",
            "HOOK_KNIGHT_DAMAGE",
            "HOOK_CREATURE_SQUELCH",
            "HOOK_SHOOT",
            "HOOK_MISSILE_MISS"
        };
    
        // Hooks are read from the global "kts" table, which should still be on top of lua stack
        for (int i=0; i<sizeof(hook_names)/sizeof(hook_names[0]); ++i) {
            LuaFunc ac(lua, -1, hook_names[i]);
            if (ac.hasValue()) {
                hooks.insert(std::make_pair(hook_names[i], ac));
            }
        }

        // Tutorial (kts.TUTORIAL)
        luaL_getsubtable(lua, -1, "TUTORIAL");  // [env kts tutorial_table]
        popTutorial(lua);                       // [env kts]

        // now pop env and "kts" table from lua stack
        lua_pop(lua, 2);

        // make colour-changed versions of the dead knight tiles
        // (this has to be done last)
        const int dead_kt_size = dead_knight_tiles.size();
        for (int i = 1; i < house_colours_normal.size(); ++i) {
            for (int j = 0; j < dead_kt_size; ++j) {
                dead_knight_tiles.push_back(makeDeadKnightTile(dead_knight_tiles[j], house_colours_normal[i]));
            }
        }

    
        // Check that the stack is properly empty
        if (!kf->isStackEmpty()) {
            kf->error("internal error: stack non-empty");
        }
    
        // Fill knight_anims
        for (int i = 0; i < house_colours_normal.size(); ++i) {
            knight_anims.push_back(new Anim(*knight_anim));
            knight_anims.back()->setID(knight_anims.size() + lua_anims.size());
            knight_anims.back()->setColourChangeNormal(house_colours_normal[i]);
            knight_anims.back()->setColourChangeInvulnerable(house_colours_invulnerable[i]);
        }
        
    } catch (...) {
        // ensure memory gets freed if there is an exception during construction.
        freeMemory();
        throw;
    }
}

void KnightsConfigImpl::freeMemory()
{
    using std::for_each;
    for (size_t i=0; i<special_item_types.size(); ++i) delete special_item_types[i];
    for (size_t i=0; i<knight_anims.size(); ++i) delete knight_anims[i];

    for_each(lua_anims.begin(), lua_anims.end(), Delete<Anim>());
    for_each(lua_controls.begin(), lua_controls.end(), Delete<Control>());
    for_each(lua_graphics.begin(), lua_graphics.end(), Delete<Graphic>());
    for_each(lua_item_types.begin(), lua_item_types.end(), Delete<ItemType>());
    for_each(lua_monster_types.begin(), lua_monster_types.end(), Delete<MonsterType>());
    for_each(lua_overlays.begin(), lua_overlays.end(), Delete<Overlay>());
    for_each(lua_segments.begin(), lua_segments.end(), Delete<Segment>());
    for_each(lua_sounds.begin(), lua_sounds.end(), Delete<Sound>());
}

KnightsConfigImpl::~KnightsConfigImpl()
{
    freeMemory();
}


//
// "Pop" routines
//

Anim * KnightsConfigImpl::popAnim()
{
    if (!kf) return 0;

    Anim *result = 0;

    if (kf->isLua()) {
        kf->popLua();
        result = ReadLuaPtr<Anim>(lua_state.get(), -1);
        lua_pop(lua_state.get(), 1);
    }

    if (!result) kf->errExpected("anim");

    return result;
}

Anim * KnightsConfigImpl::popAnim(Anim *dflt)
{
    if (!kf) return 0;
    if (kf->isNone()) {
        kf->pop();
        return dflt;
    } else {
        return popAnim();
    }
}

Graphic * KnightsConfigImpl::popGraphic()
{
    if (!kf) return 0;

    Graphic *result = 0;
    
    if (kf->isLua()) {
        kf->popLua();
        result = ReadLuaPtr<Graphic>(lua_state.get(), -1);
        lua_pop(lua_state.get(), 1);
    }

    if (!result) kf->errExpected("graphic");

    return result;
}

Graphic * KnightsConfigImpl::popGraphic(Graphic *dflt)
{
    if (!kf) return 0;
    if (kf->isNone()) {
        kf->pop();
        return dflt;
    } else {
        return popGraphic();
    }
}


void KnightsConfigImpl::popHouseColours(const ConfigMap &config_map)
{
    // Here we set the local variable "house_colours" (both normal and invulnerable versions).
    // Expected input format is a list of lists of RGB colours.
    if (!kf) return;
    KFile::List lst(*kf, "HouseColourList");
    if (lst.getSize() < 1) {
        kf->error("house colour list is empty");
    } else {

        Colour rgb;  // work space
        vector<Colour> src_cols;  // colours of the first House
        
        // First entry: N "src" colors (store these in src_cols)
        {
            lst.push(0);
            KFile::List cols(*kf, "HouseColours");
            for (int i=0; i<cols.getSize(); ++i) {
                cols.push(i);
                rgb = popRGB();
                src_cols.push_back(rgb);
            }
            // first House's ColourChange is always empty
            house_colours_normal.push_back(ColourChange());
            // for first House's invulnerability, we set a single colour change
            // from src_cols[0] to CfgInt("invuln_r/g/b")
            ColourChange cc;
            if (!src_cols.empty()) {
                cc.add(src_cols[0], Colour(config_map.getInt("invuln_r"), 
                                           config_map.getInt("invuln_g"), 
                                           config_map.getInt("invuln_b")));
            }
            house_colours_invulnerable.push_back(cc);
        }
        
        // Second and subsequent entries: N "dest" colors
        // (create a ColourChange from "src_cols" to the newly-read dest cols)
        // for the Invulnerable one: we overwrite the first entry with the value of
        // CfgInt("invuln_r/g/b").
        for (int n=1; n<lst.getSize(); ++n) {
            lst.push(n);
            KFile::List cols(*kf, "HouseColours", src_cols.size());
            ColourChange cc, cc2;
            for (int i=0; i<cols.getSize(); ++i) {
                cols.push(i);
                rgb = popRGB();
                cc.add(src_cols[i], rgb);
                cc2.add(src_cols[i], i==0? Colour(config_map.getInt("invuln_r"), 
                                                  config_map.getInt("invuln_g"), 
                                                  config_map.getInt("invuln_b")) : rgb);
            }
            house_colours_normal.push_back(cc);
            house_colours_invulnerable.push_back(cc2);
        }

        // Now do the secured cc's:
        // (By convention, home CC's change from red ie $FF0000 to the first colour
        // in the house colours list)
        for (int i=0; i<lst.getSize(); ++i) {
            lst.push(i);
            KFile::List cols(*kf, "HouseColours");
            Colour rgb;
            cols.push(0);
            rgb = popRGB();
            // (As a special rule, if all components are $40 or less, the colour value is 
            // doubled. This is to prevent homes secured by Black knights being invisible.)
            if (rgb.r <= 0x40 && rgb.g <= 0x40 && rgb.b <= 0x40) {
                rgb.r *= 2;
                rgb.g *= 2;
                rgb.b *= 2;
            }
            shared_ptr<ColourChange> cc(new ColourChange);
            cc->add(Colour(255,0,0),rgb);
            secured_cc.push_back(cc);
        }
    }
}

void KnightsConfigImpl::popHouseColoursMenu()
{
    if (!kf) return;
    KFile::List lst(*kf, "ColourList");
    if (lst.getSize() < 1) {
        kf->error("house colour menu list is empty");
    } else {
        house_col_vector.reserve(lst.getSize());
        for (int i = 0; i < lst.getSize(); ++i) {
            lst.push(i);
            const Colour rgb = popRGB();
            house_col_vector.push_back(Coercri::Color(rgb.r, rgb.g, rgb.b));
        }
    }
}

const ItemType * KnightsConfigImpl::popItemType()
{
    if (!kf) return 0;
    const ItemType * result = 0;
    
    if (kf->isLua()) {
        kf->popLua();  // pop from kfile stack, push to lua stack
        result = ReadLuaPtr<const ItemType>(lua_state.get(), -1);
        lua_pop(lua_state.get(), 1);
    }
    
    if (!result) kf->errExpected("item type");
    return result;
}

const ItemType * KnightsConfigImpl::popItemType(const ItemType *dflt)
{
    if (!kf) return 0;
    if (kf->isNone()) {
        kf->pop();
        return dflt;
    } else {
        return popItemType();
    }
}

void KnightsConfigImpl::setOverlayOffsets(lua_State *lua)
{
    // expects to find arguments at positions 1,2,3,etc on the lua stack.
    // does not modify the lua stack.
    
    for (int fr = 0; fr < Overlay::N_OVERLAY_FRAME; ++fr) {
        for (int fac = 0; fac < 4; ++fac) {
            const int i = (fr*4 + fac)*3;
            overlay_offsets[fr][fac].dir = GetMapDirection(lua, i+1);
            overlay_offsets[fr][fac].ofsx = luaL_checkinteger(lua, i+2);
            overlay_offsets[fr][fac].ofsy = luaL_checkinteger(lua, i+3);
        }
    }
}

Colour KnightsConfigImpl::popRGB()
{
    if (!kf) return Colour(0,0,0);
    int x = kf->popInt();
    return Colour((x >> 16) & 255,  (x >> 8) & 255, x & 255);
}

boost::shared_ptr<Tile> KnightsConfigImpl::makeDeadKnightTile(boost::shared_ptr<Tile> orig_tile, const ColourChange &cc)
{
    boost::shared_ptr<Graphic> new_graphic(new Graphic(*orig_tile->getGraphic()));
    new_graphic->setColourChange(cc);
    new_graphic->setID(dead_knight_graphics.size() + lua_graphics.size() + 1);
    dead_knight_graphics.push_back(new_graphic);
    return orig_tile->cloneWithNewGraphic(new_graphic.get());
}

shared_ptr<Tile> KnightsConfigImpl::popTile()
{
    shared_ptr<Tile> result;
    if (kf && kf->isLua()) {
        kf->popLua();
        result = ReadLuaSharedPtr<Tile>(lua_state.get(), -1);
        lua_pop(lua_state.get(), 1);
    }
    if (!result) kf->errExpected("tile");
    return result;
}

shared_ptr<Tile> KnightsConfigImpl::popTile(shared_ptr<Tile> dflt)
{
    if (!kf) return shared_ptr<Tile>();
    if (kf->isNone()) {
        kf->pop();
        return dflt;
    } else {
        return popTile();
    }
}

void KnightsConfigImpl::popTileList(vector<shared_ptr<Tile> > &output)
{
    output.clear();
    if (!kf) return;
    KConfig::KFile::List lst(*kf, "TileList");
    output.reserve(lst.getSize());
    for (int i=0; i<lst.getSize(); ++i) {
        lst.push(i);
        output.push_back(popTile());
    }
}

void KnightsConfigImpl::popTutorial(lua_State *lua)
{
    // [tutorial_table]
    lua_len(lua, -1);  // [tt len]
    const int sz = lua_tointeger(lua, -1);
    lua_pop(lua, 1);  // [tt]

    if (sz % 3 != 0) luaL_error(lua, "kts.TUTORIAL list size must be a multiple of 3");

    for (int i = 0; i < sz; i += 3) {
        lua_pushinteger(lua, i+1);  // [tt 1]
        lua_gettable(lua, -2);  // [tt key]
        const int t_key = lua_tointeger(lua, -1);
        lua_pop(lua, 1);  // [tt]

        lua_pushinteger(lua, i+2);  // [tt 2]
        lua_gettable(lua, -2);   // [tt title]
        const char * title_c = lua_tostring(lua, -1);
        std::string title = title_c ? title_c : "";
        lua_pop(lua, 1);  // [tt]
        
        lua_pushinteger(lua, i+3);  // [tt 3]
        lua_gettable(lua, -2); // [tt msg]
        const char * msg_c = lua_tostring(lua, -1);
        std::string msg = msg_c ? msg_c : "";
        lua_pop(lua, 1);  // [tt]

        tutorial_data.insert(std::make_pair(t_key, std::make_pair(title, msg)));
    }

    lua_pop(lua, 1);  // []
}

void KnightsConfigImpl::popZombieActivityTable(lua_State *lua)
{
    // [zomtable]
    lua_len(lua, -1);  // [zt len]
    const int sz = lua_tointeger(lua, -1);
    lua_pop(lua, 1);  // [zt]

    for (int i = 0; i < sz; ++i) {
        lua_pushinteger(lua, i+1);  // [zt i]
        lua_gettable(lua, -2);      // [zt entry]

        // entry is a table of two things: tilefrom, and tileto/monsterto.

        ZombieActivityEntry ent;
        
        lua_pushinteger(lua, 1);  // [zt entry 1]
        lua_gettable(lua, -2);    // [zt entry from]
        ent.from = ReadLuaSharedPtr<Tile>(lua, -1);
        lua_pop(lua, 1);          // [zt entry]

        lua_pushinteger(lua, 2);  // [zt entry 2]
        lua_gettable(lua, -2);    // [zt entry to]

        if (IsLuaPtr<MonsterType>(lua, -1)) {
            ent.to_monster_type = ReadLuaPtr<MonsterType>(lua, -1);
        } else {
            ent.to_monster_type = 0;
            ent.to_tile = ReadLuaSharedPtr<Tile>(lua, -1);
        }

        zombie_activity.push_back(ent);
        
        lua_pop(lua, 2);  // [zt]
    }

    lua_pop(lua, 1);  // []
}

void KnightsConfigImpl::addZombieActivity(MonsterManager &mm, shared_ptr<Tile> from, const ZombieActivityEntry &ze)
{
    if (ze.to_tile) {
        mm.addZombieDecay(from, ze.to_tile);
    } else {
        mm.addZombieReanimate(from, ze.to_monster_type);
    }
}

int KnightsConfigImpl::getSegmentCategory(const string &cname) 
{
    if (cname.empty()) return -1;
    return segment_categories.insert(make_pair(cname, segment_categories.size())).first->second;
}

int KnightsConfigImpl::getTileCategory(const string &cname)
{
    if (cname.empty()) return -1;
    return tile_categories.insert(make_pair(cname, tile_categories.size())).first->second;
}



//
// stuff forwarded from KnightsConfig
//

namespace {
    template<class T>
    class CompareID {
    public:
        bool operator()(const T* lhs, const T* rhs) const {
            return lhs->getID() < rhs->getID();
        }
    };
}

void KnightsConfigImpl::getAnims(std::vector<const Anim*> &out) const
{
    out.clear();
    out.reserve(knight_anims.size() + lua_anims.size());
    for (std::vector<Anim*>::const_iterator it = lua_anims.begin(); it != lua_anims.end(); ++it) {
        out.push_back(*it);
    }
    std::copy(knight_anims.begin(), knight_anims.end(), std::back_inserter(out));
    std::sort(out.begin(), out.end(), CompareID<Anim>());
}

void KnightsConfigImpl::getGraphics(std::vector<const Graphic*> &out) const
{
    out.clear();
    out.reserve(lua_graphics.size() + dead_knight_graphics.size());
    for (std::vector<Graphic*>::const_iterator it = lua_graphics.begin(); it != lua_graphics.end(); ++it) {
        out.push_back(*it);
    }
    for (std::vector<boost::shared_ptr<Graphic> >::const_iterator it = dead_knight_graphics.begin(); it != dead_knight_graphics.end(); ++it) {
        out.push_back(it->get());
    }
    std::sort(out.begin(), out.end(), CompareID<Graphic>());
}

void KnightsConfigImpl::getOverlays(std::vector<const Overlay*> &out) const
{
    out.clear();
    out.reserve(lua_overlays.size());
    for (std::vector<Overlay*>::const_iterator it = lua_overlays.begin(); it != lua_overlays.end(); ++it) {
        out.push_back(*it);
    }
    std::sort(out.begin(), out.end(), CompareID<Overlay>());
}

void KnightsConfigImpl::getSounds(std::vector<const Sound*> &out) const
{
    out.clear();
    out.reserve(lua_sounds.size());
    for (std::vector<Sound*>::const_iterator it = lua_sounds.begin(); it != lua_sounds.end(); ++it) {
        out.push_back(*it);
    }
    std::sort(out.begin(), out.end(), CompareID<Sound>());
}

void KnightsConfigImpl::getStandardControls(std::vector<const UserControl*> &out) const
{
    out.clear();
    out.reserve(NUM_STANDARD_CONTROLS);
    std::copy(lua_controls.begin(), lua_controls.begin() + NUM_STANDARD_CONTROLS, std::back_inserter(out));
}

void KnightsConfigImpl::getOtherControls(std::vector<const UserControl*> &out) const
{
    out.clear();
    out.reserve(lua_controls.size());
    std::copy(lua_controls.begin() + NUM_STANDARD_CONTROLS, lua_controls.end(), std::back_inserter(out));
    std::sort(out.begin(), out.end(), CompareID<UserControl>());
}

void KnightsConfigImpl::initializeGame(boost::shared_ptr<DungeonMap> &dungeon_map,
                                       boost::shared_ptr<CoordTransform> &coord_transform,
                                       HomeManager &home_manager,
                                       std::vector<boost::shared_ptr<Player> > &players,
                                       StuffManager &stuff_manager,
                                       GoreManager &gore_manager,
                                       MonsterManager &monster_manager,
                                       EventManager &event_manager,
                                       TaskManager &task_manager,
                                       const std::vector<int> &hse_cols,
                                       const std::vector<std::string> &player_names,
                                       TutorialManager *tutorial_manager)
{
    dungeon_map.reset(new DungeonMap);
    coord_transform.reset(new CoordTransform);

    // Add players
    players.clear();
    for (int i = 0; i < hse_cols.size(); ++i) {
        const int team_num = hse_cols[i];
        boost::shared_ptr<Player> player(new Player(i, *dungeon_map, 
                                                    knight_anims.at(hse_cols[i]), default_item,
                                                    control_set,
                                                    secured_cc.at(hse_cols[i]),
                                                    player_names[i],
                                                    team_num));
        players.push_back(player);
    }
    
    // Set up stuff manager
    stuff_manager.setStuffBagGraphic(lua_state.get(), stuff_bag_graphic);
    stuff_manager.setDungeonMap(dungeon_map.get());

    // Set up gore manager
    if (dead_knight_tiles.size() >= 4 * house_colours_normal.size()) {
        for (int i = 0; i < players.size(); ++i) {
            const int offset = i%2;
            gore_manager.setKnightCorpse(*players[i],
                                         dead_knight_tiles[hse_cols[i] * 4 + offset],
                                         dead_knight_tiles[hse_cols[i] * 4 + offset + 1]);
        }
    }
    for (std::map<MonsterType *, std::vector<shared_ptr<Tile> > >::const_iterator it = monster_corpse_tiles.begin();
    it != monster_corpse_tiles.end(); ++it) {
        for (std::vector<shared_ptr<Tile> >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            gore_manager.addMonsterCorpse(it->first, *it2);
        }
    }
    for (int i = 0; i < blood_tiles.size(); ++i) {
        gore_manager.addBloodTile(blood_tiles[i]);
    }
    gore_manager.setBloodIcon(blood_icon);

    // Prevent zombies getting up immediately after being killed
    monster_manager.setRespawnWait(config_map->getInt("monster_respawn_wait"));
    
    // Set up a task to run the monster manager every so often
    shared_ptr<MonsterTask> mtsk (new MonsterTask);
    task_manager.addTask(mtsk, TP_LOW, task_manager.getGVT()+1);
    
    // Set up event manager
    event_manager.setupHooks(hooks);

    // Set up tutorial manager if needed
    if (tutorial_manager) {
        for (std::map<int, std::pair<std::string,std::string> >::const_iterator it = tutorial_data.begin(); 
        it != tutorial_data.end(); ++it) {
            tutorial_manager->addTutorialKey(it->first, it->second.first, it->second.second);
        }
    }
}

const Menu & KnightsConfigImpl::getMenu() const
{
    return menu_wrapper->getMenu();
}

void KnightsConfigImpl::resetMenu()
{
    lua_State *lua = lua_state.get();
    ASSERT(lua);
    
    lua_rawgeti(lua, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS); // [env]
    luaL_getsubtable(lua, -1, "kts");  // [env kts]
    lua_getfield(lua, -1, "MENU");     // [env kts menu]
    menu_wrapper.reset(new MenuWrapper(lua, menu_wrapper->getStrict()));  // [env kts]
    lua_pop(lua, 2); // []
}

int KnightsConfigImpl::getApproachOffset() const
{
    return config_map->getInt("approach_offset");
}

void KnightsConfigImpl::getHouseColours(std::vector<Coercri::Color> &result) const
{
    result = house_col_vector;
}



//
// Lua related functions
//

Anim * KnightsConfigImpl::addLuaAnim(lua_State *lua, int idx)
{
    const int new_id = lua_anims.size() + 1;
    Anim *q = new Anim(new_id, lua, idx);
    lua_anims.push_back(q);
    return q;
}

Control * KnightsConfigImpl::addLuaControl(auto_ptr<Control> p)
{
    const int new_id = lua_controls.size() + 1;
    p->setID(new_id);
    
    Control *q = p.release();
    lua_controls.push_back(q);
    return q;
}
   
void KnightsConfigImpl::addLuaGraphic(auto_ptr<Graphic> p)
{
    ASSERT(dead_knight_graphics.empty());
    
    const int new_id = lua_graphics.size() + 1;
    p->setID(new_id);

    lua_graphics.push_back(p.release());
}

ItemType * KnightsConfigImpl::addLuaItemType(auto_ptr<ItemType> p)
{
    // If a crossbow, then also set up a second itemtype for the loaded version.
    if (p->canLoad()) {
        ItemType *it2 = new ItemType(*p);   // make a copy of the old itemtype
        special_item_types.push_back(it2);
        p->setLoaded(it2);
        it2->setUnloaded(p.get());
    }

    lua_item_types.push_back(p.release());
    return lua_item_types.back();
}

MonsterType * KnightsConfigImpl::addLuaMonsterType(auto_ptr<MonsterType> p,
                                                   std::vector<boost::shared_ptr<Tile> > &generator_tiles,
                                                   std::vector<boost::shared_ptr<Tile> > &corpse_tiles)
{
    monster_corpse_tiles.insert(std::make_pair(p.get(), corpse_tiles));
    monster_generator_tiles.insert(std::make_pair(p.get(), generator_tiles));

    MonsterType * q = p.release();
    lua_monster_types.push_back(q);
    return q;
}

Overlay * KnightsConfigImpl::addLuaOverlay(auto_ptr<Overlay> p)
{
    const int id = lua_overlays.size() + 1;
    p->setID(id);

    // setup overlay offsets
    for (int fr = 0; fr < Overlay::N_OVERLAY_FRAME; ++fr) {
        for (int fac = 0; fac < 4; ++fac) {
            p->setOffset(MapDirection(fac), fr,
                         overlay_offsets[fr][fac].dir,
                         overlay_offsets[fr][fac].ofsx,
                         overlay_offsets[fr][fac].ofsy);
        }
    }
    
    Overlay *q = p.release();
    lua_overlays.push_back(q);
    return q;
}

Segment * KnightsConfigImpl::addLuaSegment(auto_ptr<Segment> p, const char *category)
{
    Segment *q = p.release();
    lua_segments.push_back(q);
    return q;
}

Sound * KnightsConfigImpl::addLuaSound(const char *name)
{
    const int new_id = lua_sounds.size() + 1;
    Sound * p = new Sound(new_id, name);
    lua_sounds.push_back(p);
    return p;
}

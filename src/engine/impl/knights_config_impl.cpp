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

#include "action.hpp"
#include "anim.hpp"
#include "control.hpp"
#include "control_actions.hpp"
#include "coord_transform.hpp"
#include "create_quest.hpp"
#include "dungeon_generator.hpp"
#include "dungeon_layout.hpp"
#include "dungeon_map.hpp"
#include "event_manager.hpp"
#include "gore_manager.hpp"
#include "graphic.hpp"
#include "home_manager.hpp"
#include "item_check_task.hpp"
#include "item_generator.hpp"
#include "item_respawn_task.hpp"
#include "knights_config_impl.hpp"
#include "lua_ingame.hpp"
#include "lua_exec.hpp"
#include "lua_load_from_rstream.hpp"
#include "lua_sandbox.hpp"
#include "lua_setup.hpp"
#include "lua_userdata.hpp"
#include "menu_int.hpp"
#include "menu_item.hpp"
#include "menu_selections.hpp"
#include "monster_manager.hpp"
#include "monster_task.hpp"
#include "monster_type.hpp"
#include "overlay.hpp"
#include "player.hpp"
#include "round.hpp"
#include "rng.hpp"
#include "rstream.hpp"
#include "segment.hpp"
#include "sound.hpp"
#include "stuff_bag.hpp"
#include "task_manager.hpp"
#include "tile.hpp"
#include "time_limit_task.hpp"
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

    // for "MyActionPars" it is expected that the argument list is on the top
    // of the KFile stack.
    class MyActionPars : public ActionPars {
    public:
        MyActionPars(KnightsConfigImpl &kc_) : kc(kc_), lst(*kc_.getKFile(), "ActionParameters") { }
        virtual ~MyActionPars() { }
        virtual void require(int n1, int n2) {
            if (lst.getSize() != n1 && (n2<0 || lst.getSize() != n2)) error();
        }
        virtual int getSize() { return lst.getSize(); }
        virtual int getInt(int i) { lst.push(i); return kc.getKFile()->popInt(); }
        virtual const ItemType *getItemType(int i) { lst.push(i); return kc.popItemType(); }
        virtual float getProbability(int i) { lst.push(i); return kc.popProbability(); }
        virtual const MonsterType * getMonsterType(int i)
            { lst.push(i); return kc.popMonsterType(); }
        virtual const Sound * getSound(int i) { lst.push(i); return kc.popSound(); }
        virtual string getString(int i) { lst.push(i); return kc.getKFile()->popString(); }
        virtual shared_ptr<Tile> getTile(int i) { lst.push(i); return kc.popTile(); }
        virtual void error() { kc.getKFile()->error("error while processing action"); }
    private:
        KnightsConfigImpl &kc;
        KFile::List lst;
    };

    // Implementation of KConfigSource to use RStreams
    struct MyFileLoader : KConfigSource {
        virtual boost::shared_ptr<std::istream> openFile(const std::string &filename) {
            boost::shared_ptr<std::istream> result(new RStream(filename));
            return result;
        }
    };
}

KnightsConfigImpl::Popper::~Popper()
{
    // We want to be careful not to throw an exception from within a dtor; therefore,
    // explicitly check !isStackEmpty() before calling pop().
    if (!kf.isStackEmpty()) kf.pop();
}

KnightsConfigImpl::KnightsConfigImpl(const std::string &config_file_name)
    : knight_anim(0), default_item(0),
      stuff_bag_graphic(0),
      blood_icon(0)
{
    Sentry s(*this);

    try {

        // Create the lua context
        lua_state = MakeLuaSandbox();

        // Add our config functions to it
        AddLuaConfigFunctions(lua_state.get(), this);
        AddLuaIngameFunctions(lua_state.get());

        // Open the file
        MyFileLoader my_file_loader;
        kf.reset(new KFile(config_file_name, my_file_loader, random_ints, lua_state.get()));

        // Load some lua code into the context
        // TODO: The lua file name should probably be an input to the ctor, like the kconfig name is ?
        LuaExecRStream(lua_state.get(), "main.lua", 0, 0, 
                       false);   // look in top level rsrc directory only

        // Get the "kts" table
        lua_rawgeti(lua_state.get(), LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);  // [env]
        luaL_getsubtable(lua_state.get(), -1, "kts");                       // [env kts]

        // Load the Config Map
        lua_getfield(lua_state.get(), -1, "MISC_CONFIG");
        config_map.reset(new ConfigMap);
        PopConfigMap(lua_state.get(), *config_map);

        // Load the Dungeon Environment Customization Menu
        kf->pushSymbol("MAIN_MENU");
        popMenu();
        kf->pushSymbol("MAIN_MENU_SPACE");
        popMenuSpace();

        // Load the segment set
        kf->pushSymbol("DUNGEON_SEGMENTS");
        popSegmentSet();

        // Load misc other stuff.
        kf->pushSymbol("WALL");
        wall_tile = popTile();
        kf->pushSymbol("HORIZ_DOOR_1");
        horiz_door_tile[0] = popTile();
        kf->pushSymbol("HORIZ_DOOR_2");
        horiz_door_tile[1] = popTile();
        kf->pushSymbol("VERT_DOOR_1");
        vert_door_tile[0] = popTile();
        kf->pushSymbol("VERT_DOOR_2");
        vert_door_tile[1] = popTile();
        kf->pushSymbol("KNIGHT_ANIM");
        knight_anim = popAnim();
        kf->pushSymbol("KNIGHT_HOUSE_COLOURS");
        popHouseColours(*config_map);
        kf->pushSymbol("KNIGHT_HOUSE_COLOURS_MENU");
        popHouseColoursMenu();
        kf->pushSymbol("DEFAULT_ITEM");
        default_item = popItemType(0);
        kf->pushSymbol("CONTROLS");
        popControlSet(control_set);
        kf->pushSymbol("QUEST_DESCRIPTIONS");
        popQuestDescriptions();

        // monsters, gore
        kf->pushSymbol("BLOOD_ICON");
        blood_icon = popGraphic();
        kf->pushSymbol("BLOOD_TILES");
        popTileList(blood_tiles);
        kf->pushSymbol("DEAD_KNIGHT_TILES");
        popTileList(dead_knight_tiles);

        // Zombie activity table (kts.ZOMBIE_ACTIVITY)
        luaL_getsubtable(lua_state.get(), -1, "ZOMBIE_ACTIVITY");  // [env kts zombie_table]
        popZombieActivityTable(lua_state.get());                   // [env kts]

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
            "HOOK_BAT",
            "HOOK_ZOMBIE",
            "HOOK_SHOOT",
            "HOOK_MISSILE_MISS"
        };
    
        // Hooks are read from the global "kts" table, which should still be on top of lua stack
        for (int i=0; i<sizeof(hook_names)/sizeof(hook_names[0]); ++i) {
            const Action *ac = LuaGetAction(lua_state.get(), -1, hook_names[i], this);
            if (ac) {
                hooks.insert(std::make_pair(hook_names[i], ac));
            }
        }

        // Tutorial (kts.TUTORIAL)
        luaL_getsubtable(lua_state.get(), -1, "TUTORIAL");  // [env kts tutorial_table]
        popTutorial(lua_state.get());                       // [env kts]

        // now pop env and "kts" table from lua stack
        lua_pop(lua_state.get(), 2);

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
    for_each(actions.begin(), actions.end(), Delete<Action>());
    for_each(controls.begin(), controls.end(), Delete<Control>());
    for_each(dungeon_directives.begin(), dungeon_directives.end(), Delete<DungeonDirective>());
    for (size_t i=0; i<special_item_types.size(); ++i) delete special_item_types[i];
    for_each(menu_ints.begin(), menu_ints.end(), Delete<MenuInt>());
    for_each(overlays.begin(), overlays.end(), Delete<Overlay>());
    for_each(segments.begin(), segments.end(), Delete<Segment>());
    for (size_t i=0; i<knight_anims.size(); ++i) delete knight_anims[i];

    for_each(lua_actions.begin(), lua_actions.end(), Delete<Action>());
    for_each(lua_anims.begin(), lua_anims.end(), Delete<Anim>());
    for_each(lua_controls.begin(), lua_controls.end(), Delete<Control>());
    for_each(lua_dungeon_layouts.begin(), lua_dungeon_layouts.end(), Delete<RandomDungeonLayout>());
    for_each(lua_graphics.begin(), lua_graphics.end(), Delete<Graphic>());
    for_each(lua_item_generators.begin(), lua_item_generators.end(), Delete<ItemGenerator>());
    for_each(lua_item_types.begin(), lua_item_types.end(), Delete<ItemType>());
    for_each(lua_monster_types.begin(), lua_monster_types.end(), Delete<MonsterType>());
    for_each(lua_overlays.begin(), lua_overlays.end(), Delete<Overlay>());
    for_each(lua_sounds.begin(), lua_sounds.end(), Delete<Sound>());
}

KnightsConfigImpl::~KnightsConfigImpl()
{
    freeMemory();
}

KnightsConfigImpl::Sentry::~Sentry()
{
    // Free up memory
    cfg.menu_item_names.clear();
    cfg.segment_categories.clear();
    cfg.tile_categories.clear();

    // Close the file
    cfg.kf = boost::shared_ptr<KFile>();
}


//
// "Pop" routines
//

MapAccess KnightsConfigImpl::popAccessCode(MapAccess dflt)
{
    if (!kf) return A_BLOCKED;

    // We use "Popper" and getString rather than just calling
    // popString directly. This ensures that the string remains on the
    // top of the stack, at the point where "errExpected" is called.
    // This ensures in turn that the correct error traceback is given.

    // Note that this is a slight hack -- a better solution would be
    // to use sentry objects (like KFile::List or KFile::Table) for
    // strings (or other types) as needed. The reason it's a hack is
    // that if (eg) we find an integer, we'll get two errExpected's:
    // one expecting a string and one expecting an access code (in
    // this case).

    Popper p(*kf);
    std::string s = kf->getString("");

    if (s == "") return dflt;
    else if (s == "approach" || s == "partial") return A_APPROACH;
    else if (s == "blocked") return A_BLOCKED;
    else if (s == "clear") return A_CLEAR;
    kf->errExpected("access code ('approach', 'blocked' or 'clear')");
    return dflt;
}

void KnightsConfigImpl::popAccessTable(MapAccess acc[], MapAccess dflt)
{
    if (!kf) return;
    KFile::Table tab(*kf, "AccessTable");
    tab.push("flying");
    acc[H_FLYING] = popAccessCode(dflt);
    tab.push("missiles");
    acc[H_MISSILES] = popAccessCode(dflt);
    tab.push("walking");
    acc[H_WALKING] = popAccessCode(dflt);
}

Action * KnightsConfigImpl::popAction()
{
    if (!kf) return 0;
    const Value * p = kf->getTop();
    map<const Value *,Action*>::const_iterator it = actions.find(p);
    if (it == actions.end()) {

        if (kf->isLua()) {
            kf->popLua();  // pushes to lua stack
            
            // TEMPORARY code copied/pasted from lua_setup.cpp. This
            // should disappear once KConfig files are eliminated.
            Action *a = 0;
            if (lua_isnil(lua_state.get(), -1)) {
                lua_pop(lua_state.get(), 1);
            } else {
                a = new LuaAction(lua_state.get()); // pops the lua stack
            }

            it = actions.insert(make_pair(p, a)).first;
        
        } else if (kf->isList()) {
            // ListAction

            ListAction *action = new ListAction;
            it = actions.insert(make_pair(p, action)).first;
            
            KFile::List lst(*kf, "Action");
            action->reserve(lst.getSize());
            for (int i=0; i<lst.getSize(); ++i) {
                lst.push(i);
                action->add(popAction(0));
            }

        } else if (kf->isRandom()) {
            // RandomAction

            RandomAction *action = new RandomAction;
            it = actions.insert(make_pair(p, action)).first;

            KFile::Random ran(*kf, "Action");
            action->reserve(ran.getSize());
            for (int i=0; i<ran.getSize(); ++i) {
                int weight = ran.push(i);
                action->add(popAction(), weight);
            }
            
        } else if (kf->isDirective()) {
            // Normal Action

            it = actions.insert(pair<const Value *,Action*>(p, 0)).first;
            
            KFile::Directive dir(*kf, "Action");
            if (!dir.getName().empty()) {
                dir.pushArgList();
                MyActionPars pars(*this);
                actions.erase(p);
                Action *ac = ActionMaker::createAction(dir.getName(), pars);
                if (!ac) {
                    kf->error("unknown action: " + dir.getName());
                    return 0;
                }
                it = actions.insert(make_pair(p, ac)).first;
            } else {
                kf->error("directive has no name??");
            }

        } else if (kf->isNone()) {
            // Null Action
            kf->pop();
            return 0;

        } else if (kf->isString()) {
            // Lua Action
            string s = kf->getString();
            LuaLoadFromString(lua_state.get(), s.c_str());    // pushes function onto lua stack
            Action * lua_action = new LuaAction(lua_state.get());   // pops function from lua stack
            it = actions.insert(make_pair(p, lua_action)).first;
            
        } else {
            kf->errExpected("Action");
            kf->pop();
            return 0;
        }
    } else {
        kf->pop();
    }

    if (it->second == 0) {
        kf->error("Recursive action. Try enclosing the action in square brackets?");
    }
    
    return it->second;
}

Action * KnightsConfigImpl::popAction(Action *dflt)
{
    if (!kf) return 0;
    if (kf->isNone()) {
        kf->pop();
        return dflt;
    } else {
        return popAction();
    }
}

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

bool KnightsConfigImpl::popBool()
{
    if (!kf) return false;

    Popper p(*kf);
    int i = kf->getInt();

    if (i == 0) return false;
    else if (i == 1) return true;
    kf->errExpected("0 or 1");
    return false;
}

bool KnightsConfigImpl::popBool(bool dflt)
{
    if (!kf) return false;
    if (kf->isNone()) {
        kf->pop();
        return dflt;
    } else {
        return popBool();
    }
}

Control * KnightsConfigImpl::popControl()
{
    if (!kf) return 0;

    const Value * p = kf->getTop();
    map<const Value *,Control*>::iterator it = controls.find(p);
    if (it == controls.end()) {
        KFile::Table tab(*kf, "Control");   
        tab.push("action");
        Action *ac = popAction();
        tab.push("action_bar_slot");
        int abs = kf->popInt(-1);
        tab.push("action_bar_priority");
        int abp = kf->popInt(std::numeric_limits<int>::min());  // default = same as tap_priority (for now)
        tab.push("continuous");
        bool cts = popBool(false);
        tab.push("menu_direction");
        MapDirection mdir = popMapDirection(D_NORTH);
        tab.push("menu_icon");
        Graphic *g = popGraphic(0);
        tab.push("menu_special");
        unsigned int ms = kf->popInt(0);
        tab.push("name");
        std::string name = kf->popString("");
        tab.push("suicide_key");
        bool suicide = popBool(false);
        tab.push("tap_priority");
        int tp = kf->popInt(0);

        if (abp == std::numeric_limits<int>::min()) abp = tp;
        
        const int id = controls.size() + lua_controls.size() + NUM_STANDARD_CONTROLS + 1;

        Control * new_ctrl = new Control(0, 0, 
                                         g, mdir, tp, abs, abp, suicide, cts, ms, name, ac);
        new_ctrl->setID(id);
        
        it = controls.insert(make_pair(p, new_ctrl)).first;
    } else {
        kf->pop();
    }

    return it->second;
}

Control * KnightsConfigImpl::popControl(Control *dflt)
{
    if (!kf) return 0;
    if (kf->isNone()) {
        kf->pop();
        return dflt;
    } else {
        return popControl();
    }
}

void KnightsConfigImpl::popControlSet(vector<const Control*> &which_control_set)
{
    if (!kf) return;
    KFile::List lst(*kf, "ControlSet");
    which_control_set.reserve(lst.getSize());
    for (int i=0; i<lst.getSize(); ++i) {
        lst.push(i);
        which_control_set.push_back(popControl());
    }
}

DungeonDirective * KnightsConfigImpl::popDungeonDirective()
{
    if (!kf) return 0;
    
    const Value * p = kf->getTop();
    map<const Value *,DungeonDirective*>::iterator it = dungeon_directives.find(p);
    if (it == dungeon_directives.end()) {
        KFile::Directive dir(*kf, "MenuDirective");
        dir.pushArgList();
        DungeonDirective *d = DungeonDirective::create(dir.getName(), *this); // pops arglist
        it = dungeon_directives.insert(make_pair(p, d)).first;
    } else {
        kf->pop();
    }
    return it->second;
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

ItemGenerator * KnightsConfigImpl::popItemGenerator()
{
    if (!kf) return 0;

    ItemGenerator * result = 0;

    if (kf->isLua()) {
        kf->popLua();
        result = ReadLuaPtr<ItemGenerator>(lua_state.get(), -1);
        lua_pop(lua_state.get(), 1);
    }

    if (!result) kf->errExpected("item generator");
    return result;
}

ItemSize KnightsConfigImpl::popItemSize()
{
    if (!kf) return IS_NOPICKUP;

    Popper p(*kf);
    string s = kf->getString("");

    if (s == "held") return IS_BIG;
    else if (s == "backpack") return IS_SMALL;
    else if (s == "magic") return IS_MAGIC;
    else if (s == "nopickup") return IS_NOPICKUP;
    kf->errExpected("'backpack', 'held', 'magic' or 'nopickup'");
    return IS_NOPICKUP;
}

ItemSize KnightsConfigImpl::popItemSize(ItemSize dflt)
{
    if (!kf) return IS_NOPICKUP;
    if (kf->isNone()) {
        kf->pop();
        return dflt;
    } else {
        return popItemSize();
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

MapDirection KnightsConfigImpl::popMapDirection()
{
    if (!kf) return D_NORTH;

    Popper p(*kf);
    string s = kf->getString("");

    MakeLowerCase(s);
    if (s == "north" || s == "up") return D_NORTH;
    else if (s == "east" || s == "right") return D_EAST;
    else if (s == "south" || s == "down") return D_SOUTH;
    else if (s == "west" || s == "left") return D_WEST;
    kf->errExpected("compass direction");
    return D_NORTH;
}

MapDirection KnightsConfigImpl::popMapDirection(MapDirection dflt)
{
    if (!kf) return D_NORTH;
    if (kf->isNone()) {
        kf->pop();
        return dflt;
    } else {
        return popMapDirection();
    }
}

void KnightsConfigImpl::popMenu()
{
    if (!kf) return;

    KFile::List lst(*kf, "Menu");

    for (int i=0; i<lst.getSize(); ++i) {
        lst.push(i);
        if (kf->isDirective()) {
            DungeonDirective *d = popDungeonDirective();
            global_dungeon_directives.push_back(d);
        } else if (kf->isTable()) {
            menu.addItem(popMenuItem());
        } else {
            kf->errExpected("MenuItem");
            kf->pop();
        }
    }
}

void KnightsConfigImpl::popMenuSpace()
{
    if (!kf) return;

    KFile::List lst(*kf, "MenuSpace");

    for (int i=0; i<lst.getSize(); ++i) {
        lst.push(i);
        const string key = kf->popString();
        // crappy linear search for key
        for (int j = 0; j < menu.getNumItems(); ++j) {
            MenuItem &item = menu.getItem(j);
            if (item.getKey() == key) {
                item.setSpaceAfter();
                break;
            }
        }
    }
}

MenuItem KnightsConfigImpl::popMenuItem()
{
    if (!kf) return MenuItem("error", 0, 0, "error");  // should never happen.
    
    KFile::Table tab(*kf, "MenuItem");
    
    tab.push("id");
    const string id = kf->popString();

    // check we haven't already got this id
    for (int i = 0; i < menu.getNumItems(); ++i) {
        if (menu.getItem(i).getKey() == id) {
            kf->error(string("duplicate menu item: ") + id);
            break;
        }
    }

    tab.push("min_value");
    const int min_value = kf->popInt(0);

    tab.push("title");
    const string title = kf->popString();

    tab.push("values");
    KFile::List lst(*kf, "MenuValues");

    MenuItem result(id, min_value, lst.getSize(), title);
    
    for (int i=0; i<lst.getSize(); ++i) {
        lst.push(i);
        popMenuValue(i + min_value, result);
    }

    return result;
}

MenuInt * KnightsConfigImpl::popMenuInt()
{
    if (!kf) return 0;
    const Value * p = kf->getTop();
    map<const Value *, MenuInt*>::iterator it = menu_ints.find(p);

    if (it == menu_ints.end()) {
        it = menu_ints.insert(make_pair(p, static_cast<MenuInt*>(0))).first;
        if (kf->isInt()) {
            int val = kf->popInt();
            it->second = new MenuIntConst(val);
        } else if (kf->isDirective()) {
            KFile::Directive dir(*kf, "GetMenuDirective");
            if (dir.getName() != "GetMenu") {
                kf->errExpected("integer or GetMenu directive");
            } else {
                dir.pushArgList();
                KFile::List lst(*kf, "string", 1);
                if (lst.getSize()==1) {
                    lst.push(0);
                    string menu_item_name = kf->popString();
                    it->second = new MenuIntVar(menu_item_name);
                }
            }
        } else {
            kf->errExpected("integer or GetMenu directive");
            kf->pop();
        }

    } else {
        kf->pop();
    }

    return it->second;
}

void KnightsConfigImpl::popMenuValue(int val, MenuItem &menu_item)
{
    if (!kf) return;
    string name;
    if (kf->isList()) {
        KFile::List lst(*kf, "MenuValue");
        lst.push(0);
        name = popMenuValueName(val);
        for (int i=1; i<lst.getSize(); ++i) {
            lst.push(i);
            popMenuValueDirective(menu_item.getKey(), val);
        }
    } else {
        name = popMenuValueName(val);
    }
    menu_item.setValueString(val, name);
}

void KnightsConfigImpl::popMenuValueDirective(const string &key, int val)
{
    if (!kf) return;
    KFile::Directive dir(*kf, "MenuDirective");
    string dname = dir.getName();
    if (dname.substr(0,7) == "Dungeon") {
        DungeonDirective *d = popDungeonDirective();
        if (d) menu_dungeon_directives[key][val].push_back(d);
    } else if (dname.substr(0,5) == "Quest") {
        dir.pushArgList();
        CreateQuests(dname, *this, menu_quests[key][val]);
    } else if (dname.substr(0,9) == "Constrain") {
        dir.pushArgList();
        menu_constraints.addConstraintFromKFile(key, val, dname, *this);
    } else {
        kf->errExpected("MenuDirective");
    }
}

string KnightsConfigImpl::popMenuValueName(int val)
{
    if (!kf) return "error";  // should never happen
    if (kf->isString()) {
        return kf->popString();
    } else if (kf->isDirective()) {
        KFile::Directive dir(*kf, "MenuValueName");
        if (dir.getName() == "MenuNumber") {
            std::ostringstream str;
            str << val;
            return str.str();
        } else if (dir.getName() != "MenuIcon") {
            kf->errExpected("MenuValueName");
        } else {
            dir.pushArgList();
            KFile::List lst(*kf, "string or graphic", 1);
            if (lst.getSize()==1) {
                /*
                lst.push(0);
                if (kf->isString()) {
                    string result;
                    const char c = kf->popString()[0];
                    for (int x = 0; x < val; ++x) {
                        result += c;
                    }
                    return result;
                } else {
                    // graphic icons are ignored currently.
                    kf->pop();
                    std::ostringstream str;
                    str << val;
                    return str.str();
                }
                */
                // strings / graphics ignored currently
                // instead we just do the same as MenuNumber()...
                std::ostringstream str;
                str << val;
                return str.str();
            }
        }
    } else {
        kf->errExpected("MenuValueName");
        kf->pop();
    }
    return "error";
}

MonsterType * KnightsConfigImpl::popMonsterType()
{
    if (!kf) return 0;
    MonsterType *result = 0;

    if (kf->isLua()) {
        kf->popLua();
        result = ReadLuaPtr<MonsterType>(lua_state.get(), -1);
        lua_pop(lua_state.get(), 1);
    }

    if (!result) kf->errExpected("monster type");
    return result;
}

Overlay* KnightsConfigImpl::popOverlay()
{
    if (!kf) return 0;
    
    const Value * p = kf->getTop();
    map<const Value *, Overlay*>::iterator it = overlays.find(p);

    if (it == overlays.end()) {
        KFile::List lst(*kf, "Overlay", 4);

        if (lst.getSize()==4) {
            const int id = overlays.size() + lua_overlays.size() + 1;
            it = overlays.insert(make_pair(p, new Overlay(0,0))).first;
            it->second->setID(id);
            for (int i=0; i<4; ++i) {
                lst.push(i);
                it->second->setRawGraphic(MapDirection(i), popGraphic(0));
            }
            // setup overlay offsets
            for (int fr = 0; fr < Overlay::N_OVERLAY_FRAME; ++fr) {
                for (int fac = 0; fac < 4; ++fac) {
                    it->second->setOffset(MapDirection(fac), fr,
                                          overlay_offsets[fr][fac].dir,
                                          overlay_offsets[fr][fac].ofsx,
                                          overlay_offsets[fr][fac].ofsy);
                }
            }
        }
    } else {
        kf->pop();
    }
    return it->second;
}

Overlay * KnightsConfigImpl::popOverlay(Overlay *dflt)
{
    if (!kf) return 0;
    if (kf->isNone()) {
        kf->pop();
        return dflt;
    } else {
        return popOverlay();
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

float KnightsConfigImpl::popProbability()
{
    if (!kf) return 0;

    Popper pp(*kf);
    int p = kf->getInt();

    if (p < 0 || p > 100) {
        kf->errExpected("percentage");
        p = 0;
    }

    return p / 100.0f;
}

float KnightsConfigImpl::popProbability(int dflt)
{
    if (!kf) return 0;
    if (kf->isNone()) {
        kf->pop();
        return dflt / 100.0f;
    } else {
        return popProbability();
    }
}

void KnightsConfigImpl::popQuestDescriptions()
{
    if (!kf) return;
    KFile::List lst(*kf, "QuestDescriptions");
    standard_quest_descriptions.reserve(lst.getSize());
    for (int x = 0; x < lst.getSize(); ++x) {
        lst.push(x);
        std::string result = kf->popString();
        for (int j = 0; j < result.size(); ++j) {
            if (result[j] == '\\') result[j] = '\n';
        }
        standard_quest_descriptions.push_back(result);
    }
}

RandomDungeonLayout * KnightsConfigImpl::popRandomDungeonLayout()
{
    if (!kf) return 0;

    RandomDungeonLayout *result = 0;
    if (kf->isLua()) {
        kf->popLua();
        result = ReadLuaPtr<RandomDungeonLayout>(lua_state.get(), -1);
        lua_pop(lua_state.get(), 1);
    }
    if (!result) kf->errExpected("lua dungeon layout");
    
    return result;
}   

Colour KnightsConfigImpl::popRGB()
{
    if (!kf) return Colour(0,0,0);
    int x = kf->popInt();
    return Colour((x >> 16) & 255,  (x >> 8) & 255, x & 255);
}

Segment * KnightsConfigImpl::popSegment(const vector<SegmentTileData> &tile_map, int &category)
{
    if (!kf) return 0;
    KFile::Table tab(*kf, "Segment");

    tab.push("height",false);
    int h = kf->popInt();
    tab.push("width",false);
    int w = kf->popInt();
    tab.reset();

    tab.push("category");
    string catname = kf->popString("");
    category = getSegmentCategory(catname);
    tab.push("data");
    Segment *segment = popSegmentData(tile_map, w, h);
    if (!segment) return 0;
    tab.push("height"); kf->pop();
    tab.push("name"); kf->pop();  // used only by the map editor
    tab.push("rooms");
    popSegmentRooms(*segment);
    tab.push("switches"); 
    if (kf->isNone()) kf->pop();
    else popSegmentSwitches(*segment);
    tab.push("width"); kf->pop();
    return segment;
}

Sound * KnightsConfigImpl::popSound()
{
    if (!kf) return 0;
    Sound * result = 0;
    if (kf->isLua()) {
        kf->popLua();
        result = ReadLuaPtr<Sound>(lua_state.get(), -1);
        lua_pop(lua_state.get(), 1);
    }
    if (!result) kf->errExpected("sound");
    return result;
}

const KnightsConfigImpl::SegmentTileData & KnightsConfigImpl::tileFromInt(const vector<SegmentTileData> &tile_map, int t)
{
    if (t < 1 || t > tile_map.size()) {
        static SegmentTileData err;
        kf->error("tile number out of range!");
        return err;
    } else {
        return tile_map[t-1];
    }
}

boost::shared_ptr<Tile> KnightsConfigImpl::makeDeadKnightTile(boost::shared_ptr<Tile> orig_tile, const ColourChange &cc)
{
    boost::shared_ptr<Graphic> new_graphic(new Graphic(*orig_tile->getGraphic()));
    new_graphic->setColourChange(cc);
    new_graphic->setID(dead_knight_graphics.size() + lua_graphics.size() + 1);
    dead_knight_graphics.push_back(new_graphic);
    return orig_tile->cloneWithNewGraphic(new_graphic.get());
}
    
void KnightsConfigImpl::processTile(const vector<SegmentTileData> &tile_map, Segment &segment,
                                    int x, int y)
{
    const int tile = kf->popInt();
    const SegmentTileData & seg_tile(tileFromInt(tile_map, tile));
    for (int i=0; i<seg_tile.tiles.size(); ++i) {
        segment.addTile(x, y, seg_tile.tiles[i]);
    }
    for (int i=0; i<seg_tile.items.size(); ++i) {
        segment.addItem(x, y, seg_tile.items[i]);
    }
    for (int i=0; i<seg_tile.monsters.size(); ++i) {
        segment.addMonster(x, y, seg_tile.monsters[i]);
    }
}

Segment * KnightsConfigImpl::popSegmentData(const vector<SegmentTileData> &tile_map,
                                            int w, int h)
{
    if (!kf) return 0;
    if (w<=0 || h<=0) {
        kf->error("bad width/height");
        kf->pop();
        return 0;
    } else {
        KFile::List lst(*kf, "SegmentData", w*h);
        Segment *segment = new Segment(w, h);
        try {
            for (int y=0; y<h; ++y) {
                for (int x=0; x<w; ++x) {
                    lst.push(y*w+x);
                    processTile(tile_map, *segment, x, y);  // pops
                }
            }
            return segment;
        } catch (...) {
            delete segment;
            throw;
        }
    }
}

void KnightsConfigImpl::popSegmentList(const vector<SegmentTileData> &tile_map)
{
    if (!kf) return;
    
    KFile::List lst(*kf, "SegmentList");

    for (int i=0; i<lst.getSize(); ++i) {
        lst.push(i);
        int rcat;
        Segment *rm = popSegment(tile_map, rcat);
        if (rm) segment_set.addSegment(rm, rm->getNumHomes(), rcat);
    }
}

void KnightsConfigImpl::popSegmentRooms(Segment &s)
{
    if (!kf) return;
    KFile::List lst(*kf, "RoomList");
    for (int i=0; i<lst.getSize(); ++i) {
        lst.push(i);
        KFile::List rum(*kf, "RoomData", 4);
        rum.push(0);
        const int tlx = kf->popInt()-1;
        rum.push(1);
        const int tly = kf->popInt()-1;
        rum.push(2);
        const int w = kf->popInt();
        rum.push(3);
        const int h = kf->popInt();
        if (tlx < -1 || tly < -1 || w <= 0 || h <= 0 || tlx+w-1 > s.getWidth() || tly+h-1 > s.getHeight()) kf->error("bad room data");
        s.addRoom(tlx, tly, w, h);
    }
}

void KnightsConfigImpl::popSegmentSet()
{
    if (!kf) return;

    KFile::Table tbl(*kf,"SegmentSet");

    vector<SegmentTileData> tile_map;
    tbl.push("tiles",false);
    popSegmentTiles(tile_map);

    tbl.reset();
    tbl.push("segments");
    popSegmentList(tile_map);
    tbl.push("tiles");
    kf->pop();
}

void KnightsConfigImpl::popSegmentSwitches(Segment &r)
{
    if (!kf) return;
    KFile::List lst(*kf, "SwitchList");
    for (int i=0; i<lst.getSize(); ++i) {
        lst.push(i);
        KFile::List sw(*kf, "SwitchData", 3);
        sw.push(0);
        const int x = kf->popInt();
        sw.push(1);
        const int y = kf->popInt();
        sw.push(2);
        Action *ac = popAction();

        // create dummy tile for the switch-action.
        const bool activate = r.isApproachable(x,y);
        shared_ptr<Tile> t(new Tile(activate?0:ac, activate?ac:0));
        r.addTile(x,y,t);
    }
}

void KnightsConfigImpl::popSegmentTiles(vector<SegmentTileData> &tile_map)
{
    if (!kf) return;
    KFile::List lst(*kf, "TileTable");
    
    for (int i=0; i<lst.getSize(); ++i) {
        SegmentTileData seg_tile;
        bool items_yet = false;
        bool monsters_yet = false;
        lst.push(i);

        if (kf->isList()) {
            KFile::List lst2(*kf, "SegmentTile");
            for (int j=0; j<lst2.getSize(); ++j) {
                lst2.push(j);
                popSegmentTile(seg_tile, items_yet, monsters_yet);
            }
        } else {
            popSegmentTile(seg_tile, items_yet, monsters_yet);
        }
        tile_map.push_back(seg_tile);
    }
}

void KnightsConfigImpl::popSegmentTile(SegmentTileData &seg_tile, bool &items_yet, bool &monsters_yet)
{
    if (kf->isInt()) {
        // This is a slight hack: You put an integer (usually '0') to signify
        // the end of tiles and the beginning of items. (& likewise for monsters: #144)
        if (items_yet) {
            monsters_yet = true;
        } else {
            items_yet = true;
        }
        kf->pop();
    } else if (monsters_yet) {
        const MonsterType * mtype = popMonsterType();
        seg_tile.monsters.push_back(mtype);
    } else if (items_yet) {
        const ItemType *itype = popItemType();
        seg_tile.items.push_back(itype);
    } else {
        shared_ptr<Tile> t = popTile();
        if (t) seg_tile.tiles.push_back(t);
    }
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
    out.reserve(overlays.size() + lua_overlays.size());
    for (std::map<const Value *, Overlay*>::const_iterator it = overlays.begin(); it != overlays.end(); ++it) {
        out.push_back(it->second);
    }
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
    std::vector<const Control*> ctrls;
    GetStandardControls(ctrls);
    out.clear();
    out.reserve(ctrls.size());
    std::copy(ctrls.begin(), ctrls.end(), std::back_inserter(out));
}

void KnightsConfigImpl::getOtherControls(std::vector<const UserControl*> &out) const
{
    out.clear();
    out.reserve(controls.size() + lua_controls.size());
    for (std::map<const Value *, Control*>::const_iterator it = controls.begin(); it != controls.end(); ++it) {
        out.push_back(it->second);
    }
    for (std::vector<Control*>::const_iterator it = lua_controls.begin(); it != lua_controls.end(); ++it) {
        out.push_back(*it);
    }
    std::sort(out.begin(), out.end(), CompareID<UserControl>());
}

std::string KnightsConfigImpl::initializeGame(const MenuSelections &msel,
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
                                              int &final_gvt) const
{
    boost::scoped_ptr<DungeonGenerator> dg(new DungeonGenerator(lua_state.get(),
                                                                segment_set,
                                                                wall_tile,
                                                                horiz_door_tile[0],
                                                                horiz_door_tile[1],
                                                                vert_door_tile[0],
                                                                vert_door_tile[1]));
    quests.clear();
    readMenu(menu, msel, *dg, quests);   // apply DungeonDirectives and get quests
    dungeon_map.reset(new DungeonMap);
    coord_transform.reset(new CoordTransform);
    const std::string dungeon_generator_msg =
        dg->generate(*dungeon_map, monster_manager, *coord_transform, hse_cols.size(), tutorial_manager != 0);

    // add QuestEscape if need be
    CreateEscapeQuest(dg->getExitType(), quests);

    // Add homes to the home manager
    for (int i = 0; i < dg->getNumHomesOverall(); ++i) {
        MapCoord pos;
        MapDirection facing_toward_home;
        dg->getHomeOverall(i, pos, facing_toward_home);
        home_manager.addHome(pos, facing_toward_home);
    }

    // Add players
    players.clear();

    MapCoord pos;
    MapDirection facing;

    for (int i = 0; i < hse_cols.size(); ++i) {
        dg->getHome(i, pos, facing);
        const int team_num = hse_cols[i];
        boost::shared_ptr<Player> player(new Player(i, *dungeon_map, pos, facing,
                                                    knight_anims.at(hse_cols[i]), default_item,
                                                    0, control_set, quests,
                                                    secured_cc.at(hse_cols[i]),
                                                    player_names[i],
                                                    team_num));
        if (pos.isNull()) player->setRespawnType(Player::R_RANDOM_SQUARE);
        if (dg->randomizeHomeOnDeath()) player->setRespawnType(Player::R_DIFFERENT_EVERY_TIME);

        switch (dg->getExitType()) {
        case E_SELF:
            player->setExitFromPlayersHome(*player);
            break;
        case E_OTHER:
            // Do nothing
            break;
        default:
            dg->getExit(i, pos, facing);
            player->setExit(pos, facing);
            break;
        }

        players.push_back(player);
    }
    
    if (dg->getExitType() == E_OTHER) {
        for (int i = 0; i < players.size(); ++i) {
            // For each player i, choose a random player j (j != i)
            // and assign home j to player i.
            int j = g_rng.getInt(0, players.size() - 1);
            if (j >= i) ++j;
            players[i]->setExitFromPlayersHome(*players[j]);
        }
    }

    // set up stuff manager
    stuff_manager.setStuffBagGraphic(stuff_bag_graphic);
    stuff_manager.setDungeonMap(dungeon_map.get());

    // set up gore manager
    if (dead_knight_tiles.size() >= 4 * house_colours_normal.size()) {
        for (int i = 0; i < players.size(); ++i) {
            const int offset = i%2;
            gore_manager.setKnightCorpse(*players[i], dead_knight_tiles[hse_cols[i] * 4 + offset], dead_knight_tiles[hse_cols[i] * 4 + offset + 1]);
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

    // set up monster manager - tile-generated monsters

    // TODO: There should not be a single "tile generated monster level" setting, rather there should be one
    // per monster type;
    // however, there is currently only one tile generated monster type (the vampire bat), so we can get
    // away with this for now...
    // NOTE: the generation chance is linear from 0% to 100%, as generation level goes from 0 to 5.
    const float tile_generated_monster_chance = 0.2f * dg->getTileGeneratedMonsterLevel();
    
    for (std::map<MonsterType *, std::vector<shared_ptr<Tile> > >::const_iterator it = monster_generator_tiles.begin();
    it != monster_generator_tiles.end(); ++it) {
        for (std::vector<shared_ptr<Tile> >::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            monster_manager.addMonsterGenerator(*it2,    // tile
                                                it->first,  // monster type
                                                tile_generated_monster_chance);
        }
    }

    // set up monster manager - zombie activity
    for (std::vector<ZombieActivityEntry>::const_iterator it = zombie_activity.begin(); it != zombie_activity.end(); ++it) {
        addZombieActivity(monster_manager, it->from, *it);

        // if from_tile is one of the dead knight tiles, we have to make sure that the duplicates for additional house colours
        // are also dealt with properly.
        const size_t dead_kt_size = dead_knight_tiles.size() / house_colours_normal.size();
        for (size_t j = 0; j < dead_kt_size; ++j) {
            if (it->from == dead_knight_tiles[j]) {
                for (size_t i = 1; i < house_colours_normal.size(); ++i) {
                    size_t idx = i * dead_kt_size + j;
                    addZombieActivity(monster_manager, dead_knight_tiles[idx], *it);
                }
            }
        }
    }

    const int zombie_activity = dg->getZombieActivity();
    monster_manager.setZombieChance(0.04f*zombie_activity*zombie_activity);   // quadratic from 0 to 100%

    // Monster limits
    //   -- Total number of monsters is limited to CfgInt("monster_limit").
    //   -- Individual monster types are set elsewhere (DungeonGenerator::generate()).
    monster_manager.limitTotalMonsters(config_map->getInt("monster_limit"));

    monster_manager.setRespawnWait(config_map->getInt("monster_respawn_wait"));
    
    // set up event manager
    event_manager.setupHooks(hooks);

    // set up a task to run the monster manager every so often
    shared_ptr<MonsterTask> mtsk (new MonsterTask);
    task_manager.addTask(mtsk, TP_LOW, task_manager.getGVT()+1);

    // set "premapped" flag
    premapped = dg->isPremapped();

    // set starting gears
    starting_gears = dg->getStartingGears();

    // set up a task to respawn items
    boost::shared_ptr<ItemRespawnTask> itsk(new ItemRespawnTask(dg->getRespawnItems(),
                                                                dg->getItemRespawnTime(),
                                                                config_map->getInt("item_respawn_interval"),
                                                                dg->getLockpicks(),
                                                                dg->getLockpickInitialTime(),
                                                                dg->getLockpickInterval()));
    task_manager.addTask(itsk, TP_NORMAL, task_manager.getGVT()+1);

    // set up a task to check quest items and respawn them if necessary
    const int item_check_interval = config_map->getInt("item_check_interval");
    boost::shared_ptr<ItemCheckTask> chktsk(new ItemCheckTask(*dungeon_map, quests, item_check_interval));
    task_manager.addTask(chktsk, TP_NORMAL, task_manager.getGVT() + item_check_interval);
    
    // set up a time limit task if needed
    const int time_limit = msel.getValue("#time") * 60 * 1000;  // time limit in milliseconds
    if (time_limit > 0) {
        boost::shared_ptr<TimeLimitTask> ttsk(new TimeLimitTask);
        final_gvt = task_manager.getGVT() + time_limit;
        task_manager.addTask(ttsk, TP_NORMAL, final_gvt);
    } else {
        final_gvt = 0;
    }
    
    // Set up tutorial manager if needed
    if (tutorial_manager) {
        for (std::map<int, std::pair<std::string,std::string> >::const_iterator it = tutorial_data.begin(); 
        it != tutorial_data.end(); ++it) {
            tutorial_manager->addTutorialKey(it->first, it->second.first, it->second.second);
        }
    }

    return dungeon_generator_msg;
}

int KnightsConfigImpl::getApproachOffset() const
{
    return config_map->getInt("approach_offset");
}

void KnightsConfigImpl::getHouseColours(std::vector<Coercri::Color> &result) const
{
    result = house_col_vector;
}

std::string KnightsConfigImpl::getQuestDescription(int quest_num, const std::string &exit_point_string) const
{
    if (quest_num < 1 || quest_num > standard_quest_descriptions.size()) return "";

    // return the standard quest description, but replace "%X" with the exit point
    std::string result = standard_quest_descriptions[quest_num - 1];
    for (int i = 0; i < result.size(); ++i) {
        if (result[i] == '%' && i+1 < result.size() && result[i+1] == 'X') {
            result = result.substr(0,i) + exit_point_string + result.substr(i+2, std::string::npos);
        }
    }
    return result;
}

void KnightsConfigImpl::readMenu(const Menu &menu, const MenuSelections &msel, DungeonGenerator &dgen,
                                 std::vector<boost::shared_ptr<Quest> > &quests) const
{
    quests.clear();

    // Global dungeon directives
    for (std::vector<DungeonDirective*>::const_iterator it = global_dungeon_directives.begin(); it != global_dungeon_directives.end(); ++it) {
        (*it)->apply(dgen, msel);
    }

    // MenuItem-specific dungeon directives, and Quests
    for (int i = 0; i < menu.getNumItems(); ++i) {
        const MenuItem &mi = menu.getItem(i);
        const std::string &key = mi.getKey();
        const int val = msel.getValue(key);

        // dungeon directives
        key_val_directives_map::const_iterator iter_to_val_dirs_map = menu_dungeon_directives.find(key);
        if (iter_to_val_dirs_map != menu_dungeon_directives.end()) {
            const val_directives_map & val_dirs_map = iter_to_val_dirs_map->second;
            val_directives_map::const_iterator iter_to_dirs = val_dirs_map.find(val);
            if (iter_to_dirs != val_dirs_map.end()) {
                const std::vector<DungeonDirective*> &dirs = iter_to_dirs->second;
                for (std::vector<DungeonDirective*>::const_iterator it = dirs.begin(); it != dirs.end(); ++it) {
                    (*it)->apply(dgen, msel);
                }
            }
        }

        // quests
        key_val_quests_map::const_iterator iter_to_val_quests_map = menu_quests.find(key);
        if (iter_to_val_quests_map != menu_quests.end()) {
            const val_quests_map &val_quests_map = iter_to_val_quests_map->second;
            val_quests_map::const_iterator iter_to_quests = val_quests_map.find(val);
            if (iter_to_quests != val_quests_map.end()) {
                const std::vector<boost::shared_ptr<Quest> > &the_quests = iter_to_quests->second;
                // copy the quests to the output vector
                std::copy(the_quests.begin(), the_quests.end(), back_inserter(quests));
            }
        }
    }
}



//
// Lua related functions
//

Action * KnightsConfigImpl::addLuaAction(auto_ptr<Action> p)
{
    Action *q = p.release();
    lua_actions.push_back(q);
    return q;
}

Anim * KnightsConfigImpl::addLuaAnim(lua_State *lua, int idx)
{
    const int new_id = lua_anims.size() + 1;
    Anim *q = new Anim(new_id, lua, idx);
    lua_anims.push_back(q);
    return q;
}

Control * KnightsConfigImpl::addLuaControl(auto_ptr<Control> p)
{
    const int new_id = controls.size() + lua_controls.size() + NUM_STANDARD_CONTROLS + 1;
    p->setID(new_id);
    
    Control *q = p.release();
    lua_controls.push_back(q);
    return q;
}

RandomDungeonLayout * KnightsConfigImpl::addLuaDungeonLayout(lua_State *lua)
{
    RandomDungeonLayout *p = new RandomDungeonLayout(lua);
    lua_dungeon_layouts.push_back(p);
    return p;
}
    
void KnightsConfigImpl::addLuaGraphic(auto_ptr<Graphic> p)
{
    ASSERT(dead_knight_graphics.empty());
    
    const int new_id = lua_graphics.size() + 1;
    p->setID(new_id);

    lua_graphics.push_back(p.release());
}

ItemGenerator * KnightsConfigImpl::addLuaItemGenerator(lua_State *lua)
{
    ItemGenerator *ig = new ItemGenerator(lua);
    lua_item_generators.push_back(ig);
    return ig;
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
    const int id = overlays.size() + lua_overlays.size() + 1;
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

Sound * KnightsConfigImpl::addLuaSound(const char *name)
{
    const int new_id = lua_sounds.size() + 1;
    Sound * p = new Sound(new_id, name);
    lua_sounds.push_back(p);
    return p;
}


// KConfig references

void KnightsConfigImpl::kconfigControl(const char *name)
{
    if (!kf) {
        lua_pushnil(lua_state.get());
    } else {
        kf->pushSymbol(name);
        Control * ctrl = popControl();
        NewLuaPtr<Control>(lua_state.get(), ctrl);
    }
}

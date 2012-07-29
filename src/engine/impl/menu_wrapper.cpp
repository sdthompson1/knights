/*
 * menu_wrapper.cpp
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

#include "lua_exec.hpp"
#include "lua_func.hpp"
#include "lua_func_wrapper.hpp"
#include "lua_setup.hpp"
#include "menu.hpp"
#include "menu_listener.hpp"
#include "menu_wrapper.hpp"
#include "rng.hpp"

#include "lua.hpp"

#include <map>
#include <sstream>
#include <vector>

// Data structure definitions.

namespace {
    struct Funcs {
        LuaFunc constrain;
        LuaFunc features;
        LuaFunc on_select;
    };

    struct ChoiceInfo : Funcs {
        // info on min/max players/teams constraints -- note these can
        // only be attached to choices, not items or the whole menu.
        int min_players, max_players;
        int min_teams, max_teams;
    };        

    struct ItemInfo : Funcs {
        // NOTE: choice_info is only non-empty for 'dropdown' fields!
        std::vector<ChoiceInfo> choice_info;
        LuaFunc randomize;
    };
}

struct MenuWrapperImpl {

    // The Menu itself (contains the "gui" defintion of the menu, also
    // useful for getting the number of items/choices)
    Menu menu;

    
    // The table of current menu settings
    LuaRef s_table;

    // The current number of players and teams
    int num_players;
    int num_teams;
    bool strict;  // whether no's of players & teams should be checked strictly.

    
    // The Lua event handler functions (constrain, features, on_select, describe_quest),
    // and min/max players/teams information
    Funcs menu_funcs;
    LuaFunc describe_quest_func;
    LuaFunc prepare_game_func, start_game_func;
    std::vector<ItemInfo> item_info;

    // The mapping between Lua keys and C++ item numbers (done as STL containers)
    std::vector<std::string> item_num_to_key;
    std::map<std::string, int> key_to_item_num;

    // The mapping between Lua values and C++ choice numbers (done as Lua tables, but indexed under item_num)
    // NOTE: these are only non-nil for Dropdown items.
    // NOTE: instead of working with these directly, it may be easier to
    //         use functions PushChoiceVal and PopChoiceVal.
    std::vector<LuaRef> choice_val_to_choice_num;
    std::vector<LuaRef> choice_num_to_choice_val;


    // The currently active constraints
    //  --> constraints[itm] is a sorted list of valid choice numbers for item number itm.
    //  --> Only valid for dropdowns; will be EMPTY for numeric fields.
    std::vector<std::vector<int> > constraints;
};

namespace {

    lua_State * GetLuaState(MenuWrapperImpl &impl)
    {
        return impl.s_table.getLuaState();
    }
    
    // ---------------------------------------------------------------------------

    // Functions to convert between choice nums and choice vals

    // Pops a choice val from top of lua stack, and returns corresponding choice num.
    // If the choice val is invalid (i.e. does not correspond to any of the available choices for this item)
    // then 0 is returned.
    int PopChoiceVal(lua_State *lua, const MenuWrapperImpl &impl, int item_num)
    {
        // [cval]
        
        ASSERT(item_num >= 0 && item_num < impl.menu.getNumItems());

        impl.choice_val_to_choice_num[item_num].push(lua);  // [cval Ctbl]
        lua_insert(lua, -2);  // [Ctbl cval]
        
        if (!lua_isnil(lua, -2)) {
            // Choice val -> choice num table exists
            // Must be a dropdown field
            // NOTE: If cval is not valid this will put 'nil' on the stack top, which will
            // convert to zero in the lua_tointeger call below.
            lua_gettable(lua, -2);  // [Ctbl cnum]
        }
        // ELSE: It is a numeric field; the available choices are any integer.
        // Therefore, whatever lua_tointeger below returns, we guarantee that we are not
        // returning an invalid choice number...
        
        const int result = lua_tointeger(lua, -1);

        lua_pop(lua, 2);  // []
        return result;
    }

    // Inputs choicenum, pushes corresponding choiceval to the lua stack
    // (If this is a dropdown item and the choice_num is out of range, then pushes nil.)
    void PushChoiceVal(lua_State *lua, const MenuWrapperImpl &impl, int item_num, int choice_num)
    {
        ASSERT(item_num >= 0 && item_num < impl.menu.getNumItems());
        impl.choice_num_to_choice_val[item_num].push(lua);  // [Ctbl]
        lua_pushinteger(lua, choice_num);  // [Ctbl choice_num]
        if (!lua_isnil(lua, -2)) {
            // Must be a dropdown
            lua_gettable(lua, -2);  // [Ctbl choice_val]
        }
        lua_remove(lua, -2);   // [choice_val]
    }        
        
    // ---------------------------------------------------------------------------

    // Some helper functions for dealing with the S table.
    
    // Read a setting from the "s_table".
    // Inputs item_num and returns choice_num (as opposed to inputting Lua key and returning Lua value).
    // If the setting in "s_table" is invalid, then returns 0.
    int ItemNumToChoiceNum(lua_State *lua, const MenuWrapperImpl &impl, int item_num)
    {
        ASSERT(item_num >= 0 && item_num < impl.menu.getNumItems());
        
        const char *key = impl.item_num_to_key[item_num].c_str();
        
        impl.s_table.push(lua);   // [S]
        lua_pushstring(lua, key);  // [S item_key]
        lua_gettable(lua, -2);   // [S choice_val]
        int result = PopChoiceVal(lua, impl, item_num);   // [S]
                
        lua_pop(lua, 1);  // []
        return result;
    }

    // Reads out all current choices
    void ReadAllCurrentChoices(lua_State *lua, const MenuWrapperImpl &impl, std::vector<int> &result)
    {
        result.clear();
        const int n = impl.menu.getNumItems();
        result.reserve(n);
        for (int i = 0; i < n; ++i) {
            result.push_back(ItemNumToChoiceNum(lua, impl, i));
        }
    }
    
    // Write a new setting to the "s_table"
    // Inputs are item nums and choice nums (as opposed to Lua keys and values).
    void SetItemNumToChoiceNum(lua_State *lua, MenuWrapperImpl &impl, int item_num, int choice_num)
    {
        ASSERT(item_num >= 0 && item_num < impl.menu.getNumItems());
        ASSERT(impl.menu.getItem(item_num).isNumeric()
            || choice_num >= 0 && choice_num < impl.menu.getItem(item_num).getNumChoices());
        
        impl.s_table.push(lua);  // [S]
        lua_pushstring(lua, impl.item_num_to_key[item_num].c_str());   // [S item_key]
        PushChoiceVal(lua, impl, item_num, choice_num);  // [S item_key choice_val]
        lua_settable(lua, -3);   // [S], and sets S[item_key] = choice_val
        lua_pop(lua, 1);  // []
    }

    // Write a setting to the MenuListener

    void ReportSetting(lua_State *lua, MenuWrapperImpl &impl, int item_num, int choice_num, MenuListener &listener)
    {
        ASSERT(item_num >= 0 && item_num < impl.menu.getNumItems());

        const char * item_key = impl.item_num_to_key[item_num].c_str();
        const char * choice_string = 0;
        
        PushChoiceVal(lua, impl, item_num, choice_num);  // [choiceval]
        choice_string = luaL_tolstring(lua, -1, 0);  // [choiceval string]
                
        listener.settingChanged(item_num, item_key, choice_num, choice_string,
                                impl.constraints[item_num]);

        lua_pop(lua, 2);   // []
    }

    std::string GetQuestDescription(lua_State *lua, const MenuWrapperImpl &impl)
    {
        impl.s_table.push(lua);    // [S]
        if (impl.describe_quest_func.hasValue()) {
            return impl.describe_quest_func.runOneArgToString();   // []
        } else {
            lua_pop(lua, 1);  // []
            return std::string();
        }
    }

    // ---------------------------------------------------------------------------
    
    // Helper to call the "constrain", "on_select" or "features" functions.

    // Two arguments are popped from the lua stack.
    // No results are pushed.
    
    void CallAllFuncs(lua_State *lua,
                      const MenuWrapperImpl &impl,
                      LuaFunc Funcs::* which_func,
                      int restrict_to_item = -1)
    {
        // [a1 a2]

        // always run the menu-level func (if there is one)
        (impl.menu_funcs.*which_func).runNArgsNoPop(lua, 2);

        // for each item
        for (int i = 0; i < impl.menu.getNumItems(); ++i) {

            if (restrict_to_item == -1 || restrict_to_item == i) {
            
                // always run all the item-level funcs
                (impl.item_info[i].*which_func).runNArgsNoPop(lua, 2);
                
                // run the choice-level func, for the currently selected choice only
                if (!impl.item_info[i].choice_info.empty()) {
                    const int j = ItemNumToChoiceNum(lua, impl, i);
                    (impl.item_info[i].choice_info[j].*which_func).runNArgsNoPop(lua, 2);
                }
            }
        }
        
        // finally pop the two args
        lua_pop(lua, 2);
    }


    // ----------------------------------------------------------------------

    // This is the implementation of the constraint functions (Is, IsAtLeast, etc)
    // (These are called by UpdateConstraints.)
    
    std::vector<int> & ConstraintHelp(lua_State *lua, int &item_num, int &choice_num)
    {
        MenuWrapperImpl &impl = *static_cast<MenuWrapperImpl*>(lua_touserdata(lua, lua_upvalueindex(2)));

        // args: <possible tbl>, key, val
        int argpos = 1;
        if (lua_istable(lua, 1) && lua_gettop(lua) > 2) {
            argpos = 2;
        }

        // fetch the item num for this key
        const char * key = luaL_checkstring(lua, argpos);
        std::map<std::string,int>::const_iterator find_key = impl.key_to_item_num.find(key);
        if (find_key == impl.key_to_item_num.end()) {
            luaL_error(lua, "Error in constraint function: Menu item '%s' does not exist", key);
        }
        item_num = find_key->second;
        std::vector<int> &constraint_vec = impl.constraints[item_num];
        
        // convert the lua value (arg_pos+1) into a choice num
        // note: do this directly (rather than using PopChoiceVal) because we want to handle the case of
        // an invalid value being passed to us.
        impl.choice_val_to_choice_num[item_num].push(lua);  // [C]
        if (lua_isnil(lua, -1)) {
            // must be a numeric item.
            choice_num = lua_tointeger(lua, argpos+1);
        } else {
            lua_pushvalue(lua, argpos+1);  // [C v]
            lua_gettable(lua, -2);   // [C choice_num_or_nil]
            if (lua_isnil(lua, -1)) {
                luaL_error(lua,
                           "Error in constraint function: %s is not a valid choice for '%s'",
                           luaL_tolstring(lua, argpos+1, 0),
                           key);
            }
            choice_num = lua_tointeger(lua, -1);
            lua_pop(lua, 1);  // [C]
        }

        lua_pop(lua, 1);  // [] (except args)
        return constraint_vec;
    }


    // the following constraint functions have one upvalue (at upvalue
    // index 2) which is a pointer to the MenuWrapperImpl

    // also they take two arguments: a key, and a value.
    // (can be an optional table argument before those -- allows accidental S: instead of S. syntax)
        
    int Is(lua_State *lua)
    {
        int item_num, choice_num;
        std::vector<int> &constraints =
            ConstraintHelp(lua, item_num, choice_num);

        // remove all choices but the chosen one
        const bool found = std::binary_search(constraints.begin(), constraints.end(), choice_num);
        constraints.clear();
        if (found) {
            constraints.push_back(choice_num);
        }

        return 0;
    }

    int IsNot(lua_State *lua)
    {
        int item_num, choice_num;
        std::vector<int> &constraints = ConstraintHelp(lua, item_num, choice_num);

        // remove the chosen choice if it exists
        std::vector<int>::iterator find = std::lower_bound(constraints.begin(), constraints.end(), choice_num);
        if (*find == choice_num) {
            constraints.erase(find);
        }
        return 0;
    }

    int IsAtLeast(lua_State *lua)
    {
        int item_num, choice_num;
        std::vector<int> &constraints = ConstraintHelp(lua, item_num, choice_num);

        // remove everything up to, but not including, lower_bound.
        std::vector<int>::iterator find = std::lower_bound(constraints.begin(), constraints.end(), choice_num);
        constraints.erase(constraints.begin(), find);
        return 0;
    }

    int IsAtMost(lua_State *lua)
    {
        int item_num, choice_num;
        std::vector<int> &constraints = ConstraintHelp(lua, item_num, choice_num);

        // remove everything from upper_bound onwards
        std::vector<int>::iterator find = std::upper_bound(constraints.begin(), constraints.end(), choice_num);
        constraints.erase(find, constraints.end());
        return 0;
    }


    // ----------------------------------------------------------------------

    // Check the number of players/teams constraint for a particular menu choice.
    // Called by UpdateConstraints.
    bool CheckNumPlayers(MenuWrapperImpl &impl, int itemno, int choiceno)
    {
        ASSERT(itemno >= 0 && itemno < impl.menu.getNumItems());
        if (impl.item_info[itemno].choice_info.empty()) {
            return true;
        } else {
            ASSERT(choiceno >= 0 && choiceno < impl.menu.getItem(itemno).getNumChoices());
            const ChoiceInfo & info = impl.item_info[itemno].choice_info[choiceno];

            return (impl.num_players >= info.min_players || !impl.strict)
                && impl.num_players <= info.max_players
                && (impl.num_teams >= info.min_teams || !impl.strict)
                && impl.num_teams <= info.max_teams;
        }
    }
    
    // Make sure the 'constraints' vector is up to date.
    // Does not change S.

    void UpdateConstraints(lua_State *lua, MenuWrapperImpl &impl)
    {
        // First wipe all existing constraints. Everything is allowed unless someone says otherwise
        // (or unless there is a number-of-players type constraint)
        for (int i = 0; i < impl.menu.getNumItems(); ++i) {
            impl.constraints[i].clear();

            if (!impl.menu.getItem(i).isNumeric()) {
                const int sz = impl.menu.getItem(i).getNumChoices();
                impl.constraints[i].reserve(sz);
                for (int j = 0; j < sz; ++j) {
                    // check whether the number of players/teams constraints allows this choice.
                    if (CheckNumPlayers(impl, i, j)) {
                        impl.constraints[i].push_back(j);
                    }
                }
            }
        }
        
        // Now call all the lua constrain functions. This will set up
        // the constraints vector correctly.
        impl.s_table.push(lua);  // [S]
        lua_pushnil(lua);        // [S nil]
        CallAllFuncs(lua, impl, &Funcs::constrain);   // []
    }

    // Validate a single item; called by Validate
    // Returns true if the setting was changed.
    
    bool ValidateItem(lua_State *lua, MenuWrapperImpl &impl, int item_num)
    {
        ASSERT(item_num >= 0 && item_num < impl.menu.getNumItems());

        const std::vector<int> &valid_values = impl.constraints[item_num];  // empty for numeric values.
        const int current_choice = ItemNumToChoiceNum(lua, impl, item_num);
            
        if (!std::binary_search(valid_values.begin(), valid_values.end(), current_choice)) {
            // EITHER: Not a valid choice, OR: it is a numeric field
            // Set it to the first valid choice (if there is one -- only applies for dropdowns).
            if (!valid_values.empty()) {
                SetItemNumToChoiceNum(lua, impl, item_num, valid_values.front());
                return true;
            } else {
                // EITHER: it is a "bad" dropdown (the constraints forbid all the available choices)
                // OR: it is a numeric field.
                // Set it to its current value (or zero if it didn't have a current value).
                // This will ensure the Lua choice_val is valid, at least.
                SetItemNumToChoiceNum(lua, impl, item_num, current_choice);
            }
        }
        
        return false;
    }
        
    // Validate the menu settings. Ensures that the 'constraints'
    // vector is correct, and also that the S table is consistent with
    // the constraints.

    void Validate(lua_State *lua, MenuWrapperImpl &impl)
    {
        // Max number of iterations to prevent loops.
        for (int iter = 0; iter < 25; ++iter) {

            // First make sure the constraints are up to date
            UpdateConstraints(lua, impl);
            
            // Now ensure each item has a valid choice selected.
            bool changed = false;
            for (int i = 0; i < impl.menu.getNumItems(); ++i) {
                changed = ValidateItem(lua, impl, i) || changed;
            }

            // If all choices were already valid, then we are done.
            // Otherwise, go round for another pass.
            if (!changed) break;
        }

        // (If we run out of iterations, the menu is left in an
        // invalid state.)
    }

    // -------------------------------------------------------------------------------

    // Functions to report changes to the MenuListener.

    struct OldSettings {
        std::vector<int> choices;
        std::vector<std::vector<int> > constraints;
        std::string quest_description;
    };

    void SaveOldSettings(lua_State *lua, const MenuWrapperImpl &impl, OldSettings &out)
    {
        ReadAllCurrentChoices(lua, impl, out.choices);
        out.constraints = impl.constraints;
        out.quest_description = GetQuestDescription(lua, impl);
    }

    void Report(lua_State *lua, MenuWrapperImpl &impl,
                const OldSettings &old,
                MenuListener &listener)
    {
        std::vector<int> new_choices;
        ReadAllCurrentChoices(lua, impl, new_choices);
        const std::vector<std::vector<int> > & new_constraints = impl.constraints;

        for (int i = 0; i < impl.menu.getNumItems(); ++i) {
            if (old.choices[i] != new_choices[i]
            || old.constraints[i] != new_constraints[i]) {
                ReportSetting(lua, impl, i, new_choices[i], listener);
            }
        }

        // describe the new quest, as well
        std::string new_quest = GetQuestDescription(lua, impl);
        if (new_quest != old.quest_description) {
            listener.questDescriptionChanged(new_quest);
        }
    }

    void ValidateAndReport(lua_State *lua, MenuWrapperImpl &impl, 
                           const OldSettings &old,
                           MenuListener &listener)
    {
        Validate(lua, impl);
        Report(lua, impl, old, listener);
    }
    
    // ------------------------------------------------------------------------------

    // Functions for initialization / reading the kts.MENU table.

    void ReadFuncs(lua_State *lua, Funcs &out)
    {
        // Top of stack contains table with 'on_select', 'constrain' and 'features' (all optional).
        out.on_select.reset(lua, -1, "on_select");
        out.constrain.reset(lua, -1, "constrain");
        out.features.reset(lua, -1, "features");
    }
    
    std::string PopChoice(lua_State *lua, int choice_num, const LuaFunc & func, ChoiceInfo &choice_info,
                          const char *key)
    {
        // [S _ ValToNum NumToVal choicetbl]

        lua_getfield(lua, -1, "id");  // [S _ V N c choiceval]
        if (lua_isnil(lua, -1)) {
            luaL_error(lua, "Menu Choice is missing 'id' field");
        }
        lua_pushvalue(lua, -1);   // [S _ V N c choiceval choiceval]
        lua_rawseti(lua, -4, choice_num);  // [S _ V N c choiceval]   (and set num->val mapping)

        std::string result;
        if (func.hasValue()) {
            lua_pushvalue(lua, -1);  // [S _ V N c choiceval choiceval]
            result = func.runOneArgToString();   // [S _ V N c choiceval]
        } else {
            result = LuaGetString(lua, -2, "text");  // [S _ V N c choiceval]
        }

        if (key) {
            lua_pushvalue(lua, -1);  // [S _ V N c choiceval choiceval]
            lua_setfield(lua, -7, key);  // [S _ V N c choiceval]
        }
        
        lua_pushinteger(lua, choice_num);  // [S _ V N c choiceval choicenum]
        lua_rawset(lua, -5);    // [S _ V N c] and sets val->num mapping        

        ReadFuncs(lua, choice_info);
        choice_info.min_players = LuaGetInt(lua, -1, "min_players", 0);
        choice_info.max_players = LuaGetInt(lua, -1, "max_players", 9999999);
        choice_info.min_teams   = LuaGetInt(lua, -1, "min_teams",   0);
        choice_info.max_teams   = LuaGetInt(lua, -1, "max_teams",   9999999);
        
        lua_pop(lua, 1);  // [S _ V N]
        return result;
    }

    void PopMenuItem(lua_State *lua, MenuWrapperImpl &impl)
    {
        // [item]

        Menu &menu = impl.menu;

        const int item_num = menu.getNumItems();

        // try as string first.
        const char *as_string = lua_tostring(lua, -1);
        if (as_string) {
            if (std::strcmp(as_string, "spacer") == 0) {
                if (item_num != 0) menu.getItem(item_num - 1).addSpaceAfter();
                lua_pop(lua, 1);
                return;
            } else {
                luaL_error(lua, "'%s' is not a valid menu item, expecting table or 'spacer'", as_string);
            }
        }
        
        // ok, try reading it as a table.
        
        lua_getfield(lua, -1, "id");  // [item id]
        if (!lua_isstring(lua, -1)) {
            luaL_error(lua, "Menu item is missing 'id' field");
        }
        const std::string key = lua_tostring(lua, -1);
        lua_pop(lua, 1);  // [item]

        const std::string title = LuaGetString(lua, -1, "text");
        
        bool numeric = false;
        lua_getfield(lua, -1, "type");  // [item typestr]
        const char *p = lua_tostring(lua, -1);
        if (p) {
            if (std::strcmp(p, "numeric") == 0) {
                numeric = true;
            } else if (std::strcmp(p, "dropdown") != 0) {
                luaL_error(lua, "Bad menu type '%s', must be 'numeric' or 'dropdown'", p);
            }
        }
        lua_pop(lua, 1);   // [item]

        impl.item_info.push_back(ItemInfo());
        ReadFuncs(lua, impl.item_info.back());
        impl.item_info.back().randomize.reset(lua, -1, "randomize");

        // set default in S table
        impl.s_table.push(lua);  // [item S]
        lua_getfield(lua, -2, "default");  // [item S default]
        bool require_default;
        if (lua_isnil(lua, -1)) {
            require_default = true;
            lua_pop(lua, 1);  // [item S]
        } else {
            require_default = false;
            lua_setfield(lua, -2, key.c_str());  // [item S]
        }

        if (numeric) {
            // numeric box.
            
            const int digits = LuaGetInt(lua, -2, "digits");  // [item S]
            const std::string suffix = LuaGetString(lua, -2, "suffix");  // [item S]

            // add to the menu
            menu.addItem(MenuItem(title, digits, suffix));

            // add dummy refs to choice_val vectors
            impl.choice_val_to_choice_num.push_back(LuaRef());
            impl.choice_num_to_choice_val.push_back(LuaRef());

            if (require_default) {
                lua_pushinteger(lua, 0);  // [item S 0]
                lua_setfield(lua, -2, key.c_str()); // [item S]
            }

        } else {
            // dropdown -- read choices.
            
            bool cmin_set = false, cmax_set = false, choices_set = false;
            int choice_min, choice_max;
            
            lua_getfield(lua, -2, "choice_min");  // [item S cmin]
            if (!lua_isnil(lua, -1)) {
                choice_min = lua_tointeger(lua, -1);
                cmin_set = true;
            }
            lua_pop(lua, 1);  // [item S]
            lua_getfield(lua, -2, "choice_max");  // [item S cmax]
            if (!lua_isnil(lua, -1)) {
                choice_max = lua_tointeger(lua, -1);
                cmax_set = true;
            }
            lua_pop(lua, 1);  // [item S]

            // find out if there is a 'show' func
            lua_getfield(lua, -2, "show");  // [item S showfunc]
            LuaFunc show_func(lua);  // [item S]

            // read the 'choices' subtable
            lua_getfield(lua, -2, "choices");  // [item S choices]
            choices_set = !lua_isnil(lua, -1);

            // error checks
            if (cmin_set && !cmax_set) {
                luaL_error(lua, "Can't set 'choice_min' without 'choice_max'");
            } else if (cmax_set && !cmin_set) {
                luaL_error(lua, "Can't set 'choice_max' without 'choice_min'");
            } else if (choices_set && cmin_set) {
                luaL_error(lua, "Can't set 'choices' at same time as 'choice_min' and 'choice_max'");
            } else if (!choices_set && !cmin_set) {
                luaL_error(lua, "Menu item must have either 'choices' or 'choice_min' / 'choice_max'");
            }
            
            // create new tables for our choice_val and choice_num maps
            lua_newtable(lua);   // [item S choices val_to_num]
            lua_newtable(lua);   // [item S choices val_to_num num_to_val]
            
            // get the value strings
            std::vector<std::string> values;
            if (cmin_set) {
                for (int i = choice_min; i <= choice_max; ++i) {
                    const int choice_num = i - choice_min;
                    const int choice_val = i;

                    // The text string is just the choice_val (unless a 'show' function is present)
                    if (show_func.hasValue()) {
                        values.push_back(show_func.runIntToString(i));
                    } else {
                        std::ostringstream str;
                        str << choice_val;
                        values.push_back(str.str());
                    }

                    // Set the entries in the choice_num/choice_val tables
                    lua_pushinteger(lua, choice_val);  // [i S choices V N choiceval]
                    lua_rawseti(lua, -2, choice_num);  // [i S choices V N]
                    lua_pushinteger(lua, choice_num);  // [i S choices V N choicenum]
                    lua_rawseti(lua, -3, choice_val);  // [i S choices V N]
                }

                if (require_default) {
                    // set the default in 'S' table
                    lua_pushinteger(lua, choice_min);   // [i S choices V N default]
                    lua_setfield(lua, -5, key.c_str()); // [i S choices V N]
                }
                
            } else {
                // [item S choices V N]
                // read 'choices' table
                lua_len(lua, -3);  // [i S choices V N len]
                const int sz = lua_tointeger(lua, -1);
                lua_pop(lua, 1);  // [i S choices V N]
                for (int i = 1; i <= sz; ++i) {
                    const int choice_num = i - 1;
                    lua_pushinteger(lua, i);  // [i S choices V N idx]
                    lua_gettable(lua, -4);    // [i S choices V N choice]

                    impl.item_info.back().choice_info.push_back(ChoiceInfo());
                    
                    // The next line gets the gui text and puts it into 'values'.
                    // It also sets up entries in V and N tables.
                    // It also populates the ChoiceInfo object.
                    // It also sets the default in S table if required.
                    values.push_back(PopChoice(lua,
                                               choice_num,
                                               show_func,
                                               impl.item_info.back().choice_info.back(),
                                               require_default ? key.c_str() : 0));       // [i S c V N]
                    require_default = false; // only 1st choice should be set as default
                }
            }

            impl.choice_num_to_choice_val.push_back(LuaRef(lua));  // [i S choices V]
            impl.choice_val_to_choice_num.push_back(LuaRef(lua));  // [i S choices]
            
            lua_pop(lua, 1);  // [item S]

            // Add to the menu
            menu.addItem(MenuItem(title, values));            
        }

        // Some final setup
        
        impl.item_num_to_key.push_back(key);
        impl.key_to_item_num[key] = item_num;

        // initially there are no 'allowed values' for any setting (will be corrected below)
        impl.constraints.push_back(std::vector<int>());
        
        lua_pop(lua, 2);  // []
    }

    int ReadMenuWork(lua_State *lua)
    {
        // [userdata menutbl]
        
        MenuWrapperImpl &impl = *static_cast<MenuWrapperImpl*>(lua_touserdata(lua, 1));

        impl.num_players = 0;
        impl.num_teams = 0;

        // create table "S"
        lua_newtable(lua);  // [m S]
        
        lua_pushlightuserdata(lua, &impl); // [m S pimpl]
        PushCClosure(lua, &Is, 1);  // [m S func]
        lua_setfield(lua, -2, "Is");  // [m S]

        lua_pushlightuserdata(lua, &impl);
        PushCClosure(lua, &IsNot, 1);
        lua_setfield(lua, -2, "IsNot");

        lua_pushlightuserdata(lua, &impl);
        PushCClosure(lua, &IsAtLeast, 1);
        lua_setfield(lua, -2, "IsAtLeast");

        lua_pushlightuserdata(lua, &impl);
        PushCClosure(lua, &IsAtMost, 1);
        lua_setfield(lua, -2, "IsAtMost");  // [m S]

        impl.s_table.reset(lua);  // [m]        

        // read menu-level functions
        ReadFuncs(lua, impl.menu_funcs);

        // populate the Menu object
        Menu &menu = impl.menu;

        menu.setTitle(LuaGetString(lua, -1, "text"));

        lua_getfield(lua, -1, "items"); // [menutbl itemstbl]
        if (lua_isnil(lua, -1)) {
            luaL_error(lua, "Menu is missing 'items' field");
        }
        lua_len(lua, -1);  // [menu items len]
        const int sz = lua_tointeger(lua, -1);
        lua_pop(lua, 1);  // [menu items]
        for (int i = 1; i <= sz; ++i) {
            lua_pushinteger(lua, i);  // [menu items i]
            lua_gettable(lua, -2);   // [menu items item]
            PopMenuItem(lua, impl);   // [menu items]
        }

        lua_pop(lua, 1);   // [menu]

        // Store refs to menu-level functions
        impl.describe_quest_func.reset(lua, -1, "describe_quest_func");    // [m]
        impl.prepare_game_func.reset(lua, -1, "prepare_game_func");
        impl.start_game_func.reset(lua, -1, "start_game_func");
        
        // Now call the initialization function (if present).

        lua_getfield(lua, -1, "initialize_func");   // [m initfunc]
        if (!lua_isnil(lua, -1)) {
            impl.s_table.push(lua);   // [m func S]
            LuaExec(lua, 1, 0);       // [m]
        } else {
            lua_pop(lua, 1); // [m]
        }

        // Now validate the initial state
        Validate(lua, impl);
        
        return 0;
    }

    // ----------------------------------------------------

    int DoGameStartup(lua_State *lua)
    {
        MenuWrapperImpl *pimpl = static_cast<MenuWrapperImpl*>(lua_touserdata(lua, 1));
        
        // Call 'prepare_game_func' (parameter S)
        pimpl->s_table.push(lua);   // [S]
        pimpl->prepare_game_func.runNArgsNoPop(lua, 1);  // [S]
    
        // Call all the 'features' functions. (pass parameter S)
        lua_pushnil(lua);           // [S nil]
        CallAllFuncs(lua, *pimpl, &Funcs::features);  // []

        // Call 'start_game_func' (parameter S)
        pimpl->s_table.push(lua);  // [S]
        pimpl->start_game_func.runNArgsNoPop(lua, 1);  // [S]
        lua_pop(lua, 1); // []

        // See if DUNGEON_ERROR is set.
        lua_getglobal(lua, "kts");  // [kts]
        lua_getfield(lua, -1, "DUNGEON_ERROR");  // [kts err]
        if (!lua_isnil(lua, -1)) {
            lua_error(lua);   // throw the err msg as an error
        }

        return 0;
    }
}

// ----------------------------------------------------

// The constructor.
    
// Pops a menu table from the top of the stack.

MenuWrapper::MenuWrapper(lua_State *lua, bool strict)
    : pimpl(new MenuWrapperImpl)
{
    pimpl->strict = strict;
    
    // Wrap this in a Lua call, because we might get Lua errors while
    // accessing the menu table, and we want to trap them (rather than
    // just panicking).

    PushCFunction(lua, &ReadMenuWork);  // [menutbl func]
    lua_pushlightuserdata(lua, &pimpl->menu);  // [menutbl func userdata]
    lua_pushvalue(lua, -3);             // [menutbl func userdata menutbl]
    LuaExec(lua, 2, 0);                 // [menutbl]

    lua_pop(lua, 1);  // []
}


// ----------------------------------------------------

// Implementation of public member functions.

const Menu & MenuWrapper::getMenu() const
{
    return pimpl->menu;
}

bool MenuWrapper::getStrict() const
{
    return pimpl->strict;
}

void MenuWrapper::getCurrentSettings(MenuListener &listener) const
{
    lua_State *lua = GetLuaState(*pimpl);
    for (int item = 0; item < pimpl->menu.getNumItems(); ++item) {
        const int choice = ItemNumToChoiceNum(lua, *pimpl, item);
        ReportSetting(lua, *pimpl, item, choice, listener);
    }
    listener.questDescriptionChanged(GetQuestDescription(lua, *pimpl));
}

void MenuWrapper::changeSetting(int item_num, int new_choice_num, MenuListener &listener)
{
    // Validate the inputs
    const Menu &menu = pimpl->menu;
    if (item_num < 0 || item_num >= menu.getNumItems()) return;
    const MenuItem &item = pimpl->menu.getItem(item_num);
    if (!item.isNumeric() && (new_choice_num < 0 || new_choice_num >= item.getNumChoices())) return;
    
    lua_State *lua = GetLuaState(*pimpl);

    // Save old settings
    OldSettings old;
    SaveOldSettings(lua, *pimpl, old);

    // change the setting in S table
    SetItemNumToChoiceNum(lua, *pimpl, item_num, new_choice_num);

    // call the 'on_select' func (note this may change S)
    pimpl->s_table.push(lua);            // [S]
    lua_pushstring(lua, pimpl->item_num_to_key[item_num].c_str());  // [S item_key]
    CallAllFuncs(lua, *pimpl, &Funcs::on_select, item_num);   // []

    // Finally validate the settings and report the change(s) back to the listener.
    ValidateAndReport(lua, *pimpl, old, listener);
}

void MenuWrapper::changeNumberOfPlayers(int nplayers, int nteams, MenuListener &listener)
{
    pimpl->num_players = nplayers;
    pimpl->num_teams = nteams;

    // strictly speaking we don't need to call all the lua constraint
    // functions again, but for the sake of simplicity we want to have
    // only one unified "Validate" function that does everything...
    OldSettings old;
    SaveOldSettings(GetLuaState(*pimpl), *pimpl, old);
    ValidateAndReport(GetLuaState(*pimpl), *pimpl, old, listener);
}

bool MenuWrapper::checkNumPlayersStrict(std::string &err_msg) const
{
    // NOTE: We assume menu is in an acceptable state.
    // We simply want to check the min. players/teams constraints.

    lua_State *lua = GetLuaState(*pimpl);

    std::ostringstream str;
    bool ok = true;
    
    for (int item = 0; item < pimpl->menu.getNumItems(); ++item) {
        if (!pimpl->item_info[item].choice_info.empty()) {
            const int choice = ItemNumToChoiceNum(lua, *pimpl, item);
            const ChoiceInfo &info = pimpl->item_info[item].choice_info[choice];

            if (pimpl->num_players < info.min_players) {
                ok = false;
                str << info.min_players << " players.";
            } else if (pimpl->num_teams < info.min_teams) {
                ok = false;
                str << info.min_teams << " teams.";
            }

            if (!ok) {
                std::ostringstream str2;
                const MenuItem &it = pimpl->menu.getItem(item);
                str2 << "ERROR: " << it.getTitleString() << " = " << it.getChoiceString(choice)
                     << " requires at least " << str.str();
                err_msg = str2.str();
                return false;
            }
        }
    }

    return true;
}

void MenuWrapper::randomQuest(MenuListener &listener)
{
    lua_State *lua = GetLuaState(*pimpl);
    RNG_Wrapper myrng(g_rng);

    // Save the old settings
    OldSettings old;
    SaveOldSettings(lua, *pimpl, old);

    // Make a vector of menu item numbers. Can do this once at the beginning.
    std::vector<int> item_nos;
    item_nos.reserve(pimpl->menu.getNumItems());
    for (int i = 0; i < pimpl->menu.getNumItems(); ++i) {
        item_nos.push_back(i);
    }

    // Iterate a number of times, to make sure we get a good randomization
    for (int iterations = 0; iterations < 3; ++iterations) {

        // Shuffle the menu item numbers into a random order
        // Note: we are using the global rng (from the main thread), as opposed to
        // the rng's from the game threads. This should mean that the replay feature
        // is not messed up by the extra random numbers being generated here.
        std::random_shuffle(item_nos.begin(), item_nos.end(), myrng);

        // For each menu item in the random ordering:
        for (std::vector<int>::const_iterator item_no = item_nos.begin(); item_no != item_nos.end(); ++item_no) {

            int new_value;
            bool new_value_set = false;

            // If this item has a randomize function, then call it
            if (pimpl->item_info[*item_no].randomize.hasValue()) {
                // []
                pimpl->s_table.push(lua);
                lua_pushinteger(lua, pimpl->num_players);
                lua_pushinteger(lua, pimpl->num_teams);  // [S nplayers nteams]
                pimpl->item_info[*item_no].randomize.run(lua, 3, 1);  // [result]
                new_value = PopChoiceVal(lua, *pimpl, *item_no);    // []
                new_value_set = true;
                
            } else {
                // find out the allowed values
                std::vector<int> allowed_values = pimpl->constraints[*item_no];

                // If allowed_values is empty, this is either a
                // dropdown where all choices are forbidden, or a
                // numeric field. In either case, we just leave it
                // alone.
                if (!allowed_values.empty()) {
                    // pick one at random
                    new_value = allowed_values[g_rng.getInt(0, allowed_values.size())];
                    new_value_set = true;
                }
            }

            if (new_value_set) {
                // set it to that value
                SetItemNumToChoiceNum(lua, *pimpl, *item_no, new_value);

                // Revalidate settings before continuing
                Validate(lua, *pimpl);
            }
        }
    }

    // Report the changed settings to the listener.
    Report(lua, *pimpl, old, listener);
}

bool MenuWrapper::runGameStartup(std::string &err_msg)
{
    lua_State *lua = GetLuaState(*pimpl);

    PushCFunction(lua, &DoGameStartup);
    lua_pushlightuserdata(lua, pimpl.get());
    const int result = lua_pcall(lua, 1, 0, 0);
    
    if (result == LUA_OK) {
        return true;
    } else {
        err_msg = luaL_tolstring(lua, -1, 0);
        lua_pop(lua, 1);
        return false;
    }
}

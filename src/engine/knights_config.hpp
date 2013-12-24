/*
 * knights_config.hpp
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
 * Class representing the setup and configuration of a single game of
 * Knights.
 * 
 * This class knows how to generate the dungeon and perform other
 * initialization tasks. This is used by KnightsEngine.
 *
 * NOTE: Have to be a little careful with thread safety as
 * KnightsConfigs can be shared between multiple KnightsEngines. (In
 * particular note that things like the contained Graphics and Anims
 * are referred to throughout the whole game, not just during
 * initialization!) However we only require concurrent *read* access,
 * as KnightsEngine does not write to the config at all. Therefore, we
 * should be OK, as long as (a) we are careful not to use any
 * "mutable" variables, and (b) the standard library (and in
 * particular STL containers) are thread safe for multiple concurrent
 * reads to the same container.
 * 
 */

#ifndef KNIGHTS_CONFIG_HPP
#define KNIGHTS_CONFIG_HPP

#include "gfx/color.hpp"

#include "boost/shared_ptr.hpp"
#include <string>
#include <vector>

class Anim;
class ConfigMap;
class DungeonGenerator;
class EventManager;
class GoreManager;
class Graphic;
class HomeManager;
class ItemType;
class KnightsConfigImpl;
class KnightsEngine;
class Menu;
class MenuListener;
class MonsterManager;
class Overlay;
class Player;
class Sound;
class StuffManager;
class TaskManager;
class TutorialManager;
class UserControl;

struct lua_State;

// KnightsConfig itself
class KnightsConfig {
public:

    //
    // Interface used by client code
    //

    // Constructor.
    // NOTE: The KnightsConfig should live at least as long as the
    // KnightsGame, as it owns certain objects (tiles, graphics etc)
    // that are used during the game.
    KnightsConfig(const std::string &config_filename, bool menu_strict);

    // Get lists of all anims, graphics etc that will be used in the
    // game. This is used in network games to send lists of these
    // objects to the client computers, so that they can load all
    // necessary files before the game begins.
    void getAnims(std::vector<const Anim*> &anims) const;
    void getGraphics(std::vector<const Graphic*> &graphics) const;
    void getOverlays(std::vector<const Overlay*> &overlays) const;
    void getSounds(std::vector<const Sound*> &sounds) const;
    void getStandardControls(std::vector<const UserControl*> &standard_controls) const;
    void getOtherControls(std::vector<const UserControl*> &other_controls) const;
    const Menu & getMenu() const;
    int getApproachOffset() const;  // clients will need to know this for rendering purposes.
    void getHouseColours(std::vector<Coercri::Color> &cols) const;   // for use on the menu screen.
    
    // Menu related functions:

    // report all current menu settings to the listener
    void getCurrentMenuSettings(MenuListener &listener) const;

    // call when a menu option is changed by the user. reports all changed settings back to listener.
    void changeMenuSetting(int item_num, int new_choice_num, MenuListener &listener);

    // call when the number of players or teams changes. reports changed settings to listener.
    void changeNumberOfPlayers(int nplayers, int nteams, MenuListener &listener);

    // check whether the game can be started under strict interpretation of no. of players constraints
    bool checkNumPlayersStrict(std::string &err_msg) const;
    
    // call when all players have left game. resets settings to defaults.
    void resetMenu();

    // call when a random quest is required. reports changes to listener.
    void randomQuest(MenuListener &listener);

        
    
    //
    // Interface used by KnightsEngine
    //

    // NOTE: this creates dependencies on to quite a lot of other
    // classes, it might be nice if we could reduce this somehow.

    void initializeGame(HomeManager &home_manager,
                        std::vector<boost::shared_ptr<Player> > &players,
                        StuffManager &stuff_manager,
                        GoreManager &gore_manager,
                        MonsterManager &monster_manager,
                        EventManager &event_manager,
                        TaskManager &task_manager,
                        const std::vector<int> &hse_cols,
                        const std::vector<std::string> &player_names) const;
    boost::shared_ptr<const ConfigMap> getConfigMap() const;
    boost::shared_ptr<lua_State> getLuaState();  // This will live for as long as the KnightsConfig lives.

    // Run all the game startup functions.
    // On success, returns true.
    // On failure, returns false and sets err msg in the given string argument.
    // (Need to setup LuaStartupSentinel around this call. Ugly, but that's what we have at the moment...)
    bool runGameStartup(std::string &err_msg);

    
private:
    boost::shared_ptr<KnightsConfigImpl> pimpl;
};

#endif

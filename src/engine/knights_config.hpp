/*
 * knights_config.hpp
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
#include <vector>

class Anim;
class ConfigMap;
class CoordTransform;
class DungeonGenerator;
class DungeonMap;
class EventManager;
class GoreManager;
class Graphic;
class HomeManager;
class ItemType;
class KnightsConfigImpl;
class Menu;
class MenuListener;
class MonsterManager;
class Overlay;
class Player;
class Quest;
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

    // Menu related functions.
    //  - getCurrentMenuSettings reports all current settings to the listener
    //  - The two "change" functions report only the menu-items that have changed as a result of the operation.
    //  - In changeMenuSetting, if inputs are out of range, the function does nothing.
    void getCurrentMenuSettings(MenuListener &listener) const;
    void changeMenuSetting(int item_num, int new_choice_num, MenuListener &listener);
    void changeNumberOfPlayers(int nplayers, int nteams, MenuListener &listener);
    void resetMenu();  // called when all players have left game.
    
    
    //
    // Interface used by KnightsEngine
    //

    // NOTE: this creates dependencies on to quite a lot of other
    // classes, it might be nice if we could reduce this somehow.

    // Return value = a warning message to be displayed to players (or "" if there are no warnings)
    
    std::string initializeGame(boost::shared_ptr<DungeonMap> &dungeon_map,
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
                               TutorialManager *tutorial_manager,    // TutorialManager is optional
                               int &final_gvt) const;
    boost::shared_ptr<const ConfigMap> getConfigMap() const;
    boost::shared_ptr<lua_State> getLuaState();  // This will live for as long as the KnightsConfig lives.
    
private:
    boost::shared_ptr<KnightsConfigImpl> pimpl;
};

#endif

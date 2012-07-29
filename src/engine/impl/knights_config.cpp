/*
 * knights_config.cpp
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

#include "knights_config.hpp"
#include "knights_config_impl.hpp"
#include "menu_wrapper.hpp"

//
// constructor
//

KnightsConfig::KnightsConfig(const std::string &config_filename, bool menu_strict)
    : pimpl(new KnightsConfigImpl(config_filename, menu_strict))
{ }

void KnightsConfig::getAnims(std::vector<const Anim*> &anims) const
{
    pimpl->getAnims(anims);
}

void KnightsConfig::getGraphics(std::vector<const Graphic*> &graphics) const
{
    pimpl->getGraphics(graphics);
}

void KnightsConfig::getOverlays(std::vector<const Overlay*> &overlays) const
{
    pimpl->getOverlays(overlays);
}

void KnightsConfig::getSounds(std::vector<const Sound*> &sounds) const
{
    pimpl->getSounds(sounds);
}

void KnightsConfig::getStandardControls(std::vector<const UserControl*> &controls) const
{
    pimpl->getStandardControls(controls);
}

void KnightsConfig::getOtherControls(std::vector<const UserControl*> &controls) const
{
    pimpl->getOtherControls(controls);
}

const Menu & KnightsConfig::getMenu() const
{
    return pimpl->getMenu();
}

int KnightsConfig::getApproachOffset() const
{
    return pimpl->getApproachOffset();
}

void KnightsConfig::getHouseColours(std::vector<Coercri::Color> &result) const
{
    pimpl->getHouseColours(result);
}

void KnightsConfig::getCurrentMenuSettings(MenuListener &listener) const
{
    pimpl->getMenuWrapper().getCurrentSettings(listener);
}

void KnightsConfig::changeMenuSetting(int item_num, int new_choice_num, MenuListener &listener)
{
    pimpl->getMenuWrapper().changeSetting(item_num, new_choice_num, listener);
}

void KnightsConfig::changeNumberOfPlayers(int nplayers, int nteams, MenuListener &listener)
{
    pimpl->getMenuWrapper().changeNumberOfPlayers(nplayers, nteams, listener);
}

void KnightsConfig::resetMenu()
{
    pimpl->resetMenu();
}

void KnightsConfig::randomQuest(MenuListener &listener)
{
    pimpl->getMenuWrapper().randomQuest(listener);
}


//
// interface used by KnightsEngine
//

void KnightsConfig::initializeGame(HomeManager &home_manager,
                                   std::vector<boost::shared_ptr<Player> > &players,
                                   StuffManager &stuff_manager,
                                   GoreManager &gore_manager,
                                   MonsterManager &monster_manager,
                                   EventManager &event_manager,
                                   TaskManager &task_manager,
                                   const std::vector<int> &hse_cols,
                                   const std::vector<std::string> &player_names,
                                   TutorialManager *tutorial_manager) const
{
    pimpl->initializeGame(home_manager,
                          players,
                          stuff_manager,
                          gore_manager,
                          monster_manager,
                          event_manager,
                          task_manager,
                          hse_cols,
                          player_names,
                          tutorial_manager);
}

boost::shared_ptr<const ConfigMap> KnightsConfig::getConfigMap() const
{
    return pimpl->getConfigMap();
}

boost::shared_ptr<lua_State> KnightsConfig::getLuaState()
{
    return pimpl->getLuaState();
}

bool KnightsConfig::runGameStartup(KnightsEngine &ke, std::string &err_msg)
{
    return pimpl->getMenuWrapper().runGameStartup(ke, err_msg);
}

int KnightsConfig::getTileCategory(const std::string &s) const
{
    return pimpl->getTileCategory(s);
}

/*
 * knights_config.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
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

//
// constructor
//

KnightsConfig::KnightsConfig(const std::string &config_filename)
    : pimpl(new KnightsConfigImpl(config_filename))
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

const MenuConstraints & KnightsConfig::getMenuConstraints() const
{
    return pimpl->getMenuConstraints();
}

int KnightsConfig::getApproachOffset() const
{
    return pimpl->getApproachOffset();
}

void KnightsConfig::getHouseColours(std::vector<Coercri::Color> &result) const
{
    pimpl->getHouseColours(result);
}

std::string KnightsConfig::getQuestDescription(int quest_num, const std::string & exit_point_string) const
{
    return pimpl->getQuestDescription(quest_num, exit_point_string);
}


//
// interface used by KnightsEngine
//

void KnightsConfig::initializeGame(const MenuSelections &msel,
                                   boost::shared_ptr<DungeonMap> &dungeon_map,
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
    pimpl->initializeGame(msel,
                          dungeon_map,
                          quests,
                          home_manager,
                          players,
                          stuff_manager,
                          gore_manager,
                          monster_manager,
                          event_manager,
                          premapped,
                          starting_gears,
                          task_manager,
                          hse_cols,
                          player_names,
                          tutorial_manager,
                          final_gvt);
}

boost::shared_ptr<const ConfigMap> KnightsConfig::getConfigMap() const
{
    return pimpl->getConfigMap();
}

boost::shared_ptr<lua_State> KnightsConfig::getLuaState()
{
    return pimpl->getLuaState();
}

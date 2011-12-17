/*
 * knights_engine.hpp
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

/*
 * This is the main class that runs a single game of Knights.
 *
 * NOTE: There is a limitation that only one instance of KnightsEngine
 * can be created per thread at the moment. This is because of the way
 * my Mediator class works.
 *
 */

#ifndef KNIGHTS_ENGINE_HPP
#define KNIGHTS_ENGINE_HPP

#include "gfx/color.hpp"

#include "boost/shared_ptr.hpp"

class DungeonView;
class KnightsCallbacks;
class KnightsConfig;
class KnightsEngineImpl;
class MenuSelections;
class MiniMap;
class StatusDisplay;
class UserControl;

struct PlayerInfo {
    std::string name;
    Coercri::Color house_colour;
    int player_num;
    int kills;
    int deaths;
    int frags;
    bool eliminated;
};

class KnightsEngine {
public:
    // Start up a new KnightsEngine. Requires KnightsConfig and menu settings.
    // Note that each KnightsGame should have a unique KnightsConfig.
    KnightsEngine(boost::shared_ptr<KnightsConfig> config,
                  const MenuSelections &msel,
                  const std::vector<int> &hse_cols,
                  const std::vector<std::string> &player_names,
                  bool tutorial_mode,
                  bool deathmatch_mode,  // HACK, this should really be in the KnightsConfig.
                  std::string &warning_msg);
    ~KnightsEngine();

    // Run one update step (for a given time).
    // Uses KnightsCallbacks to inform caller of what happened during the update.
    void update(int time_delta, KnightsCallbacks &callbacks);

    // Input cmds that might be received from players.
    void setControl(int player, const UserControl *control);
    void setApproachBasedControls(int player, bool flag);
    void setActionBarControls(int player, bool flag);
    void setSpeechBubble(int player, bool show);
    
    // Send initial updates to an observer who has just joined the game.
    void catchUp(int player, DungeonView &dungeon_view, MiniMap &mini_map, StatusDisplay &status_display);

    // Find out how many players are still active in the game
    int getNumPlayersRemaining() const;

    // Eliminate a player -- used when somebody disconnects
    void eliminatePlayer(int player);

    // Get the player list
    // Only players still in the game are included.
    void getPlayerList(std::vector<PlayerInfo> &player_list) const;

    // Get total of skulls and kills across all players (including eliminated players).
    // Used as a quick way of determining whether someone has died since last update:
    int getSkullsPlusKills() const;

    // Find out how much time is left (in milliseconds), or -1 if there is no time limit.
    int getTimeRemaining() const;
    
private:
    boost::shared_ptr<KnightsEngineImpl> pimpl;
};

#endif

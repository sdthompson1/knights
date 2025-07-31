/*
 * knights_engine.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2025.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

#include "player_state.hpp"

#include "utf8string.hpp"

#include "gfx/color.hpp"

#include "boost/shared_ptr.hpp"

#include <vector>

class DungeonView;
class ItemType;
class KnightsCallbacks;
class KnightsConfig;
class KnightsEngineImpl;
class MiniMap;
class StatusDisplay;
class UserControl;

struct PlayerInfo {
    UTF8String name;
    Coercri::Color house_colour;
    int house_colour_index;
    int player_num;
    int kills;
    int deaths;
    int frags;   // 'score' for deathmatch games
    PlayerState player_state;
};

class KnightsEngine {
public:
    // Start up a new KnightsEngine. Requires KnightsConfig and menu settings.
    // Note that each KnightsGame should have a unique KnightsConfig.
    KnightsEngine(boost::shared_ptr<KnightsConfig> config,
                  const std::vector<int> &hse_cols,
                  const std::vector<UTF8String> &player_names,
                  bool &deathmatch_mode,   // output.
                  std::vector<std::string> &msgs);  // output.
    ~KnightsEngine();

    // Run one update step (for a given time).
    // Uses KnightsCallbacks to inform caller of what happened during the update.
    void update(int time_delta, KnightsCallbacks &callbacks);

    // Find out how long until the next update is required.
    int getTimeToNextUpdate() const;
    
    // Input cmds that might be received from players.
    void setControl(int player, const UserControl *control);
    void setApproachBasedControls(int player, bool flag);
    void setActionBarControls(int player, bool flag);
    void setSpeechBubble(int player, bool show);
    
    // Send initial updates to an observer who has just joined the game.
    void catchUp(int player, KnightsCallbacks &cb);

    // Find out how many players are still active in the game
    int getNumPlayersRemaining() const;

    // Change player state
    void changePlayerState(int player, PlayerState new_state);

    // Get the player list
    // NOTE: the result is sorted by house colour, then name. (#172)
    void getPlayerList(std::vector<PlayerInfo> &player_list) const;

    // Get total of skulls and kills across all players (including eliminated players).
    // Used as a quick way of determining whether someone has died since last update:
    int getSkullsPlusKills() const;

    // Find out how much time is left (in milliseconds), or -1 if there is no time limit.
    int getTimeRemaining() const;


    //
    // Interface used by lua game setup functions.
    //

    void resetMap();
    void setPremapped(bool);
    void gameStartupMsg(const std::string &msg);
    void addStartingGear(ItemType *, const std::vector<int> &);
    void setItemRespawn(const std::vector<ItemType*> &items_to_respawn, int respawn_delay);
    void setLockpickSpawn(ItemType *lockpicks, int init_time, int interval);
    void setTimeLimit(int ms);
        
private:
    boost::shared_ptr<KnightsEngineImpl> pimpl;
};

#endif

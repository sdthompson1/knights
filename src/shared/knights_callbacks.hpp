/*
 * knights_callbacks.hpp
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
 * Interface used by KnightsEngine to inform client code of events
 * that are taking place in the dungeon. This interface is also
 * re-used in KnightsClient.
 *
 */

#ifndef KNIGHTS_CALLBACKS_HPP
#define KNIGHTS_CALLBACKS_HPP

#include "player_id.hpp"
#include "tutorial_window.hpp"
#include "utf8string.hpp"

#include <string>
#include <vector>

class DungeonView;
class LocalKey;
class LocalParam;
class MiniMap;
class Sound;
class StatusDisplay;
class UserControl;

class KnightsCallbacks {
public:
    virtual ~KnightsCallbacks() { }

    //
    // Most updates are sent through the DungeonView, MiniMap or
    // StatusDisplay interfaces.
    // 
    // The client code must create these objects separately, and
    // implement the following methods to allow KnightsEngine to
    // access them.
    //
    virtual DungeonView & getDungeonView(int player_num) = 0;
    virtual MiniMap & getMiniMap(int player_num) = 0;
    virtual StatusDisplay & getStatusDisplay(int player_num) = 0;

    //
    // The remaining updates are sent directly through this interface;
    // see methods below.
    //
    
    // Sound
    virtual void playSound(int player_num, const Sound &sound, int frequency) = 0;

    // Tell us when someone has won/lost
    virtual void winGame(int player_num) = 0;
    virtual void loseGame(int player_num) = 0;

    // Controls
    // NOTE: Clients should return ONLY UserControls that they got from setAvailableControls, they should not make their
    // own UserControl objects. This is because we static_cast the UserControls down to Controls and this would crash if
    // the UserControl didn't have the right type!
    virtual void setAvailableControls(int player_num, const std::vector<std::pair<const UserControl*, bool> > &available_controls) = 0;
    virtual void setMenuHighlight(int player_num, const UserControl *highlight) = 0;

    // Flash Screen
    virtual void flashScreen(int player_num, int delay) = 0;

    // GameMsg -- a message to be sent to a particular player.
    // NOTE: If player_num < 0 then the message will be broadcast to all players.
    // (If is_err is set, the total no of msgs will be limited to 100 per game. Used 
    // for Lua errors.)
    virtual void gameMsgLoc(int player_num, const LocalKey &key, const std::vector<LocalParam> &params, bool is_err = false) = 0;

    // Tutorial pop-up windows
    virtual void popUpWindow(const std::vector<TutorialWindow> &windows) = 0;

    // These functions are slightly hacky but used for Trac #10 -- Go into observer mode once you are eliminated.
    // And also #165 -- Disabling the observer window for players who are eliminated.
    virtual void onElimination(int player_num) = 0;
    virtual void disableView(int player_num) = 0;
    virtual void goIntoObserverMode(int nplayers, const std::vector<PlayerID> &ids) = 0;


    // This is a slight hack needed to get catchUp to work properly
    virtual void prepareForCatchUp(int player_num) { }
};

#endif

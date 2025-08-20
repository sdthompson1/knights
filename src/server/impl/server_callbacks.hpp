/*
 * server_callbacks.hpp
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
 * Implementation of KnightsCallbacks that writes binary data to a
 * buffer.
 *
 */

#ifndef SERVER_CALLBACKS_HPP
#define SERVER_CALLBACKS_HPP

#include "knights_callbacks.hpp"

#include "boost/shared_ptr.hpp"

class ServerDungeonView;
class ServerMiniMap;
class ServerStatusDisplay;

class ServerCallbacks : public KnightsCallbacks {
public:
    typedef unsigned char ubyte;
    
    explicit ServerCallbacks(int nplayers);
    virtual ~ServerCallbacks();

    // methods to append queued cmds to the given vector.
    void appendPlayerCmds(int plyr, std::vector<ubyte> &cmds) const;
    void appendObserverCmds(int observer_num, std::vector<ubyte> &cmds) const;

    // observer num management
    int allocObserverNum();
    void rmObserverNum(int observer_num);
    std::vector<int> getPlayersToPutIntoObsMode();  // clears the list afterwards

    // query game over state
    bool isGameOver() const { return game_over; }
    int getWinnerNum() const { return winner_num; } // plyr num of the winner, valid if isGameOver() true. (-1 if no winner.)
    
    // clear queued cmds.
    void clearCmds();

    
    // functions overridden from KnightsCallbacks
    virtual DungeonView & getDungeonView(int plyr) override;
    virtual MiniMap & getMiniMap(int plyr) override;
    virtual StatusDisplay & getStatusDisplay(int plyr) override;

    virtual void playSound(int plyr, const Sound &sound, int frequency) override;

    virtual void winGame(int plyr) override;
    virtual void loseGame(int plyr) override;

    virtual void setAvailableControls(int plyr, const std::vector<std::pair<const UserControl*, bool> > &available_controls) override;
    virtual void setMenuHighlight(int plyr, const UserControl *highlight) override;

    virtual void flashScreen(int plyr, int delay) override;

    virtual void gameMsgRaw(int plyr_num, const Coercri::UTF8String &msg, bool is_err) override;
    virtual void gameMsgLoc(int plyr_num, const LocalKey &key, const std::vector<LocalParam> &params, bool is_err) override;
    virtual void popUpWindow(const std::vector<TutorialWindow> &windows) override;

    virtual void onElimination(int player_num) override;
    virtual void disableView(int player_num) override;
    virtual void goIntoObserverMode(int nplayers, const std::vector<PlayerID> &ids) override { }

    virtual void prepareForCatchUp(int player_num) override;

private:
    void doAppendPlayerCmds(int plyr, std::vector<ubyte> &out, int observer_num, bool include_private) const;
    void gameMsgImpl(int plyr, const Coercri::UTF8String &msg, const LocalKey &key, const std::vector<LocalParam> &params, bool is_err);

private:
    std::vector<boost::shared_ptr<ServerDungeonView> > dungeon_view;
    std::vector<boost::shared_ptr<ServerMiniMap> > mini_map;
    std::vector<boost::shared_ptr<ServerStatusDisplay> > status_display;

    // output buffers
    std::vector<std::vector<ubyte> > pub, prv;

    // caching
    std::vector<const UserControl*> prev_menu_highlight;

    std::vector<bool> loser;
    
    bool game_over;
    int winner_num;
    int next_observer_num;

    std::vector<int> players_to_put_into_obs_mode;

    int no_err_msgs;
};

#endif

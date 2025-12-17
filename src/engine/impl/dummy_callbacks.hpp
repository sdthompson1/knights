/*
 * dummy_callbacks.hpp
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

#ifndef DUMMY_CALLBACKS_HPP
#define DUMMY_CALLBACKS_HPP

#include "dungeon_view.hpp"
#include "mini_map.hpp"
#include "player_id.hpp"
#include "status_display.hpp"
#include "knights_callbacks.hpp"

class DummyDungeonView : public DungeonView {
public:
    virtual void setCurrentRoom(int,int,int) override { }
    virtual void addEntity(unsigned short int, int, int, MapHeight, MapDirection,
        const Anim *, const Overlay *, int, int, bool, bool, bool, int, MotionType, int, const PlayerID &) override { }
    virtual void rmEntity(unsigned short int) override { }
    virtual void repositionEntity(unsigned short int, int, int) override { }
    virtual void moveEntity(unsigned short int, MotionType, int, bool) override { }
    virtual void flipEntityMotion(unsigned short int, int, int) override { }
    virtual void setAnimData(unsigned short int, const Anim *, const Overlay *, int, int, bool, bool, bool) override { }
    virtual void setFacing(unsigned short int, MapDirection) override { }
    virtual void setSpeechBubble(unsigned short int, bool) override { }
    virtual void clearTiles(int,int,bool) override { }
    virtual void setTile(int,int,int,const Graphic *, boost::shared_ptr<const ColourChange>,bool) override { }
    virtual void setItem(int, int, const Graphic *,bool) override { }
    virtual void placeIcon(int, int, const Graphic *, int) override { }
    virtual void flashMessage(const std::string&, int) override { }
    virtual void cancelContinuousMessages() override { }
    virtual void addContinuousMessage(const std::string&) override { }
};

class DummyMiniMap : public MiniMap {
public:
    virtual void setSize(int,int) override { }
    virtual void setColour(int, int, MiniMapColour) override { }
    virtual void wipeMap() override { }
    virtual void mapKnightLocation(int,int,int) override { }
    virtual void mapItemLocation(int,int,bool) override { }
};

class DummyStatusDisplay : public StatusDisplay {
public:
    virtual void setBackpack(int, const Graphic *, const Graphic *, int, int) override { }
    virtual void addSkull() override { }
    virtual void setHealth(int) override { }
    virtual void setPotionMagic(PotionMagic, bool) override { }
    virtual void setQuestHints(const std::vector<std::string> &) override { }
};

class DummyCallbacks : public KnightsCallbacks {
public:
    virtual DungeonView & getDungeonView(int) override { return dummy_dungeon_view; }
    virtual MiniMap & getMiniMap(int) override { return dummy_mini_map; }
    virtual StatusDisplay & getStatusDisplay(int) override { return dummy_status_display; }
    virtual void playSound(int, const Sound&, int) override { }
    virtual void winGame(int) override { }
    virtual void loseGame(int) override { }
    virtual void setAvailableControls(int, const std::vector<std::pair<const UserControl*,bool> >&) override { }
    virtual void setMenuHighlight(int, const UserControl*) override { }
    virtual void flashScreen(int,int) override { }
    virtual void gameMsgLoc(int, const LocalKey&, const std::vector<LocalParam>&, bool) override { }
    virtual void popUpWindow(const std::vector<TutorialWindow> &) override { }
    virtual void onElimination(int) override { }
    virtual void disableView(int) override { }
    virtual void goIntoObserverMode(int, const std::vector<PlayerID>&) override { }
private:
    DummyDungeonView dummy_dungeon_view;
    DummyMiniMap dummy_mini_map;
    DummyStatusDisplay dummy_status_display;
};

#endif

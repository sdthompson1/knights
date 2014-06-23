/*
 * dummy_callbacks.hpp
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

#ifndef DUMMY_CALLBACKS_HPP
#define DUMMY_CALLBACKS_HPP

#include "dungeon_view.hpp"
#include "mini_map.hpp"
#include "status_display.hpp"
#include "knights_callbacks.hpp"

class DummyDungeonView : public DungeonView {
public:
    virtual void setCurrentRoom(int,int,int) { }
    virtual void addEntity(unsigned short int, int, int, MapHeight, MapDirection,
        const Anim *, const Overlay *, int, int, bool, bool, bool, int, MotionType, int, const UTF8String &) { }
    virtual void rmEntity(unsigned short int) { }
    virtual void repositionEntity(unsigned short int, int, int) { }
    virtual void moveEntity(unsigned short int, MotionType, int, bool) { }
    virtual void flipEntityMotion(unsigned short int, int, int) { }
    virtual void setAnimData(unsigned short int, const Anim *, const Overlay *, int, int, bool, bool, bool) { }
    virtual void setFacing(unsigned short int, MapDirection) { }
    virtual void setSpeechBubble(unsigned short int, bool) { }
    virtual void clearTiles(int,int,bool) { }
    virtual void setTile(int,int,int,const Graphic *, boost::shared_ptr<const ColourChange>,bool) { }
    virtual void setItem(int, int, const Graphic *,bool) { }
    virtual void placeIcon(int, int, const Graphic *, int) { }
    virtual void flashMessage(const std::string&, int) { }
    virtual void cancelContinuousMessages() { }
    virtual void addContinuousMessage(const std::string&) { }
};

class DummyMiniMap : public MiniMap {
public:
    virtual void setSize(int,int) { }
    virtual void setColour(int, int, MiniMapColour) { }
    virtual void wipeMap() { }
    virtual void mapKnightLocation(int,int,int) { }
    virtual void mapItemLocation(int,int,bool) { }
};

class DummyStatusDisplay : public StatusDisplay {
public:
    virtual void setBackpack(int, const Graphic *, const Graphic *, int, int) { }
    virtual void addSkull() { }
    virtual void setHealth(int) { }
    virtual void setPotionMagic(PotionMagic, bool) { }
    virtual void setQuestHints(const std::vector<std::string> &) { }
};

class DummyCallbacks : public KnightsCallbacks {
public:
    virtual DungeonView & getDungeonView(int) { return dummy_dungeon_view; }
    virtual MiniMap & getMiniMap(int) { return dummy_mini_map; }
    virtual StatusDisplay & getStatusDisplay(int) { return dummy_status_display; }
    virtual void playSound(int, const Sound&, int) { }
    virtual void winGame(int) { }
    virtual void loseGame(int) { }
    virtual void setAvailableControls(int, const std::vector<std::pair<const UserControl*,bool> >&) { }
    virtual void setMenuHighlight(int, const UserControl*) { }
    virtual void flashScreen(int,int) { }
    virtual void gameMsg(int,const std::string&,bool) { }
    virtual void popUpWindow(const std::vector<TutorialWindow> &) { }
    virtual void onElimination(int) { }
    virtual void disableView(int) { }
    virtual void goIntoObserverMode(int, const std::vector<UTF8String>&) { }
private:
    DummyDungeonView dummy_dungeon_view;
    DummyMiniMap dummy_mini_map;
    DummyStatusDisplay dummy_status_display;
};

#endif

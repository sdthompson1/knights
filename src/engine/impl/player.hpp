/*
 * player.hpp
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

#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "map_support.hpp"
#include "mini_map_colour.hpp"
#include "status_display.hpp"

#include "boost/weak_ptr.hpp"
using namespace boost;

#include <deque>
#include <map>
#include <set>
#include <vector>
using namespace std;

class Anim;
class ColourChange;
class Control;
class DungeonMap;
class DungeonView;
class HealingTask;
class Item;
class ItemType;
class Knight;
class MiniMap;
class Quest;
class RespawnTask;
class RoomMap;
class StatusDisplay;
class TaskManager;
class Tile;

//
// Player
// Effectively this just manages respawning of a particular Knight.
// It also keeps track of where that knight's home is, and other such information.
//

class Player {
public:
    // Ctor. (Note that this does not automatically spawn the knight; you have to call respawn
    // explicitly to get the knight to appear for the first time.)
    // Home_facing means the direction YOU have to face to be approaching your home.
    // Control_set (NOT copied) is the set of controls available (for menus or "tapping"
    // fire). (Items and Tiles will be checked for additional controls, and the global
    // controls from control.hpp will also always be available.)
    Player(int plyr_num,
           DungeonMap &home_map, const MapCoord &home_pos, MapDirection home_facing,
           const Anim * anim, const ItemType * dflt_item, 
           const map<const ItemType *, int> * backpack_capacities_,
           const vector<const Control*> &control_set_,
           const vector<shared_ptr<Quest> > &quests_,
           shared_ptr<const ColourChange> secured_home_cc,
           const std::string &name_, int team_num_);

    // Respawn type
    enum RespawnType { R_NORMAL, R_RANDOM_SQUARE, R_DIFFERENT_EVERY_TIME };
    void setRespawnType(RespawnType r) { respawn_type = r; }
    RespawnType getRespawnType() const { return respawn_type; }
    
    // Add starting gear.
    void addStartingGear(const ItemType &itype, const vector<int> &nos);

    // Setup exit
    void setExit(const MapCoord &exit_loc, MapDirection facing)
    { exit_location = exit_loc; exit_facing = facing; exit_from_players_home = 0; }

    void setExitFromPlayersHome(const Player &p)
    { exit_from_players_home = &p; }
    
    // accessor functions   
    const MapCoord & getHomeLocation() const { return home_location; }  // square just outside the home,
                                                                        // or NULL if all homes secured.
    MapDirection getHomeFacing() const { return home_facing; }  // direction pointing towards the home.
    const MapCoord & getExitLocation() const;
    MapDirection getExitFacing() const;
    
    shared_ptr<Knight> getKnight() const { return knight.lock(); }
    MapCoord getKnightPos() const; // convenience. (Returns Null mapcoord if knight doesn't exist.)
    DungeonMap * getDungeonMap() const; // convenience
    RoomMap * getRoomMap() const; // convenience
    bool checkQuests(vector<string> &hints_out) const;
    bool isItemInteresting(const ItemType &itm) const; // check item against quest rqmts.
    std::string getQuestMessage() const;
    void getQuestIcons(std::vector<StatusDisplay::QuestIconInfo> &icons_out) const;
    shared_ptr<const ColourChange> getSecuredCC() const { return secured_home_cc; }

    // get player num
    int getPlayerNum() const { return player_num; }

    // Access to DungeonView, MiniMap, StatusDisplay.
    // NOTE: If changing the current room, or mapping/unmapping rooms, you MUST use the
    // special functions below. Otherwise can access dungeonview/minimap/statusdisplay directly.
    // This is so that local state (current room, list of mapped rooms) can be kept up to date.
    void setCurrentRoom(int room, int width, int height);
    int getCurrentRoom() const;
    void mapCurrentRoom(const MapCoord &top_left);
    bool isRoomMapped(int room) const;
    bool isSquareMapped(const MapCoord &mc) const;  // tests if the square != COL_UNMAPPED.
    DungeonView & getDungeonView() const;
    StatusDisplay & getStatusDisplay() const;

    // 17-Aug-2009: decided to prevent direct access to MiniMap,
    // instead have to go through the following functions. This is so
    // that we can keep a local cache of what squares are mapped, for
    // sending to observers if they join a game half-way through.
    // See comments in mini_map.hpp for descriptions of these functions
    void setMiniMapSize(int w, int h);
    void setMiniMapColour(int x, int y, MiniMapColour col);
    void wipeMap();
    void mapKnightLocation(int n, int x, int y);
    void mapItemLocation(int x, int y, bool on);

    void sendMiniMap(MiniMap &m);
    void sendStatusDisplay(StatusDisplay &s);

    // Trac #68: Should show position on mini-map except in unmapped rooms after teleport.
    // This flag keeps track of whether we have teleported recently.
    bool getTeleportFlag() const { return teleport_flag; }
    void setTeleportFlag(bool t) { teleport_flag = t; }
    
    // access to controls. this is set by KnightsEngine when we
    // receive a control from the client, and read by KnightTask when
    // we need to process the controls.
    // NOTE: readControl will only return a non-cts control ONCE (for
    // each press). It will return cts controls multiple times (until
    // the user releases it).
    const Control * readControl();
    const Control * peekControl() const;
    void setControl(const Control *ctrl);

    // what type of controls is the player using -- some places in the code need to know this
    // (e.g. action bar players have a delay on dagger throwing;
    // non approach based players can pick up from a table without approaching it.)
    bool getApproachBasedControls() const { return approach_based_controls; }
    void setApproachBasedControls(bool flag) { approach_based_controls = flag; }
    bool getActionBarControls() const { return action_bar_controls; }
    void setActionBarControls(bool flag) { action_bar_controls = flag; }
    
    // speech bubble.
    void setSpeechBubble(bool show);
    bool getSpeechBubble() const { return speech_bubble; }
    
    // reset home (called by HomeManager, also called by eliminatePlayer to stop the player respawning)
    void resetHome(const MapCoord &mc, const MapDirection &facing);
    
    // event handling functions
    void onDeath(); // Called by Knight dtor. Organizes respawning, and adds a skull.

    // Try to respawn. Returns true if successful.
    bool respawn();

    // Control functions (see .cpp file for more info)
    void getControlInfo(const Control *ctrl_in, const ItemType *& itype_out,
                        weak_ptr<Tile> &tile_out, MapCoord &tile_mc_out) const;
    void computeAvailableControls();

    // get player name
    const std::string & getName() const { return name; }

    // "eliminated" flag.
    void setElimFlag() { elim_flag = true; }
    bool getElimFlag() const { return elim_flag; }

    int getNSkulls() const { return nskulls; }
    int getKills() const { return nkills; }
    void addKill() { ++nkills; }

    // team_num is -1 for a normal game or >= 0 for team games
    int getTeamNum() const { return team_num; }
    
private:
    struct ControlInfo {
        weak_ptr<Tile> tile;
        MapCoord tile_mc;
        const ItemType *item_type; // The item is always assumed to be in the creature's inventory...
        bool primary;
        
        ControlInfo() : item_type(0), primary(true) { }
        bool operator==(const ControlInfo &rhs) const {
            return !(tile < rhs.tile) && !(rhs.tile < tile) 
                && tile_mc == rhs.tile_mc
                && item_type == rhs.item_type
                && primary == rhs.primary;
        }
    };

private:
    void addTileControls(DungeonMap *dmap, const MapCoord &mc,
                         map<const Control *, ControlInfo> &cmap, bool ahead,
                         bool approaching, bool approach_based, MapHeight ht, shared_ptr<Creature>);
    void addItemControls(const ItemType &itype,
                         map<const Control *, ControlInfo> &cmap,
                         shared_ptr<Creature>);
    static void giveStartingGear(shared_ptr<Knight> knight, const vector<pair<const ItemType*, int> > &items);

private:
    friend class RespawnTask;
    friend class HealingTask;

    int player_num;
    const Control *control;

    int current_room;
    int current_room_width, current_room_height;
    int mini_map_width, mini_map_height;
    std::set<int> mapped_rooms;
    std::vector<MiniMapColour> mapped_squares;  // local copy of their mini map...
    std::map<int, std::pair<int, int> > knight_locations;
    std::set<std::pair<int, int> > item_locations;
    
    DungeonMap &home_dmap;
    const Player * exit_from_players_home;   // If set, this overrides exit_location/exit_facing.
    MapCoord home_location, exit_location;
    MapDirection home_facing, exit_facing;
    const Anim * anim;
    const ItemType * default_item;
    const map<const ItemType *, int> * backpack_capacities;
    const vector<const Control *> & control_set;

    map<const Control *, ControlInfo> current_controls;
    
    weak_ptr<Knight> knight;
    shared_ptr<RespawnTask> respawn_task;
    shared_ptr<HealingTask> healing_task;

    vector<shared_ptr<Quest> > quests;

    shared_ptr<const ColourChange> secured_home_cc;

    deque<vector<pair<const ItemType *, int> > > gears;

    int nskulls;
    int nkills;
    
    std::string name;
    bool elim_flag;

    RespawnType respawn_type;
    int team_num;

    bool teleport_flag;
    bool speech_bubble;
    bool approach_based_controls;
    bool action_bar_controls;
};

#endif

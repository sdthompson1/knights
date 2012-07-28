/*
 * player.cpp
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
 * Player class, and Respawning.
 *
 */

#include "misc.hpp"

#include "action_data.hpp"
#include "control.hpp"
#include "dungeon_map.hpp"
#include "dungeon_view.hpp"
#include "item_type.hpp"
#include "knight.hpp"
#include "knight_task.hpp"
#include "knights_callbacks.hpp"
#include "mediator.hpp"
#include "mini_map.hpp"
#include "player.hpp"
#include "quest.hpp"
#include "rng.hpp"
#include "status_display.hpp"
#include "task.hpp"
#include "task_manager.hpp"
#include "tile.hpp"

#include <set>
#include <sstream>

namespace {

    // operator< based on distance from a base point
    struct DistCmp {
        explicit DistCmp(const MapCoord &b) : base(b) { }
        bool operator()(const MapCoord &lhs, const MapCoord &rhs) const {
            const int d_lhs = std::abs(lhs.getX() - base.getX()) + std::abs(lhs.getY() - base.getY());
            const int d_rhs = std::abs(rhs.getX() - base.getX()) + std::abs(rhs.getY() - base.getY());
            if (d_lhs < d_rhs) return true;
            if (d_lhs > d_rhs) return false;
            return lhs < rhs;
        }
        MapCoord base;
    };

    void TryInsert(std::set<MapCoord, DistCmp> &open, const DungeonMap &dmap, 
                   const std::set<MapCoord> &closed, const MapCoord &mc)
    {
        // To go through a square, we need it to be A_APPROACH or A_CLEAR,
        // and also not visited before (i.e. not in closed list)
        if (closed.find(mc) == closed.end()
        && dmap.getAccessTilesOnly(mc, H_WALKING) >= A_APPROACH) {
            open.insert(mc);
        }
    }

    MapCoord FindRespawnPoint(DungeonMap &dmap, MapCoord home_point)
    {
        if (home_point.isNull()) {
            // knight doesn't have a home, so pick a random point to respawn
            home_point = MapCoord(g_rng.getInt(1, dmap.getWidth()-1),
                                  g_rng.getInt(1, dmap.getHeight()-1));
        }
        
        // Trac #24. Try to find an alternative nearby respawn point
        // if the home point is occupied.

        DistCmp cmp(home_point);
        
        std::set<MapCoord, DistCmp> open(cmp);
        open.insert(home_point);

        std::set<MapCoord> closed;

        while (!open.empty()) {
            // Get next square to visit
            MapCoord mc = *open.begin();
            open.erase(open.begin());

            // Mark this square as visited
            closed.insert(mc);

            // Add the surrounding squares to open list
            TryInsert(open, dmap, closed, DisplaceCoord(mc, D_NORTH));
            TryInsert(open, dmap, closed, DisplaceCoord(mc, D_SOUTH));
            TryInsert(open, dmap, closed, DisplaceCoord(mc, D_EAST));
            TryInsert(open, dmap, closed, DisplaceCoord(mc, D_WEST));
            
            // Now try to respawn on this point
            if (dmap.getAccess(mc, H_WALKING) == A_CLEAR) {
                return mc;
            }
        }

        return MapCoord(); // failed
    }
}


//
// RespawnTask
//

class RespawnTask : public Task {
public:
    RespawnTask(Player &p) : player(p) { }
    virtual void execute(TaskManager &);
private:
    Player &player;
};

void RespawnTask::execute(TaskManager &tm)
{
    // The player->respawn_task thing is to make sure that any manual calls to respawn() do
    // not conflict with the auto-respawn system.
    if (this != player.respawn_task.get()) return;
    bool success = player.respawn();
    if (!success) {
        tm.addTask(shared_from_this(), TP_NORMAL, 
            tm.getGVT() + Mediator::instance().cfgInt("respawn_delay")); // try again later
    }
}



//
// Player
//

Player::Player(int plyr_num,
               DungeonMap &home_map, 
               const Anim *a, const ItemType * di, 
               const vector<const Control*> &cs, 
               shared_ptr<const ColourChange> sec_home_cc,
               const std::string &name_,
               int team_num_)
    : player_num(plyr_num), control(0),
      current_room(-1), current_room_width(0), current_room_height(0),
      mini_map_width(0), mini_map_height(0),
      home_dmap(home_map), exit_from_players_home(0), 
      anim(a),
      default_item(di), backpack_capacities(0), control_set(cs),
      secured_home_cc(sec_home_cc),
      nskulls(0), nkills(0), frags(0), name(name_), elim_flag(false), respawn_type(R_NORMAL),
      team_num(team_num_),
      teleport_flag(false), speech_bubble(false), approach_based_controls(true), action_bar_controls(false)
{
}


void Player::addStartingGear(const ItemType &itype, const vector<int> &numbers)
{
    if (gears.size() < numbers.size()) gears.resize(numbers.size());

    for (int i=0; i<numbers.size(); ++i) {
        gears[i].push_back(make_pair(&itype, numbers[i]));
    }
}


//
// exit location
//

const MapCoord& Player::getExitLocation() const
{
    if (exit_from_players_home) {
        return exit_from_players_home->getHomeLocation();
    } else {
        return exit_location;
    }
}

MapDirection Player::getExitFacing() const
{
    if (exit_from_players_home) {
        return exit_from_players_home->getHomeFacing();
    } else {
        return exit_facing;
    }
}


//
// convenience functions
//

MapCoord Player::getKnightPos() const
{
    shared_ptr<Knight> kt(getKnight());
    if (kt) return kt->getPos();
    else return MapCoord();
}

DungeonMap * Player::getDungeonMap() const
{
    shared_ptr<Knight> kt = getKnight();
    if (kt) return kt->getMap();
    else return 0;
}

RoomMap * Player::getRoomMap() const
{
    DungeonMap * dmap = getDungeonMap();
    if (dmap) return dmap->getRoomMap();
    else return 0;
}

bool Player::checkQuests(vector<string> &hints_out) const
{
    hints_out.clear();
    bool result = true;
    
    shared_ptr<Knight> kt = getKnight();
    if (!kt) return false;
    
    for (vector<shared_ptr<Quest> >::const_iterator it = quests.begin();
    it != quests.end(); ++it) {
        if (!(*it)->check(*kt)) {
            result = false;
            hints_out.push_back((*it)->getHint());
        }
    }
    return result;
}

bool Player::isItemInteresting(const ItemType &itype) const
{
    for (vector<shared_ptr<Quest> >::const_iterator it = quests.begin();
    it != quests.end(); ++it) {
        if ((*it)->isItemInteresting(itype)) {
            return true;
        }
    }
    return false;
}

void Player::getQuestIcons(vector<StatusDisplay::QuestIcon> &quests_out, QuestCircumstance c) const
{
    quests_out.clear();
    shared_ptr<Knight> kt = getKnight();
    
    for (vector<shared_ptr<Quest> >::const_iterator it = quests.begin();
    it != quests.end(); ++it) {
        const StatusDisplay::QuestIcon icon = (*it)->getQuestIcon(*kt, c);
        quests_out.push_back(icon);
    }
}

//
// mapping functions
//

void Player::setCurrentRoom(int room, int width, int height)
{
    getDungeonView().setCurrentRoom(room, width, height);
    current_room = room;
    current_room_width = width;
    current_room_height = height;
}

int Player::getCurrentRoom() const
{
    return current_room;
}

void Player::mapCurrentRoom(const MapCoord &top_left)
{
    if (current_room == -1) return;
    if (isRoomMapped(current_room)) return;

    const int ox = top_left.getX();
    const int oy = top_left.getY();

    const DungeonMap *dmap = getDungeonMap();
    if (!dmap) return;

    std::vector<boost::shared_ptr<Tile> > tiles;
    
    for (int j = 0; j < current_room_height; ++j) {
        for (int i = 0; i < current_room_width; ++i) {

            MiniMapColour col = COL_UNMAPPED;
            dmap->getTiles(MapCoord(i+ox, j+oy), tiles);
            for (std::vector<boost::shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
                const MiniMapColour c = (*it)->getColour();
                if (c < col) col = c;
            }
            
            setMiniMapColour(i + ox, j + oy, col);
        }
    }
    
    mapped_rooms.insert(current_room);
}

bool Player::isRoomMapped(int room) const
{
    return room != -1 && mapped_rooms.find(room) != mapped_rooms.end();
}

bool Player::isSquareMapped(const MapCoord &mc) const
{
    if (mc.getX() < 0 || mc.getY() < 0 || mc.getX() >= mini_map_width || mc.getY() >= mini_map_height) return false;
    return mapped_squares[mc.getY() * mini_map_width + mc.getX()] != COL_UNMAPPED;
}

DungeonView & Player::getDungeonView() const
{
    return Mediator::instance().getCallbacks().getDungeonView(player_num);
}

StatusDisplay & Player::getStatusDisplay() const
{
    return Mediator::instance().getCallbacks().getStatusDisplay(player_num);
}

//
// mini map stuff
//

void Player::setMiniMapSize(int w, int h)
{
    Mediator::instance().getCallbacks().getMiniMap(player_num).setSize(w, h);
    mini_map_width = w;
    mini_map_height = h;
    mapped_squares.resize(w*h);
    for (int i = 0; i < w*h; ++i) mapped_squares[i] = COL_UNMAPPED;
    knight_locations.clear();
    item_locations.clear();
}

void Player::setMiniMapColour(int x, int y, MiniMapColour col)
{
    if (x < 0 || y < 0 || x >= mini_map_width || y >= mini_map_height) return;   // square doesn't exist. ignore
    if (mapped_squares[y * mini_map_width + x] == col) return;  // don't need to do anything - client already has this square
    Mediator::instance().getCallbacks().getMiniMap(player_num).setColour(x, y, col);
    mapped_squares[y*mini_map_width + x] = col;
}

void Player::wipeMap()
{
    Mediator::instance().getCallbacks().getMiniMap(player_num).wipeMap();
    for (int i = 0; i < mini_map_width * mini_map_height; ++i) mapped_squares[i] = COL_UNMAPPED;
    mapped_rooms.clear();
}

void Player::mapKnightLocation(int n, int x, int y)
{
    Mediator::instance().getCallbacks().getMiniMap(player_num).mapKnightLocation(n, x, y);
    if (x < 0 || y < 0 || x >= mini_map_width || y >= mini_map_height) {
        knight_locations.erase(n);
    } else {
        knight_locations[n] = std::make_pair(x,y);
    }
}

void Player::mapItemLocation(int x, int y, bool on)
{
    Mediator::instance().getCallbacks().getMiniMap(player_num).mapItemLocation(x, y, on);
    if (x < 0 || y < 0 || x >= mini_map_width || y >= mini_map_height) return;
    if (on) item_locations.erase(std::make_pair(x,y));
    else item_locations.insert(std::make_pair(x,y));
}

void Player::setControl(const Control *ctrl)
{
    // Discontinuous controls are never replaced by a new control (the
    // second control just gets lost!). Other controls are replaced
    // immediately.
    if (control == 0 || control->isContinuous()) {
        control = ctrl;
    }
}

const Control * Player::readControl()
{
    const Control * result = control;
    
    // If the control is discts it will only be returned once
    if (control && !control->isContinuous()) {
        control = 0;
    }

    return result;
}

const Control * Player::peekControl() const
{
    // "Peek" at the current control, without modifying it.
    return control;
}

void Player::setSpeechBubble(bool show)
{
    if (show != speech_bubble) {
        speech_bubble = show;
        shared_ptr<Knight> kt = getKnight();
        if (kt) Mediator::instance().onChangeSpeechBubble(getKnight());
    }
}


//
// event handling functions
//

void Player::onDeath()
{
    // This is called by the knight's dtor. It schedules the
    // respawning of a new knight, after a short delay.

    // NOTE: This MUST NOT call Lua because we are not allowed to raise Lua errors from
    // this function...

    Mediator &mediator = Mediator::instance();
    shared_ptr<RespawnTask> rt(new RespawnTask(*this));
    respawn_task = rt;
    TaskManager &tm(mediator.getTaskManager());
    tm.addTask(respawn_task, TP_NORMAL, tm.getGVT() + mediator.cfgInt("respawn_delay"));

    // Also add a skull
    getStatusDisplay().addSkull();
    nskulls++;

    // Clear speech bubble on death
    setSpeechBubble(false);
}



//
// respawn
//

void Player::giveStartingGear(shared_ptr<Knight> knight, const vector<pair<const ItemType *, int> > &items)
{
    for (vector<pair<const ItemType *, int> >::const_iterator it = items.begin(); it != items.end(); ++it) {
        knight->addToBackpack(*it->first, it->second);
    }
}

bool Player::respawn()
{
    Mediator &mediator = Mediator::instance();

    // If knight is still alive, then respawn will fail.
    if (knight.lock()) return false;

    // If we do not have a home, then it's game over
    if (getHomeLocation().isNull() && getRespawnType() != R_RANDOM_SQUARE) {

        const bool already_eliminated = getElimFlag();
        
        if (!already_eliminated) {
            mediator.eliminatePlayer(*this);
        }

        if (mediator.gameRunning() && !already_eliminated) {

            // NOTE: These messages ONLY get sent if the player is eliminated by dying in-game.
            // If they are eliminated "externally" (i.e. by quitting or leaving the game) then 
            // the external code is assumed to have already sent a "has quit" or "has left" type 
            // message, so we don't send further messages here. (Trac #36.)

            // Send "PlayerName has been eliminated" to all players.
            std::ostringstream str;
            str << getName() << " has been eliminated.";
            const int nleft = mediator.getNumPlayersRemaining();
            if (nleft > 1) {
                str << " " << nleft << " players are left.";
            }
            mediator.getCallbacks().gameMsg(-1, str.str());
        }
        return true;  // this counts as a "success" -- we do not want the respawn task to keep retrying.
    }
    
    // Find an empty square (at the home, or nearby) to respawn on.
    MapCoord respawn_point = FindRespawnPoint(home_dmap, getHomeLocation());
    if (respawn_point.isNull()) return false;

    // Respawn successful -- Create a new knight.
    shared_ptr<Knight> my_knight(new Knight(*this, backpack_capacities,
        mediator.cfgInt("knight_hitpoints"), H_WALKING, default_item, anim, 100));
    knight = my_knight;

    // Move the knight into the map.
    // NB This is done AFTER setting "knight" because we want getKnight to respond with 
    // the current knight, even before that knight is added to the map. (Otherwise Game
    // won't correctly process the onAddEntity event.)
    my_knight->addToMap(&home_dmap, respawn_point);
    my_knight->setFacing(Opposite(home_facing));

    // Add starting gear
    if (!gears.empty()) {
        giveStartingGear(my_knight, gears.front());
        gears.pop_front();
    }

    // Clear the teleported-recently flag
    setTeleportFlag(false);

    // Cancel any respawn task that might still be in progress.
    respawn_task = shared_ptr<RespawnTask>();

    // Create a KnightTask so that the knight can be controlled.
    shared_ptr<KnightTask> ktsk(new KnightTask(*this));
    TaskManager &tm(mediator.getTaskManager());
    tm.addTask(ktsk, TP_NORMAL, tm.getGVT() + 1);
    
    return true;
}


//
// control functions
//

// This returns the item and tile that caused a control to be generated. This information can
// be used to set up an ActionData. Note that the corresponding dmap can be taken from the
// Knight itself.
void Player::getControlInfo(const Control *ctrl_in, const ItemType *& itype_out,
                            weak_ptr<Tile> &tile_out, MapCoord &tile_mc_out) const
{
    map<const Control *, ControlInfo>::const_iterator it;
    it = current_controls.find(ctrl_in);
    if (it != current_controls.end()) {
        // Note: Item controls always refer to an item in the knight's
        // inventory, not an item in the map. Therefore we have only
        // itype_out, and not item_dmap_out / item_mc_out.
        itype_out = it->second.item_type;
        tile_out = it->second.tile;
        tile_mc_out = it->second.tile_mc;
    }
}


// Work out the possible controls available to the player. This
// includes "standard" controls plus any controls available from items
// or nearby tiles. The available controls are then sent to the
// DungeonView (which is responsible for selecting a control based on
// the joystick/controller input).
// Player::computeAvailableControls is called by KnightTask::execute.
//
void Player::computeAvailableControls()
{
    shared_ptr<Knight> kt = knight.lock();

    map<const Control *, ControlInfo> new_controls;
    
    if (kt && kt->getMap()) {
        // "Generic" controls
        vector<const Control *>::const_iterator it;
        for (it = control_set.begin(); it != control_set.end(); ++it) {
            if (*it) {
                // check if the control is currently possible
                // also, check if it has MS_APPR_BASED flag; if so, the control only applies
                // to approach-based controls.
                ActionData ad;
                ad.setActor(kt);
                ad.setGenericPos(kt->getMap(), kt->getPos());
                ad.setOriginator(Originator(OT_Player(), this));
                if ((*it)->getExecuteFunc().hasValue() && (*it)->checkPossible(ad)
                && (getApproachBasedControls() || ((*it)->getMenuSpecial() & UserControl::MS_APPR_BASED) == 0 )) {
                    new_controls.insert(make_pair(*it, ControlInfo()));
                }
            }
        }

        // Item-based controls
        for (int i=0; i<kt->getBackpackCount(); ++i) {
            addItemControls(kt->getBackpackItem(i), new_controls, kt);
        }
        if (kt->getItemInHand()) {
            addItemControls(*kt->getItemInHand(), new_controls, kt);
        }

        // Tile-based controls
        MapCoord mc = kt->getDestinationPos();
        MapCoord mc_ahead = DisplaceCoord(mc, kt->getFacing());
        const bool app = kt->isApproaching();
        const bool app_based = kt->getPlayer()->getApproachBasedControls();
        const MapHeight ht = kt->getHeight();
        addTileControls(kt->getMap(), mc_ahead, new_controls, true, app, app_based, ht, kt);
        addTileControls(kt->getMap(), mc, new_controls, false, app, app_based, ht, kt);
    }

    if (new_controls != current_controls) {
        current_controls.swap(new_controls);
        vector<pair<const UserControl *, bool> > ctrl_vec;
        for (map<const Control *, ControlInfo>::const_iterator it = current_controls.begin();
        it != current_controls.end(); ++it) {
            ctrl_vec.push_back(make_pair(it->first, it->second.primary));
        }
        Mediator::instance().getCallbacks().setAvailableControls(getPlayerNum(), ctrl_vec);
    }
}


void Player::addTileControls(DungeonMap *dmap, const MapCoord &mc,
                             map<const Control *, ControlInfo> &cmap,
                             bool ahead,
                             bool approaching, 
                             bool approach_based,
                             MapHeight ht, shared_ptr<Creature> cr)
{
    if (!dmap) return;
    if (!ahead && approaching) return;  // can't do current square if approaching the square ahead.

    vector<shared_ptr<Tile> > tiles;
    dmap->getTiles(mc, tiles);

    MapAccess acc = A_CLEAR;
    for (vector<shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
        acc = std::min(acc, (*it)->getAccess(ht));
    }

    for (vector<shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end();
    ++it) {
        const Control *ctrl = (*it)->getControl(mc);
        if (ctrl) {
            bool ok;
            bool primary;
            if (ahead) {
                // square-ahead is ok if there are no entities there.
                // (This prevents us from shutting doors on other knights, or monsters.)
                vector<shared_ptr<Entity> > entities;
                dmap->getEntities(mc, entities);
                ok = (entities.empty());

                // In the approach based system, square-ahead is usually secondary;
                // however, if we are approaching it, or it cannot be approached, then it is primary.
                // In the non-approach based system, it is always primary.
                // (NOTE: in this context, "primary" means it can be activated either by tapping the fire button
                // or by using the menu, while "secondary" means you have to use the menu to activate it.)
                primary = (approaching || !approach_based || acc != A_APPROACH);
            } else {
                // current square is ok if canActivateHere() is true. It is always primary.
                ok = (*it)->canActivateHere();
                primary = true;
            }
            if (ok) {
                ActionData ad;
                ad.setActor(cr);
                ad.setOriginator(cr->getOriginator());
                ad.setTile(dmap, mc, *it);
                ad.setGenericPos(dmap, mc);
                if (ctrl->getExecuteFunc().hasValue() && ctrl->checkPossible(ad)) {
                    ControlInfo ci;
                    ci.tile = *it;
                    ci.tile_mc = mc;
                    ci.primary = primary;
                    cmap.insert(make_pair(ctrl, ci));
                }
            }
        }
    }
}

void Player::addItemControls(const ItemType &itype, map<const Control *, ControlInfo> &cmap,
                             shared_ptr<Creature> cr)
{
    if (!cr) return;
    const Control *ctrl = itype.getControl();
    if (ctrl) {
        ActionData ad;
        ad.setActor(cr);
        ad.setOriginator(Originator(OT_Player(), this));
        ad.setItem(0, MapCoord(), &itype);
        ad.setGenericPos(cr->getMap(), cr->getPos());
        if (ctrl->getExecuteFunc().hasValue() && ctrl->checkPossible(ad)) {
            ControlInfo ci;
            ci.item_type = &itype;
            cmap.insert(make_pair(ctrl, ci));
        }
    }
}


void Player::resetHome(const MapCoord &mc, const MapDirection &facing)
{
    // NOTE: We assume the player is not currently approaching his own home.
    // (This is OK at the moment because homes can only be secured when no-one is
    // approaching them...)
    home_location = mc;
    home_facing = facing;
}


void Player::sendMiniMap(MiniMap &m)
{
    m.setSize(mini_map_width, mini_map_height);
    std::vector<MiniMapColour>::const_iterator it = mapped_squares.begin();
    for (int y = 0; y < mini_map_height; ++y) {
        for (int x = 0; x < mini_map_width; ++x) {
            MiniMapColour col = *it++;
            if (col != COL_UNMAPPED) {
                m.setColour(x, y, col);
            }
        }
    }

    for (std::map<int, std::pair<int, int> >::const_iterator it = knight_locations.begin(); it != knight_locations.end(); ++it) {
        m.mapKnightLocation(it->first, it->second.first, it->second.second);
    }

    for (std::set<std::pair<int, int> >::const_iterator it = item_locations.begin(); it != item_locations.end(); ++it) {
        m.mapItemLocation(it->first, it->second, true);
    }
}

void Player::sendStatusDisplay(StatusDisplay &status_display)
{
    shared_ptr<Knight> kt = getKnight();
    if (kt) {
        // backpack
        for (int i = 0; i < kt->getBackpackCount(); ++i) {
            const ItemType & item_type = kt->getBackpackItem(i);
            const int num_carried = kt->getNumCarried(i);
            const int maxno = kt->getMaxNo(item_type);
            
            status_display.setBackpack(item_type.getBackpackSlot(), item_type.getBackpackGraphic(),
                                       item_type.getBackpackOverdraw(), num_carried, maxno);
        }

        // Health
        status_display.setHealth(kt->getHealth());
        status_display.setPotionMagic(kt->getPotionMagic(), kt->getPoisonImmunity());

        // NOTE: We ignore the quest info stuff for now.
    }

    // Skulls
    for (int i = 0; i < nskulls; ++i) {
        status_display.addSkull();
    }
}

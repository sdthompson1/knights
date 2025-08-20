/*
 * player.cpp
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
 * Player class, and Respawning.
 *
 */

#include "misc.hpp"

#include "action_data.hpp"
#include "control.hpp"
#include "dungeon_map.hpp"
#include "dungeon_view.hpp"
#include "item_type.hpp"
#include "lua_func_wrapper.hpp"
#include "lua_userdata.hpp"
#include "knight.hpp"
#include "knight_task.hpp"
#include "knights_callbacks.hpp"
#include "localization.hpp"
#include "mediator.hpp"
#include "mini_map.hpp"
#include "player.hpp"
#include "rng.hpp"
#include "status_display.hpp"
#include "task.hpp"
#include "task_manager.hpp"
#include "tile.hpp"

#include "include_lua.hpp"

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

    MapCoord FindRespawnPoint(DungeonMap *dmap, MapCoord home_point)
    {
        if (!dmap || home_point.isNull()) {
            return MapCoord();
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
            TryInsert(open, *dmap, closed, DisplaceCoord(mc, D_NORTH));
            TryInsert(open, *dmap, closed, DisplaceCoord(mc, D_SOUTH));
            TryInsert(open, *dmap, closed, DisplaceCoord(mc, D_EAST));
            TryInsert(open, *dmap, closed, DisplaceCoord(mc, D_WEST));
            
            // Now try to respawn on this point
            if (dmap->getAccess(mc, H_WALKING) == A_CLEAR) {
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
        tm.addTask(shared_from_this(), TP_NORMAL, tm.getGVT() + 100); // try again later
    }
}



//
// Player
//

Player::Player(int plyr_num,
               const Anim *a, ItemType * di,
               const std::vector<const Control*> &cs,
               shared_ptr<const ColourChange> sec_home_cc,
               const PlayerID &plyr_id,
               int team_num_)
    : player_num(plyr_num), control(0),
      current_room(-1), current_room_width(0), current_room_height(0),
      mini_map_width(0), mini_map_height(0),
      home_dmap(0), 
      anim(a),
      default_item(di), backpack_capacities(0), control_set(cs),
      secured_home_cc(sec_home_cc),
      nskulls(0), nkills(0), frags(0), player_id(plyr_id), player_state(PlayerState::NORMAL),
      respawn_type(R_NORMAL),
      team_num(team_num_),
      teleport_flag(false), speech_bubble(false), approach_based_controls(true), action_bar_controls(false)
{
}


void Player::addStartingGear(ItemType &itype, const std::vector<int> &numbers)
{
    if (gears.size() < numbers.size()) gears.resize(numbers.size());

    for (int i=0; i<numbers.size(); ++i) {
        gears[i].push_back(std::make_pair(&itype, numbers[i]));
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
        if (control != ctrl) { // skip update if we are not changing anything

            control = ctrl;

            // Wake up the KnightTask immediately, rather than waiting for its poll interval
            // (this reduces control latency)
            TaskManager &tm(Mediator::instance().getTaskManager());
            tm.rmTask(knight_task);
            tm.addTask(knight_task, TP_NORMAL, tm.getGVT());
        }
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

    scheduleRespawn(Mediator::instance().cfgInt("respawn_delay"));

    // Also add a skull
    getStatusDisplay().addSkull();
    nskulls++;

    // Clear speech bubble on death
    setSpeechBubble(false);
}

void Player::scheduleRespawn(int delay)
{
    // NOTE: This function must not call Lua, because called by Player::onDeath
    Mediator &mediator = Mediator::instance();
    shared_ptr<RespawnTask> rt(new RespawnTask(*this));
    respawn_task = rt;
    TaskManager &tm(mediator.getTaskManager());
    tm.addTask(respawn_task, TP_NORMAL, tm.getGVT() + delay);
}


//
// respawn
//

void Player::giveStartingGear(shared_ptr<Knight> knight, const std::vector<std::pair<ItemType *, int> > &items)
{
    for (std::vector<std::pair<ItemType *, int> >::const_iterator it = items.begin(); it != items.end(); ++it) {
        knight->addToBackpack(*it->first, it->second);
    }
}

bool Player::respawn()
{
    Mediator &mediator = Mediator::instance();

    // If knight is still alive, then respawn will fail.
    if (knight.lock()) return false;


    //
    // Find out where to respawn:
    //

    DungeonMap *dmap = 0;
    MapCoord mc;
    MapDirection facing;

    // First run the respawn_func (if there is one).
    if (respawn_func.hasValue()) {
        // Call the respawn func, with "this" as argument.
        lua_State *lua = Mediator::instance().getLuaState();

        // Lua api usage, wrap in pcall
        struct Pcall {
            DungeonMap *& dmap;
            MapCoord &mc;
            MapDirection &facing;
            Player *plyr;
            bool want_retry;
            static int run(lua_State *lua) {
                Pcall *p = static_cast<Pcall*>(lua_touserdata(lua, 1));
                NewLuaPtr<Player>(lua, p->plyr);  // [plyr]
                p->plyr->respawn_func.run(lua, 1, 3);   // [x y dir] or [nil nil nil] or ["retry" nil nil]
                if (lua_tostring(lua, -3) == "retry") {
                    p->want_retry = true;
                } else if (!lua_isnil(lua, -3)) {
                    p->dmap = Mediator::instance().getMap().get();
                    p->mc.setX(lua_tointeger(lua, -3));
                    p->mc.setY(lua_tointeger(lua, -2));
                    p->facing = GetMapDirection(lua, -1);
                }
                return 0;
            }
        } pc = { dmap, mc, facing, this, false };

        PushCFunction(lua, &Pcall::run);   // [func]
        lua_pushlightuserdata(lua, &pc);   // [func arg]
        int err = lua_pcall(lua, 1, 0, 0);
        if (pc.want_retry) {
            // the lua code returned "retry", so we should abort the respawn and return false
            // (which means "please try the respawn again later")
            return false;

        } else if (err) {
            // the lua code raised an error!
            // in this case we should print an error message and then fall through to the 
            // "normal" respawn logic (i.e. as if lua respawn func was not set).

            // lua stack = [errmsg]

            Mediator::instance().gameMsgRaw(
                -1,
                UTF8String::fromUTF8Safe(std::string("(Error in respawn function) ") + lua_tostring(lua, -1)),
                true);
            lua_pop(lua, 1); // []

        } else {
            // []
            // Lua returned a suggested respawn point. Copy this into the dmap, mc, facing variables.
            dmap = pc.dmap;
            mc = pc.mc;
            facing = pc.facing;
        }
    }

    if (!dmap) {
        // There was no respawn function, or, it failed to run.
        // Use the normal "respawn type" logic instead.

        // If respawn type is R_RANDOM_SQUARE then set the spawn point to
        // somewhere completely random
        if (respawn_type == R_RANDOM_SQUARE) {
            dmap = Mediator::instance().getMap().get();
            mc = MapCoord(g_rng.getInt(1, dmap->getWidth() - 1),
                g_rng.getInt(1, dmap->getHeight() - 1));
            facing = MapDirection(g_rng.getInt(0, 4));

        } else {

            // The respawn type is either R_NORMAL or R_DIFFERENT_EVERY_TIME
            // In these cases:
            //  -- If the player has a home location, use that as the respawn point.
            //  -- Otherwise, it's game over for that player.

            // (Note: With R_DIFFERENT_EVERY_TIME, HomeManager is responsible for reassigning the 
            // home on each respawn, not us.)

            // First check whether they have a spawn point.
            if (!this->home_dmap || this->home_location.isNull()) {

                // They don't have one -- Game Over

                const bool already_eliminated = (getPlayerState() == PlayerState::ELIMINATED);
                if (!already_eliminated) {

                    // Send "PlayerName has been eliminated" to all players.
                    std::vector<LocalParam> params;
                    params.push_back(LocalParam(getPlayerID()));
                    const int nleft = mediator.getNumPlayersRemaining() - 1;
                    LocalKey key;
                    if (nleft > 1) {
                        params.push_back(LocalParam(nleft));
                        key = LocalKey("player_eliminated_num");
                    } else {
                        key = LocalKey("player_eliminated");
                    }
                    mediator.getCallbacks().gameMsgLoc(-1, key, params);

                    // Now eliminate the player.
                    mediator.changePlayerState(*this, PlayerState::ELIMINATED);
                }
                return true;  // this counts as a "success" -- we do not want the respawn task to keep retrying.

            } else {

                // They do have a home. Use it as the respawn point.
                dmap = home_dmap;
                mc = home_location;
                facing = Opposite(home_facing);
            }
        }
    }

    // OK so if we get to here, a respawn point has been chosen and put into the 
    // "dmap", "mc" and "facing" variables.
    // Note: these might be "null" (if the respawn is to be temporarily delayed).

    // If the player is disconnected, delay the respawn until they are back
    if (getPlayerState() == PlayerState::DISCONNECTED) {
        return false;
    }

    // If the chosen spawn point is blocked this will move it to a nearby unblocked point
    // (or set mc to null if no unblocked point is available).
    mc = FindRespawnPoint(dmap, mc);

    // If we get to here and either dmap or mc is null, then return false which means "try again later".
    if (!dmap || mc.isNull()) return false;

    // Respawn successful -- Create a new knight.
    shared_ptr<Knight> my_knight(new Knight(*this, backpack_capacities,
        mediator.cfgInt("knight_hitpoints"), H_WALKING, default_item, anim, 100));
    this->knight = my_knight;

    // Move the knight into the map.
    // NB This is done AFTER setting "this->knight" because we want getKnight to respond with 
    // the current knight, even before that knight is added to the map. (Otherwise Game
    // won't correctly process the onAddEntity event.)
    my_knight->addToMap(dmap, mc);
    my_knight->setFacing(facing);

    // Add starting gear
    if (!gears.empty()) {
        giveStartingGear(my_knight, gears.front());
        gears.pop_front();
    }

    // Clear the teleported-recently flag
    setTeleportFlag(false);

    // Cancel any respawn task that might still be in progress.
    this->respawn_task = shared_ptr<RespawnTask>();

    // Cancel/delete existing KnightTask and recreate it
    TaskManager &tm(mediator.getTaskManager());
    tm.rmTask(knight_task);
    knight_task.reset(new KnightTask(*this));
    tm.addTask(knight_task, TP_NORMAL, tm.getGVT() + 1);
    
    // Respawn was successful!
    return true;
}


//
// control functions
//

// This returns the item and tile that caused a control to be generated. This information can
// be used to set up an ActionData. Note that the corresponding dmap can be taken from the
// Knight itself.
void Player::getControlInfo(const Control *ctrl_in, ItemType *& itype_out,
                            weak_ptr<Tile> &tile_out, MapCoord &tile_mc_out) const
{
    std::map<const Control *, ControlInfo>::const_iterator it;
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

    std::map<const Control *, ControlInfo> new_controls;
    
    if (kt && kt->getMap()) {
        // "Generic" controls
        std::vector<const Control *>::const_iterator it;
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
                    new_controls.insert(std::make_pair(*it, ControlInfo()));
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
        std::vector<std::pair<const UserControl *, bool> > ctrl_vec;
        for (std::map<const Control *, ControlInfo>::const_iterator it = current_controls.begin();
        it != current_controls.end(); ++it) {
            ctrl_vec.push_back(std::make_pair(it->first, it->second.primary));
        }
        Mediator::instance().getCallbacks().setAvailableControls(getPlayerNum(), ctrl_vec);
    }
}


void Player::addTileControls(DungeonMap *dmap, const MapCoord &mc,
                             std::map<const Control *, ControlInfo> &cmap,
                             bool ahead,
                             bool approaching, 
                             bool approach_based,
                             MapHeight ht, shared_ptr<Creature> cr)
{
    if (!dmap) return;
    if (!ahead && approaching) return;  // can't do current square if approaching the square ahead.

    std::vector<shared_ptr<Tile> > tiles;
    dmap->getTiles(mc, tiles);

    MapAccess acc = A_CLEAR;
    for (std::vector<shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
        acc = std::min(acc, (*it)->getAccess(ht));
    }

    for (std::vector<shared_ptr<Tile> >::const_iterator it = tiles.begin(); it != tiles.end();
    ++it) {
        const Control *ctrl = (*it)->getControl(mc);
        if (ctrl) {
            bool ok;
            bool primary;
            if (ahead) {
                // square-ahead is ok if there are no entities there.
                // (This prevents us from shutting doors on other knights, or monsters.)
                std::vector<shared_ptr<Entity> > entities;
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
                    cmap.insert(std::make_pair(ctrl, ci));
                }
            }
        }
    }
}

void Player::addItemControls(ItemType &itype, std::map<const Control *, ControlInfo> &cmap,
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
            cmap.insert(std::make_pair(ctrl, ci));
        }
    }
}


void Player::resetHome(DungeonMap *dmap, const MapCoord &mc, const MapDirection &facing)
{
    // NOTE: We assume the player is not currently approaching his own home.
    // (This is OK at the moment because homes can only be secured when no-one is
    // approaching them...)
    home_dmap = dmap;
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
    }

    // Skulls
    for (int i = 0; i < nskulls; ++i) {
        status_display.addSkull();
    }
}

/*
 * knight_task.cpp
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

#include "misc.hpp"

#include "action_data.hpp"
#include "anim.hpp"
#include "control.hpp"
#include "item.hpp"
#include "knight.hpp"
#include "knight_task.hpp"
#include "knights_callbacks.hpp"
#include "mediator.hpp"
#include "player.hpp"
#include "task_manager.hpp"


void KnightTask::doControls(shared_ptr<Knight> knight)
{
    // Compute available controls
    player.computeAvailableControls();

    // Read the player's controller input.
    const Control * ctrl = player.readControl();

    // If we didn't get anything from the controller, then use
    // stored_control instead.
    if (!ctrl) ctrl = stored_control;

    // Clear the stored_control
    stored_control = 0;

    bool possible = true;
    ActionData ad;
    
    // If we didn't get a valid control, then can't proceed
    if (!ctrl || !ctrl->getExecuteFunc().hasValue()) possible = false;

    if (possible) {
        // If knight is stunned, and the control cannot be executed while stunned, then can't proceed
        if (knight->isStunned() && !ctrl->canExecuteWhileStunned()) possible = false;
        
        // If knight is moving, and the control cannot be executed while moving, then can't proceed
        // (but if the control is non-continuous, save it into stored_control, as
        // we may be able to execute it later, when the motion finishes).
        if (knight->isMoving() && !ctrl->canExecuteWhileMoving()) {
            if (!ctrl->isContinuous()) {
                stored_control = ctrl;
            }
            possible = false;
        }
        
        // check Lua possible function. Also set up the ActionData.
        if (possible) {
            const ItemType *item_type = 0;
            weak_ptr<Tile> tile;
            MapCoord tile_mc;
            player.getControlInfo(ctrl, item_type, tile, tile_mc);
            ad.setActor(knight);
            ad.setOriginator(knight->getOriginator());
            shared_ptr<Tile> tile_lock = tile.lock();
            if (item_type) ad.setItem(0, MapCoord(), item_type);
            if (tile_lock) {
                ad.setTile(knight->getMap(), tile_mc, tile_lock);
            }
            ad.setGenericPos(knight->getMap(), 
                             tile_lock ? tile_mc : knight->getPos());
            possible = ctrl->checkPossible(ad);
        }
    }
    
    // If knight non-stunned then update the menu-highlight
    // (whether control possible or not).
    if (!knight->isStunned()) {
        KnightsCallbacks &callbacks = Mediator::instance().getCallbacks();
        if (possible) callbacks.setMenuHighlight(player.getPlayerNum(), ctrl);
        else callbacks.setMenuHighlight(player.getPlayerNum(), 0);
    }

    // Execute control if possible
    if (possible) {
        // first clear any crossbow animation
        // (if we don't do this then e.g. knight will still be on AF_XBOW_LOAD when he starts moving)
        if (knight->getAnimFrame() == AF_XBOW_LOAD) knight->setAnimFrame(AF_XBOW, 0);

        // now execute it
        ctrl->getExecuteFunc().execute(ad);
    }
}

    
void KnightTask::execute(TaskManager &tm)
{
    // This routine calls Player::computeAvailableControls at regular
    // intervals. It might be better to call computeAvailableControls
    // only when needed (eg after motion, or after any change to
    // items, tiles etc in a square adjacent to the knight), ie in an
    // event driven way. That would be more efficient, but also more
    // complicated than what we are doing now.

    // This routine is also responsible for executing controls
    // selected by the player (information on the selected controls is
    // read from DungeonView).

    // Also: we do crossbow loading here.


    // If player's knight is not in a valid map, exit.
    shared_ptr<Knight> knight = player.getKnight();
    if (!knight || !knight->getMap()) return;
    

    // Handle controls
    doControls(knight);


    // Now do the crossbow loading
    const int gvt = tm.getGVT();    
    const ItemType *held_item = knight->getItemInHand();
    bool can_load_xbow = !knight->isStunned()
        && !knight->isMoving()
        && held_item
        && held_item->canLoad()
        && held_item->getAmmo()
        && knight->getNumCarried(*held_item->getAmmo()) > 0
        && !knight->isApproaching();
    if (!can_load_xbow) {
        xbow_load_time = 0;
        xbow_action_timer = 0;
    } else if (xbow_load_time == 0) {
        int reload_time = held_item->getReloadTime();
        if (knight->hasQuickness()) reload_time /= 2;
        xbow_load_time = gvt + reload_time;
        xbow_action_timer = 0;
    } else if (gvt >= xbow_load_time) {
        knight->setItemInHand(held_item->getLoaded());
        knight->rmFromBackpack(*held_item->getAmmo(), 1);
        xbow_load_time = 0;
        xbow_action_timer = 0;
    }
    held_item = knight->getItemInHand();  // update held_item (item-in-hand may have been changed above)

    // Play clicking sound while crossbow is loading (reload_action).
    if (held_item && xbow_load_time != 0 && held_item->getReloadAction().hasValue()
    && gvt >= xbow_action_timer + held_item->getReloadActionTime()) {
        xbow_action_timer = gvt;
        ActionData ad;
        ad.setActor(knight);
        ad.setOriginator(knight->getOriginator());
        ad.setGenericPos(knight->getMap(), knight->getPos());
        held_item->getReloadAction().execute(ad);
    }   

    // Set anim frame for crossbow loading if necessary
    int af = knight->getAnimFrame();
    if (af == AF_NORMAL || af == AF_XBOW || af == AF_XBOW_LOAD) {
        if (held_item) {
            if (held_item->canLoad() && xbow_load_time != 0) {
                knight->setAnimFrame(AF_XBOW_LOAD, 0);
            } else if (held_item->canShoot() || held_item->canLoad()) {
                knight->setAnimFrame(AF_XBOW, 0);
            } else {
                knight->setAnimFrame(AF_NORMAL, 0);
            }
        } else {
            knight->setAnimFrame(AF_NORMAL, 0);
        }
    }

    
    // Now set up for next time. Task should re-execute at
    // min(gvt+control_poll_interval, stunned_until, moving_until).

    int new_time = gvt + Mediator::instance().cfgInt("control_poll_interval");

    if (knight->isMoving()) {
        const int arr_time = knight->getArrivalTime();
        if (arr_time < new_time) new_time = arr_time;
    }

    bool known;
    int stunned_until;
    knight->stunnedUntil(known, stunned_until);
    if (known && stunned_until < new_time) new_time = stunned_until;
    
    tm.addTask(shared_from_this(), TP_NORMAL, new_time);
}


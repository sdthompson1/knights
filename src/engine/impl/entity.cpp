/*
 * entity.cpp
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

#include "anim.hpp"
#include "entity.hpp"
#include "map_helper.hpp"
#include "mediator.hpp"
#include "task_manager.hpp"

//
// function that computes travel times
//

namespace {
    int TravelTime(int dist, int speed)
    {
        const int result = Mediator::instance().cfgInt("walk_time") * dist * 100 / (speed * 1000);
        return (result > 0) ? result : 1;   // result should be at least 1.
    }
}

//
// MotionTask: helper class. Stops motion when an entity reaches its destination.
//

class MotionTask : public Task {
public:
    MotionTask(weak_ptr<Entity> a) : agt(a) { }
    virtual void execute(TaskManager &);
private:
    weak_ptr<Entity> agt;
};


void MotionTask::execute(TaskManager &tm)
{
    // Check entity is valid
    shared_ptr<Entity> entity(agt.lock());
    if (!entity) return;
    if (!entity->getMap()) return;

    // the entity may have stopped moving and/or started a different
    // move with a different arrival time, so check for this.
    // (The motion task should always run at arrival_time - 1.)
    if (!entity->isMoving()) return;
    if (tm.getGVT() < entity->getArrivalTime()-1) return;

    // Stop the motion
    MotionType mt = entity->getMotionType();
    if (mt == MT_MOVE) {
        // moved a complete square. shift position forward one square.
        entity->reposition(DisplaceCoord(entity->getPos(), entity->getFacing()));
    } else {
        entity->flags &= (~Entity::MT_BITS);
        if (mt == MT_WITHDRAW) entity->flags &= (~Entity::APPROACHING_BIT);
    }
    entity->arrival_time = 0;

    // Tell mediator
    // nb. entity may have been removed from map by onReposition events; we must not call
    // Mediator::onChangeEntityMotion if this is the case.
    if (entity->getMap()) {
        Mediator::instance().onChangeEntityMotion(entity, false);
    }
}



//
// Entity
//

Entity::Entity()
    : dmap(0), anim(0), anim_frame(0), anim_tzero(0), anim_invuln(false), overlay(0),
      start_time(0), start_offset(0), arrival_time(0), speed(100), flags(VISIBLE_BIT)
{ }


//
// getNearestPos
//

MapCoord Entity::getNearestPos() const
{
    const int halfway_point = 500;
    if (getOffset() < halfway_point) return getPos();
    else return DisplaceCoord(getPos(), getFacing());
}


//
// getTargettedPos
//

MapCoord Entity::getTargettedPos() const
{
    if (isApproaching()) return DisplaceCoord(getPos(), getFacing());
    else return getDestinationPos();
}

//
// getDestinationPos
//

MapCoord Entity::getDestinationPos() const
{
    if (getMotionType() == MT_MOVE) return DisplaceCoord(getPos(), getFacing());
    else return getPos();       
}


//
// map positioning
//

void Entity::addToMap(DungeonMap *new_dmap, const MapCoord &new_pos)
{
    if (dmap || !new_dmap) return;
    
    // Set motion data.
    cancelAllMotion();

    // Add to the map
    dmap = new_dmap;
    pos = new_pos;
    MapHelper::addEntity(shared_from_this());

    // Tell mediator
    Mediator::instance().onAddEntity(shared_from_this());

    clearDaggerThrownFlag();
}

void Entity::rmFromMap()
{
    if (!dmap) return;
    
    // Tell mediator
    Mediator::instance().onRmEntity(shared_from_this());

    // Remove from the map
    MapHelper::rmEntity(shared_from_this());
    dmap = 0;
    pos = MapCoord();
}

void Entity::reposition(const MapCoord &new_pos)
{
    if (!dmap) return;
    shared_ptr<Entity> self(shared_from_this());

    // Remove from map
    MapHelper::rmEntity(self);
    
    // Set new position, and cancel motion
    pos = new_pos;
    cancelAllMotion();

    // Add to map
    MapHelper::addEntity(self);

    // Tell mediator
    Mediator::instance().onRepositionEntity(shared_from_this());

    clearDaggerThrownFlag();
}

void Entity::setFacing(MapDirection dir)
{
    if (isMoving() || isApproaching()) return;

    // Change the facing direction
    flags &= (~FACING_MASK);
    flags |= (int(dir) << FACING_SHIFT);

    // Tell mediator
    Mediator::instance().onChangeEntityFacing(shared_from_this());

    clearDaggerThrownFlag();
}



//
// visibility
//

void Entity::setVisible(bool v)
{
    if (isVisible() != v) {
        if (v) flags |= VISIBLE_BIT;
        else flags &= (~VISIBLE_BIT);
        Mediator::instance().onChangeEntityVisible(shared_from_this());
    }
}


//
// Anim/overlay routines
//

void Entity::setAnim(const Anim * new_anim)
{
    if (anim != new_anim) {
        anim = new_anim;
        if (getMap()) {
            // setAnim can be called from the ctor, before shared_from_this is available
            // Hence the check "if (getMap())" above.
            Mediator::instance().onChangeEntityAnim(shared_from_this());
        }
    }
}

void Entity::setAnimFrame(int frame, int tzero)
{
    if (anim_frame != frame || anim_tzero != tzero) {
        anim_frame = frame;
        anim_tzero = tzero;
        Mediator::instance().onChangeEntityAnim(shared_from_this());
    }
}

void Entity::setFrameToZeroImmediately()
{
    if (anim_tzero != 0) setAnimFrame(0, 0);
}

void Entity::setAnimInvulnerability(bool i)
{
    if (anim_invuln != i) {
        anim_invuln = i;
        Mediator::instance().onChangeEntityAnim(shared_from_this());
    }
}

void Entity::setOverlay(const Overlay *new_overlay)
{
    if (overlay != new_overlay) {
        overlay = new_overlay;
        if (getMap()) { // prevent problems if overlay changed before shared_from_this() is available
            Mediator::instance().onChangeEntityAnim(shared_from_this());
        }
    }
}

int Entity::getAnimFrame() const
{
    if (dmap) return (Mediator::instance().getGVT() < anim_tzero || anim_tzero <= 0)
                  ? anim_frame : 0;
    else return 0;
}

void Entity::getAnimData(const Anim *&a, const Overlay *&ov, int &f, int &zt, bool &inv) const
{
    a = anim;
    ov = overlay;
    f = getAnimFrame();
    zt = anim_tzero;
    inv = anim_invuln;
}



//
// Motion routines (see also MotionTask above)
//

void Entity::move(MotionType mt, bool missile_mode /* = false */)
{
    Mediator &mediator = Mediator::instance();
    const int approach_offset = mediator.cfgInt("approach_offset");
    const int points_per_square = 1000;

    // Get some constants
    if (!dmap) return;
    if (mt == MT_NOT_MOVING) return;
    if (getSpeed() <= 0) return;
    TaskManager &tm(mediator.getTaskManager());
    const int gvt = tm.getGVT();
    shared_ptr<Entity> self(shared_from_this());
    
    // Remove from map
    MapHelper::rmEntity(self);

    // Set start_offset and compute distance
    int dist;
    switch (mt) {
    case MT_APPROACH:
        start_offset = 0;
        dist = approach_offset;
        break;
    case MT_WITHDRAW:
        start_offset = approach_offset;
        dist = approach_offset;
        break;
    case MT_MOVE:
        if (missile_mode) {
            start_offset = dist = 500;
        } else if (isApproaching()) {
            start_offset = approach_offset;
            dist = 1000 - approach_offset;
        } else {
            start_offset = 0;
            dist = 1000;
        }
        break;
    default:
        ASSERT(0);
        break;
    }

    // Set motion flags and start_time, arrival_time
    const int travel_time = TravelTime(dist, getSpeed());
    start_time = gvt;
    arrival_time = travel_time + gvt;
    flags = (flags & ~(MT_BITS+APPROACHING_BIT)) | int(mt);
    if (mt == MT_APPROACH || mt == MT_WITHDRAW) flags |= APPROACHING_BIT;

    // Add to map
    MapHelper::addEntity(self);

    // Set a MotionTask, to be run when the motion finishes.
    shared_ptr<MotionTask> task(new MotionTask(self));
    tm.addTask(task, TP_NORMAL, arrival_time - 1);

    // Tell Mediator.
    Mediator::instance().onChangeEntityMotion(self, missile_mode);
}

void Entity::flipMotion()
{
    Mediator &mediator = Mediator::instance();
    const int turn_delay = mediator.cfgInt("turn_delay");
    
    if (!dmap) return;
    if (!isMoving() || isApproaching()) return;
    if (getSpeed() <= 0) return;
    TaskManager &tm(mediator.getTaskManager());
    const int gvt = tm.getGVT();
    shared_ptr<Entity> self(shared_from_this());

    // Compute the new motion parameters
    int old_ofs = getOffset();
    if (old_ofs == 0) old_ofs = 1;
    const int travel_time = TravelTime(old_ofs, getSpeed());
    const int new_start_time = gvt + turn_delay;
    const int new_arrival_time = gvt + turn_delay + travel_time;

    // Update the entity
    MapHelper::rmEntity(self);
    const MapDirection old_facing = getFacing();
    pos = DisplaceCoord(pos, old_facing);
    flags &= (~FACING_MASK);
    flags |= (int(Opposite(old_facing)) << FACING_SHIFT);
    start_time = new_start_time;
    arrival_time = new_arrival_time;
    MapHelper::addEntity(self);

    // Add a new MotionTask
    // (The old one will automatically cancel itself when it executes.)
    shared_ptr<MotionTask> task(new MotionTask(self));
    tm.addTask(task, TP_NORMAL, arrival_time - 1);

    // Tell Mediator.
    mediator.onFlipEntityMotion(self);
}


int Entity::getOffset() const
{
    Mediator &mediator = Mediator::instance();
    const int approach_offset = mediator.cfgInt("approach_offset");
    
    if (!getMap()) return 0;
    const MotionType mt = getMotionType();
    if (mt == MT_NOT_MOVING) {
        if (isApproaching()) return approach_offset;
        else return 0;
    } else {
        const int gvt = mediator.getGVT();
        if (gvt < start_time) {
            return start_offset;
        } else {
            int ep;
            switch (mt) {
            case MT_MOVE:
                ep = 1000;
                break;
            case MT_APPROACH:
                ep = approach_offset;
                break;
            case MT_WITHDRAW:
                ep = 0;
                break;
            default:
                ASSERT(0);
                break;
            }
            if (gvt > arrival_time) {
                return ep;
            } else {
                const int result 
                    = start_offset + (ep - start_offset) * (gvt - start_time) / (arrival_time - start_time);
                // result should always be in correct range, but just to be on safe side, we clamp it to
                // 0-1000 range.
                if (result < 0) return 0;
                else if (result > 1000) return 1000;
                else return result;
            }
        }
    }
}


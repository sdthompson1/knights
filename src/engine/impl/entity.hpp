/*
 * entity.hpp
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
 * Entities are things that can move. Basically this is just Creatures
 * and Missiles (at the moment).
 *
 */

#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "map_support.hpp"

#include "boost/enable_shared_from_this.hpp"
#include "boost/shared_ptr.hpp"
using namespace boost;

#include <functional>

class Anim;
class ColourChange;
class DungeonMap;
class Overlay;
class Player;

class Entity : public enable_shared_from_this<Entity> {
    friend class MapHelper;
    friend class MotionTask;

public:
    Entity();
    virtual ~Entity() { }

    // add to or remove from dmap; reposition; set facing.
    // NB don't try to place an entity in an illegal position (eg outside the map)...
    void addToMap(DungeonMap *dmap_, const MapCoord &position);
    void rmFromMap();
    void reposition(const MapCoord &position); // reposition to centre of a square
    void setFacing(MapDirection);
    void flipMotion();   // Can be used when entity is MT_MOVING; will turn around and go back.

    // map and position accessors
    // NB if getMap() is non-null then getPos() always returns a valid position within the
    // map.
    const DungeonMap *getMap() const { return dmap; }
    DungeonMap *getMap() { return dmap; }
    const MapCoord &getPos() const { return pos; }         // base square
    MapCoord getNearestPos() const; // get base sq if ofs < 50%, or sq ahead otherwise.
    MapCoord getTargettedPos() const;  // current sq, unless approaching, in which case sq one ahead.
    MapCoord getDestinationPos() const;  // current sq, unless moving (not approaching), in which case sq one ahead.
    MapDirection getFacing() const { return MapDirection(flags >> FACING_SHIFT); }
    
    // visibility. This is used for invisibility potions. (Default is visible.)
    bool isVisible() const { return (flags & VISIBLE_BIT) != 0; }
    void setVisible(bool);

    // Entity::isVisibleToPlayer checks if (a) this entity does not have INVISIBILITY
    // *or* (b) this entity is a teammate of the given player. (Overridden in Knight.)
    // Note: Not to be confused with ViewManager::entityVisibleToPlayer().
    virtual bool isVisibleToPlayer(const Player &p) const { return isVisible(); }

    // Anim controls what the "lower" graphic looks like.
    // OverlayGraphic controls the "upper" graphic.
    void setAnim(const Anim * anim);
    void setAnimFrame(int frame, int tzero);  // NB tzero=0 means "tzero not used".
    void setFrameToZeroImmediately();   // If tzero!=0 then set frame to AF_NORMAL, else do not change frame.
    void setAnimInvulnerability(bool);
    void setOverlay(const Overlay * overlay);

    // accessors for anim data (used by DungeonViews)
    // (getAnimFrame is used by KnightTask...)
    void getAnimData(const Anim * &a, const Overlay * &ov, int &f, int &zt, bool &inv) const;
    int getAnimFrame() const;
    
    // Move the entity
    // NOTE: missile_mode is a special case hack for newly created missiles
    // (to make them start half a square forward).
    void move(MotionType mt, bool missile_mode = false);

    // query functions for motion
    // getMotionType:      motion type (move, approach, withdraw, reverse, none.)
    // isMoving:           entity is currently in motion.
    // getArrivalTime:     if moving, the gvt at which entity will stop moving. (This will
    //                     return 0 if we're not moving; otherwise the return value will
    //                     always be greater than the current gvt.)
    // getStartTime:       get time at which motion proper will start (only relevant for
    //                     FlipEntityMotion)
    // isApproaching:      entity is either approaching, or has already approached, or is
    //                     withdrawing.
    // getOffset:          works out current offset.
    MotionType getMotionType() const { return MotionType(flags & MT_BITS); }
    bool isMoving() const { return (getMotionType() != MT_NOT_MOVING); }
    int getStartTime() const { return arrival_time==0 ? 0 : start_time; }
    int getArrivalTime() const { return arrival_time; }
    bool isApproaching() const { return (flags & APPROACHING_BIT) != 0; }
    int getOffset() const;

    // Get/set the motion speed of the entity. (100 is the default speed,
    // corresponding to a normal knight.)
    // NB -- if setSpeed is called during motion then the change will
    // not come into effect until the motion finishes.
    int getSpeed() const { return speed; }
    void setSpeed(int spd) { speed = spd; } 

    //
    // Virtual Functions
    //

    // Get height of this entity.
    // (The height is used to decide what squares the entity can move into.)
    virtual MapHeight getHeight() const = 0;

protected:
    // Dagger thrown flag. Slight hack, see Creature::doThrow.
    bool getDaggerThrownFlag() const { return (flags & DAGGER_THROWN_BIT) != 0; }
    void setDaggerThrownFlag() { flags |= DAGGER_THROWN_BIT; }
    void clearDaggerThrownFlag() { flags &= (~DAGGER_THROWN_BIT); }
    
private:
    void cancelAllMotion() { arrival_time = 0; flags &= ~(MT_BITS + APPROACHING_BIT); }
    
private:
    // map data
    DungeonMap *dmap;
    MapCoord pos;

    // anim data
    // frame is "anim_frame" for gvt < tzero and "0" for gvt >= tzero,
    // unless tzero<=0, in which case frame is always "anim_frame".
    // anim_invuln --> determines which colour change to use (normal or invulnerable).
    const Anim *anim;
    int anim_frame, anim_tzero;
    bool anim_invuln;
    const Overlay *overlay;

    // motion data (see also flags)
    int start_time;      // time at which motion starts. (valid only if arrival_time != 0)
    int start_offset;    // offset when the motion started. (valid only if arrival_time != 0)
    int arrival_time;    // time at which motion finishes, or 0 if we are not moving.
    // NOTE: "finish_offset" is implicit, based on the motion type.

    // motion speed
    int speed;
    
    // flags.
    // bits 0-2: motion type.
    // bit 3: set if approaching.
    // bit 4: set if visible.
    // bit 5: dagger-thrown flag.
    // bits 6-7: facing direction.
    enum { MT_BITS = 7, APPROACHING_BIT = 8,
           VISIBLE_BIT = 16, FACING_MASK = 192, FACING_SHIFT = 6,
           DAGGER_THROWN_BIT = 32 };
    unsigned char flags;
};

//
// function objects to compare entities by height
//

struct HeightCompare {
    bool operator()(const shared_ptr<Entity> &lhs, const shared_ptr<Entity> &rhs) const {
        return lhs->getHeight() < rhs->getHeight();
    }
};

struct HeightReverseCompare {
    bool operator()(const shared_ptr<Entity> &lhs, const shared_ptr<Entity> &rhs) const {
        return lhs->getHeight() > rhs->getHeight(); 
    }
};

#endif

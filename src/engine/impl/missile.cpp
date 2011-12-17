/*
 * missile.cpp
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

#include "misc.hpp"

#include "creature.hpp"
#include "dungeon_map.hpp"
#include "item.hpp"
#include "item_type.hpp"
#include "mediator.hpp"
#include "missile.hpp"
#include "player.hpp"
#include "rng.hpp"
#include "task_manager.hpp"
#include "tile.hpp"

class MissileTask : public Task {
public:
    MissileTask(weak_ptr<Missile> m) : missile(m) { }
    virtual void execute(TaskManager &);
private:
    weak_ptr<Missile> missile;
};

void MissileTask::execute(TaskManager &tm)
{
    // Check missile is valid
    shared_ptr<Missile> m(missile.lock());
    if (!m) return;
    if (!m->getMap()) return;

    bool delete_from_map = false;

    // Check to see if we have hit something
    shared_ptr<Creature> cr = m->getMap()->getTargetCreature(*m, false);
    if (cr) {

        // There is a creature present. See what kind of hit it is.
        Missile::HitResult hit_type = m->canHitPlayer(cr->getPlayer());

        // If hit type is IGNORE then proceed as if the creature was not there (i.e. missile goes right through it)
        // (This is to prevent crossbow bolts hitting the knight that fires them.)

        if (hit_type != Missile::IGNORE) {

            // The missile has a chance to hit something
            const int hit_chance = m->range_left * m->itype.getMissileHitMultiplier() + 1;
            if (g_rng.getBool(1.0f - 1.0f / hit_chance)) {

                // OK the missile successfully hit.
                // If hit type is CAN_HIT then process damage and delete the missile from the map.
                // If hit type is FRIENDLY_FIRE then delete missile from map, but don't do any damage.

                if (hit_type == Missile::CAN_HIT) {
                    const RandomInt *st = m->itype.getMissileStunTime();
                    const RandomInt *dam = m->itype.getMissileDamage();
                    const int stun_time = st? st->get() : 0;
                    const int damage = dam? dam->get() : 0;
                    cr->damage(damage, m->getOwner(), stun_time + tm.getGVT());
                }

                delete_from_map = true;
            }
        }
    }

    // Have we arrived at the next square yet?
    if (!m->isMoving()) {

        // We have
        bool do_hook = false;

        // Decrease range (taking stairs into account)
        MapCoord mc_ahead = DisplaceCoord(m->getPos(), m->getFacing());
        --(m->range_left);
        vector<shared_ptr<Tile> > tiles;
        m->getMap()->getTiles(mc_ahead, tiles);
        for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end(); ++it) {
            if ((*it)->isStair()) {
                MapDirection dn = (*it)->stairsDownDirection();
                if (dn == m->getFacing()) {
                    // going down. add to range
                    if ((mc_ahead.getX() + mc_ahead.getY()) % 2 == 0) {
                        ++(m->range_left);
                    }
                } else if (dn == Opposite(m->getFacing())) {
                    // going up. subtract from range
                    --(m->range_left);
                }
                break;
            }
        }
        if (m->range_left <= 0) {
            delete_from_map = true;
            do_hook = true;
        }
    
        // Check access ahead (and at current sq).
        if (!delete_from_map) {
            const MapAccess ma = m->getMap()->getAccess(mc_ahead,
                                                        MapHeight(H_MISSILES + m->getFacing()));
            if (ma == A_APPROACH) {
                // 'partial' missile access
                int chance = m->itype.getMissileAccessChance();
                if (g_rng.getBool((100-chance) / 100.0f)) {
                    delete_from_map = true;
                    do_hook = true;
                }
            } else if (ma == A_BLOCKED) {
                delete_from_map = true;
                do_hook = true;
            }
        }

        if (do_hook) {
            Mediator::instance().runHook("HOOK_MISSILE_MISS", m->getMap(), m->getPos());
        }
    }
    
    if (delete_from_map) {
        // Remove the missile from the map. Drop item if necessary.
        // NOTE: we don't check if the drop was successful; if there was nowhere to
        // place the missile then tough, the item is lost. (Although we do set "allow_nonlocal"
        // so as to give the best chance of finding a place for the item.)
        if (m->drop_after) {
            shared_ptr<Item> item(new Item(m->itype));
            DropItem(item, *m->getMap(), m->getNearestPos(), true, false, m->getFacing(), shared_ptr<Creature>());
        }
        m->rmFromMap();
    } else {

        // Move on if necessary
        if (!m->isMoving()) {
            m->move(MT_MOVE);
        }

        // Reschedule. We check for collisions every "missile_check_interval" but make sure
        // to run the missile task at arrival_time+1 if that is sooner.
        const int new_time = min(tm.getGVT() + Mediator::instance().cfgInt("missile_check_interval"),
                                 m->getArrivalTime() + 1);
        tm.addTask(shared_from_this(), TP_NORMAL, new_time);
    }
}

bool CreateMissile(DungeonMap &dmap, const MapCoord &mc, MapDirection dir,
                   const ItemType &itype, bool drop_after, bool with_strength,
                   const Originator &owner, bool allow_friendly_fire)
{
    // Check if we can place a missile here
    if (!dmap.canPlaceMissile(mc, dir)) return false;

    // Check access ahead. If A_APPROACH ("partial missile access") then check random chance that missile cant be placed
    const MapAccess acc = dmap.getAccess(DisplaceCoord(mc, dir), MapHeight(H_MISSILES + dir));
    if (acc == A_APPROACH) {
        if (g_rng.getBool((100 - itype.getMissileAccessChance()) / 100.0f)) {
            return false;
        }
    }
    
    Mediator &mediator = Mediator::instance();

    // Create the missile
    shared_ptr<Missile> m(new Missile(itype, drop_after, owner, allow_friendly_fire));
    if (with_strength) m->doubleRange();
    m->setFacing(dir);
    m->addToMap(&dmap, mc);
    m->move(MT_MOVE, true);  // start it forward half a square
    shared_ptr<MissileTask> mt(new MissileTask(m));
    const int new_time = min(mediator.getGVT() + mediator.cfgInt("missile_check_interval"),
                              m->getArrivalTime() + 1);
    mediator.getTaskManager().addTask(mt, TP_NORMAL, new_time);
    return true;
}

Missile::Missile(const ItemType &it, bool da, const Originator & ownr, bool aff)
    : itype(it), range_left(it.getMissileRange()), drop_after(da), owner(ownr), allow_friendly_fire(aff)
{
    setAnim(it.getMissileAnim());
    setSpeed(it.getMissileSpeed());
}

MapHeight Missile::getHeight() const
{
    return MapHeight(H_MISSILES + getFacing());
}

Missile::HitResult Missile::canHitPlayer(const Player *pl) const
{
    // If friendly fire is allowed then we can hit anything
    if (allow_friendly_fire) return CAN_HIT;

    // If the target is a monster (!pl), or the missile was NOT shot by a player (!owner.isPlayer()),
    // then the missile can hit
    if (!pl) return CAN_HIT;
    if (!owner.isPlayer()) return CAN_HIT;
    
    // If the firer and target are both the same player then it is an IGNORE situation
    // (e.g. crossbow bolt hitting the firer)
    if (pl == owner.getPlayer()) return IGNORE;

    // Otherwise, it is either CAN_HIT or FRIENDLY_FIRE depending on whether players 
    // are on same team or different team
    if (owner.getPlayer()->getTeamNum() == -1 || pl->getTeamNum() == -1) {
        // Not a team game. So it is always CAN_HIT
        return CAN_HIT;
    } else {
        // Team game. Friendly fire if the teams match, otherwise CAN_HIT.
        if (owner.getPlayer()->getTeamNum() == pl->getTeamNum()) return FRIENDLY_FIRE;
        else return CAN_HIT;
    }
}

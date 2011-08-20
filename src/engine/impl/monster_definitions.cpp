/*
 * monster_definitions.cpp
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

#include "anim.hpp"
#include "dungeon_map.hpp"
#include "item.hpp"
#include "mediator.hpp"
#include "monster_definitions.hpp"
#include "monster_manager.hpp"
#include "monster_support.hpp"
#include "rng.hpp"
#include "special_tiles.hpp"
#include "task.hpp"
#include "task_manager.hpp"
#include "tile.hpp"

#include "random_int.hpp"
using namespace KConfig;


//
// vampire bats
//

class VampireBatAI : public Task {
public:
    explicit VampireBatAI(weak_ptr<VampireBat> v) : vbat(v), next_bite_time(0) { }
    virtual void execute(TaskManager &tm);
private:
    weak_ptr<VampireBat> vbat;
    int next_bite_time;
};

namespace {
    struct Always {
        bool operator()(shared_ptr<Knight> kt) const { return true; }
    };

    struct BatCanEnter {
        bool operator()(DungeonMap &dmap, const MapCoord &mc) {
            return (dmap.getAccess(mc, H_FLYING) == A_CLEAR);
        }
    };


    bool TargetUnderneathMe(const Creature &me, const Creature &target)
    {
        Mediator &mediator = Mediator::instance();
        const int bat_targetting_offset = mediator.cfgInt("bat_targetting_offset");

        int dist = 0;

        if (target.getPos() == me.getPos()) {
            if (target.getFacing() == me.getFacing()) {
                dist = abs(target.getOffset() - me.getOffset());
            } else {
                dist = target.getOffset() + me.getOffset();
            }
        } else if (target.getPos() == DisplaceCoord(me.getPos(), me.getFacing())) {
            if (target.getFacing() == Opposite(me.getFacing())) {
                dist = abs(1000 - me.getOffset() - target.getOffset());
            } else {
                dist = 1000 - me.getOffset() + target.getOffset();
            }
        } else {
            return false;
        }

        return dist < bat_targetting_offset;
    }


    void ReplaceTask(TaskManager &tm, shared_ptr<Task> task, Monster &mon, bool replace_halfway_through_move)
    {
        const int monster_wait_time = Mediator::instance().cfgInt("monster_wait_time");
        const int gvt = tm.getGVT();

        bool can_act = !mon.isStunned() && !mon.isMoving();
        int cannot_act_until;
        if (!can_act) {
            // Set cannot_act_until from both stunned and motion.
            bool known;
            mon.stunnedUntil(known, cannot_act_until);
            if (mon.isMoving()) {
                const int arrival_time = mon.getArrivalTime();
                if (!known || arrival_time > cannot_act_until) {
                    known = true;
                    if (replace_halfway_through_move) {
                        cannot_act_until = gvt + (arrival_time - gvt) / 2;
                    } else {
                        cannot_act_until = arrival_time;
                    }
                }
            }
            if (!known) can_act = true;   // Treat unknown case as if we can act (ie. just wait a preset delay time).
        }
        if (can_act) {
            // Monster did not do anything. Wait for a preset delay time.
            tm.addTask(task, TP_LOW, gvt + monster_wait_time);
        } else {
            // Wait until the monster's current action finishes.
            tm.addTask(task, TP_LOW, cannot_act_until + 1);
        }
    }
}

void VampireBatAI::execute(TaskManager &tm)
{
    Mediator &mediator = Mediator::instance();
    const float monster_wait_chance_as_fraction = mediator.cfgInt("monster_wait_chance") / 100.0f;
    const int bat_bite_wait = mediator.cfgInt("bat_bite_wait");

    shared_ptr<VampireBat> bat = vbat.lock();
    if (!bat || !bat->getMap()) return;  // the bat has died

    // These vars store whether we should move (and in which dir) and whether we should bite
    pair<MapDirection,bool> p;
    p.second = false;
    bool bite = false;

    // This determines whether we will attempt a bite when we are halfway through our movement
    bool allow_bite_halfway = true;

    // Find a target.
    shared_ptr<Knight> target = FindClosestKnight(bat, Always());

    // Are we allowed to bite the target?
    const bool bite_allowed = tm.getGVT() >= next_bite_time && !bat->getRunAwayFlag() && target && TargetUnderneathMe(*bat, *target);

    // Choose our action:

    if (bat->isStunned()) {
        // Do nothing!
        allow_bite_halfway = true;
    } else if (bat->isMoving()) {
        // We must be halfway through a move, and checking whether we can bite someone...
        if (bite_allowed) bite = true;
        // Wait until the move expires before attempting another bite
        allow_bite_halfway = false;
    } else if (bat->getRunAwayFlag() && target) {
        // "Run away flag" means we must move in the direction that the target is facing
        // (or a standard "run away" direction if that is not possible)
        MapDirection dir = target->getFacing();
        if (BatCanEnter()(*bat->getMap(), DisplaceCoord(bat->getPos(), dir))) {
            p.first = dir;
            p.second = true;
        } else {
            p = ChooseDirection(bat, target->getPos(), true, BatCanEnter());
        }
        // Don't allow halfway-through bites when running away
        allow_bite_halfway = false;
    } else if (bite_allowed) {
        // Bite the target
        bite = true;
        allow_bite_halfway = false;  // since we're not moving anyway
    } else if (target) {
        // Move towards the target
        p = ChooseDirection(bat, target->getPos(), false, BatCanEnter());
        // We're allowed to bite him halfway through the move...
        allow_bite_halfway = true;
    } else if (g_rng.getBool(monster_wait_chance_as_fraction)) {
        // Special rule - if there is no target then we have a "monster_wait_chance" 
        // chance of doing nothing (as for zombies).
        allow_bite_halfway = false; // since we're not moving anyway
    } else {
        // Move randomly
        p = ChooseDirection(bat, MapCoord(), false, BatCanEnter());
        // No target so don't bother allowing bite halfway
        allow_bite_halfway = false;
    }


    // Execute the selected action
    if (!bat->isStunned()) {
        if (p.second) {
            bat->setFacing(p.first);
            bat->move(MT_MOVE);
            bat->clearRunAwayFlag();
        } else if (bite) {
            ASSERT(bite_allowed);
            ASSERT(target);  // implied by bite_allowed
            next_bite_time = tm.getGVT() + bat_bite_wait;
            bat->bite(target);
        }
    }

    // Now replace the task
    ReplaceTask(tm, shared_from_this(), *bat, allow_bite_halfway);
}

shared_ptr<Monster> VampireBatMonsterType::makeMonster(MonsterManager &mm, TaskManager &tm)
    const
{
    const int h = health ? health->get() : 1; 
    shared_ptr<VampireBat> vbat(new VampireBat(mm, *this, h, anim, speed));
    shared_ptr<Task> ai(new VampireBatAI(vbat));
    tm.addTask(ai, TP_LOW, tm.getGVT()+1);
    return vbat;
}


void VampireBat::damage(int amount, Player *attacker, int su, bool inhibit_squelch)
{
    // run vampire bat hook (usually plays "screech" sound effect)
    // -- Only want this if the bat was not killed.
    shared_ptr<VampireBat> self(static_pointer_cast<VampireBat>(shared_from_this()));
    if (amount < getHealth()) {
        Mediator::instance().runHook("HOOK_BAT", self);
    } else if (!inhibit_squelch) {
        Mediator::instance().runHook("HOOK_CREATURE_SQUELCH", self);
    }
    
    // Vampire bats will run away after they get hit.
    // Also: Vampire bats are immune to being "stunned" by weapon impacts.
    Creature::damage(amount, attacker, -1, inhibit_squelch);
    run_away_flag = true;
}

void VampireBat::bite(shared_ptr<Creature> target)
{
    if (isStunned()) return;
    Mediator &mediator = Mediator::instance();
    const int gvt = mediator.getGVT();

    // strike the target
    if (target) {
        const int stun_until = (mtype.stun? mtype.stun->get() : 0) + gvt;
        target->damage(mtype.dmg, 0, stun_until);
    }

    // set my anim frame, and stun myself.
    const int wait_until = gvt + mediator.cfgInt("melee_delay_time");
    setAnimFrame(AF_IMPACT, wait_until);
    stunUntil(wait_until);
}


//
// zombies
//

class ZombieAI : public Task {
public:
    ZombieAI(const MonsterManager &mm, weak_ptr<Monster> z) : mmgr(mm), zombie(z) { }
    virtual void execute(TaskManager &tm);
private:
    const MonsterManager &mmgr;
    weak_ptr<Monster> zombie;
};

namespace {
    // some predicates:
    
    struct IsVisible {
        bool operator()(shared_ptr<Knight> kt) const {
            return kt->isVisible();
        }
    };

    struct VisibleAndCarrying {
        explicit VisibleAndCarrying(const ItemType *it_) : it(it_) { }
        bool operator()(shared_ptr<Knight> kt) const {
            return kt->getItemInHand() == it && kt->isVisible();
        }
        const ItemType *it;
    };

    struct ZombieCanWalk {
        explicit ZombieCanWalk(const MonsterManager &m) : mm(m) { }
        const MonsterManager &mm;
        bool operator()(DungeonMap &dmap, const MapCoord &mc) const {
            // If the access is not A_CLEAR then we may not walk into this square
            if (dmap.getAccess(mc, H_WALKING) != A_CLEAR) return false;
            
            // If a tile is on the "avoid" list then we may not walk into this square
            vector<shared_ptr<Tile> > tiles;
            dmap.getTiles(mc, tiles);
            for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end();
            ++it) {
                if (find(mm.getZombieAvoid().begin(), mm.getZombieAvoid().end(), *it)
                != mm.getZombieAvoid().end()) {
                    return false;
                }
            }

            return true;
        }
    };

    struct ZombieCanFight {
        explicit ZombieCanFight(const MonsterManager &m) : mm(m) { }
        const MonsterManager &mm;
        bool operator()(DungeonMap &dmap, const MapCoord &mc) const {
            
            // A zombie can always attack a knight (assuming we're not afraid of him)
            // Note: this means a zombie will be able to attack an invisible knight if it
            // is next to such a knight. But zombies will not *target* invisible knights
            // from a distance.
            if (KnightAt(dmap, mc, mm.getZombieFear())) return true;

            // A zombie can fight a "bear trap" tile
            if (dmap.getItem(mc) && &dmap.getItem(mc)->getType() == mm.getZombieHit()) {
                return true;
            }
            
            // A zombie can also try to smash furniture tiles (as long as they're
            // not doors).
            vector<shared_ptr<Tile> > tiles;
            dmap.getTiles(mc, tiles);
            for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end();
            ++it) {
                if ((*it)->destructible() && !dynamic_cast<Door*>(it->get())) {
                    return true;
                }
            }

            return false;
        }
    };

    struct ZombieCanMove {
        explicit ZombieCanMove(const MonsterManager &m) : mm(m) { }
        bool operator()(DungeonMap &dmap, const MapCoord &mc) const {
            const ZombieCanWalk zcw(mm);
            const ZombieCanFight zcf(mm);
            return zcw(dmap,mc) || zcf(dmap,mc);
        }
        const MonsterManager &mm;
    };
}

void ZombieAI::execute(TaskManager &tm)
{
    shared_ptr<Monster> zom = zombie.lock();
    if (!zom || !zom->getMap()) return;  // our zombie appears to have died.

    Mediator &mediator = Mediator::instance();

    pair<MapDirection,bool> p;
    p.second = false;
    
    // Find a target (or something to run away from!)
    shared_ptr<Knight> target = FindClosestKnight(zom,
            VisibleAndCarrying(mmgr.getZombieFear()));
    bool run_away;
    if (target) {
        run_away = true;
    } else {
        target = FindClosestKnight(zom, IsVisible());
        run_away = false;
    }
    
    // Choose a direction to move in:
    // (If there is no target, then there is a chance that the monster will stay
    // where it is and do nothing, rather than randomly walking about.)
    if (!(!target && g_rng.getBool(mediator.cfgInt("monster_wait_chance")/100.0f))) {
        p = ChooseDirection(zom, target? target->getPos() : MapCoord(), run_away,
                            ZombieCanMove(mmgr));
    }

    // Move (if we can act)
    if (!zom->isStunned() && !zom->isMoving()) {

        if (p.second) {
            // A direction was chosen above. Turn to face this direction
            zom->setFacing(p.first);
            
            // Either fight or walk, depending on what's in the tile ahead.
            MapCoord sq_ahead = DisplaceCoord(zom->getPos(), zom->getFacing());
            const ZombieCanFight zcf(mmgr);
            const ZombieCanWalk zcw(mmgr);
            if (zcf(*zom->getMap(), sq_ahead)) {
                zom->swing();
            } else if (zcw(*zom->getMap(), sq_ahead)) {
                zom->move(MT_MOVE);
                // Play a moo sound 1 in every 20 zombie moves
                if (g_rng.getBool(0.05f)) {
                    mediator.runHook("HOOK_ZOMBIE", zom);
                }
            }
        } else {
            // We have chosen to stay where we are. But we should at least
            // turn to face the player (this looks a bit better).
            if (target) {
                zom->setFacing(DirectionFromTo(zom->getPos(), target->getPos()));
            } else {
                zom->setFacing(MapDirection(g_rng.getInt(0,4)));
            }
        }
    }

    // Now wait for an appropriate time before making the next move. 
    ReplaceTask(tm, shared_from_this(), *zom, false);
}

shared_ptr<Monster> ZombieMonsterType::makeMonster(MonsterManager &mm, TaskManager &tm) const
{
    const int h = health ? health->get() : 1;
    shared_ptr<Monster> zombie(new Zombie(mm, *this, h, weapon, anim, speed));
    zombie->setFacing(MapDirection(g_rng.getInt(0,4))); // random initial facing
    shared_ptr<Task> ai(new ZombieAI(mm, zombie));
    tm.addTask(ai, TP_LOW, tm.getGVT()+1);
    return zombie;
}

//
// This routine makes zombies 'recoil' when they're hit.
// Also it runs the hooks (usually sound effects).

void Zombie::damage(int amount, Player *attacker, int stun_until, bool inhibit_squelch)
{
    Mediator &mediator = Mediator::instance();
    shared_ptr<Zombie> self(static_pointer_cast<Zombie>(shared_from_this()));
    mediator.runHook("HOOK_ZOMBIE", self);
    if (amount >= getHealth() && !inhibit_squelch) {
        mediator.runHook("HOOK_CREATURE_SQUELCH", self);
    }
    Creature::damage(amount, attacker, stun_until, inhibit_squelch);
    setAnimFrame(AF_PARRY,
                 stun_until != -1 ? stun_until
                                  : (mediator.getGVT() + mediator.cfgInt("zombie_damage_delay")));
}

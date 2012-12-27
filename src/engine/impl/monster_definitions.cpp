/*
 * monster_definitions.cpp
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
#include "dungeon_map.hpp"
#include "item.hpp"
#include "lua_setup.hpp"
#include "mediator.hpp"
#include "monster_definitions.hpp"
#include "monster_support.hpp"
#include "random_int.hpp"
#include "rng.hpp"
#include "special_tiles.hpp"
#include "task.hpp"
#include "task_manager.hpp"
#include "tile.hpp"



//
// flying monsters
//

class FlyingMonsterAI : public Task {
public:
    explicit FlyingMonsterAI(weak_ptr<FlyingMonster> v) : vbat(v), next_bite_time(0) { }
    virtual void execute(TaskManager &tm);
private:
    // note: vbat can be any flying monster (not necessarily a vampire bat) but at the
    // current time, vampire bats are the only defined flying monster.
    weak_ptr<FlyingMonster> vbat;
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
        const int flying_monster_targetting_offset = mediator.cfgInt("flying_monster_targetting_offset");

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

        return dist < flying_monster_targetting_offset;
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

void FlyingMonsterAI::execute(TaskManager &tm)
{
    Mediator &mediator = Mediator::instance();
    const float monster_wait_chance_as_fraction = mediator.cfgProbability("monster_wait_chance");
    const int flying_monster_bite_wait = mediator.cfgInt("flying_monster_bite_wait");

    shared_ptr<FlyingMonster> bat = vbat.lock();
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
            bat->runMovementAction();
            bat->clearRunAwayFlag();
        } else if (bite) {
            ASSERT(bite_allowed);
            ASSERT(target);  // implied by bite_allowed
            next_bite_time = tm.getGVT() + flying_monster_bite_wait;
            bat->bite(target);
        }
    }

    // Now replace the task
    ReplaceTask(tm, shared_from_this(), *bat, allow_bite_halfway);
}

FlyingMonsterType::FlyingMonsterType(lua_State *lua)
    : MonsterType(lua)
{
    health = LuaGetRandomInt(lua, -1, "health");
    speed = LuaGetInt(lua, -1, "speed");
    anim = LuaGetPtr<Anim>(lua, -1, "anim");

    dmg = LuaGetInt(lua, -1, "attack_damage");
    stun = LuaGetRandomInt(lua, -1, "attack_stun_time");
}

void FlyingMonsterType::newIndex(lua_State *lua)
{
    if (!lua_isstring(lua, 2)) return;
    const std::string k = lua_tostring(lua, 2);

    if (k == "health") {
        lua_pushvalue(lua, 3);
        health = LuaPopRandomInt(lua, "health");

    } else if (k == "speed") {
        speed = lua_tointeger(lua, 3);

    } else if (k == "anim") {
        anim = ReadLuaPtr<Anim>(lua, 3);

    } else if (k == "attack_damage") {
        dmg = lua_tointeger(lua ,3);

    } else if (k == "attack_stun_time") {
        lua_pushvalue(lua, 3);
        stun = LuaPopRandomInt(lua, "attack_stun_time");

    } else {
        MonsterType::newIndex(lua);
    }
}    

shared_ptr<Monster> FlyingMonsterType::makeMonster(TaskManager &tm) const
{
    const int h = std::max(1, health.get());
    shared_ptr<FlyingMonster> mon(new FlyingMonster(*this, h, anim, speed));
    shared_ptr<Task> ai(new FlyingMonsterAI(mon));
    tm.addTask(ai, TP_LOW, tm.getGVT()+1);
    return mon;
}


void FlyingMonster::damage(int amount, const Originator &attacker, int su, bool inhibit_squelch)
{
    // call HOOK_CREATURE_SQUELCH if required
    shared_ptr<FlyingMonster> self(static_pointer_cast<FlyingMonster>(shared_from_this()));
    if (amount >= getHealth() && !inhibit_squelch) {
        Mediator::instance().runHook("HOOK_CREATURE_SQUELCH", self);
    }
    
    // Flying monsters will run away after they get hit.
    // Also: Flying monsters are immune to being "stunned" by weapon impacts.
    Monster::damage(amount, attacker, -1, inhibit_squelch);
    run_away_flag = true;
}

void FlyingMonster::bite(shared_ptr<Creature> target)
{
    if (isStunned()) return;

    Mediator &mediator = Mediator::instance();
    const int gvt = mediator.getGVT();

    // call Lua on_attack method
    runAction(getMonsterType().getOnAttack());
    
    // strike the target
    if (target) {
        const int stun_until = mtype.stun.get() + gvt;
        target->damage(mtype.dmg, Originator(OT_Monster()), stun_until);
    }

    // set my anim frame, and stun myself.
    const int wait_until = gvt + mediator.cfgInt("melee_delay_time");
    setAnimFrame(AF_IMPACT, wait_until);
    stunUntil(wait_until);
}


//
// walking monsters
//

class WalkingMonsterAI : public Task {
public:
    WalkingMonsterAI(weak_ptr<WalkingMonster> m,
                     const std::vector<shared_ptr<Tile> > &avoid_tiles_,
                     const std::vector<ItemType *> &fear_items_,
                     const std::vector<ItemType *> &hit_items_)
        : monster(m),
          avoid_tiles(avoid_tiles_),
          fear_items(fear_items_),
          hit_items(hit_items_)
    { }

    virtual void execute(TaskManager &tm);

private:
    weak_ptr<WalkingMonster> monster;
    
    const std::vector<shared_ptr<Tile> > & avoid_tiles;
    const std::vector<ItemType*> & fear_items;
    const std::vector<ItemType*> & hit_items;
};

namespace {
    // some predicates:
    
    struct IsVisible {
        bool operator()(shared_ptr<Knight> kt) const {
            return kt->isVisible();
        }
    };

    struct VisibleAndCarrying {
        explicit VisibleAndCarrying(const std::vector<ItemType *> &it_) : items(it_) { }
        bool operator()(shared_ptr<Knight> kt) const {
            return kt->isVisible() &&
                std::find(items.begin(), items.end(), kt->getItemInHand()) != items.end();
        }
        const std::vector<ItemType *> &items;
    };

    struct ZombieCanWalk {
        explicit ZombieCanWalk(const std::vector<shared_ptr<Tile> > &avoid_tiles_) : avoid_tiles(avoid_tiles_) { }

        const std::vector<shared_ptr<Tile> > & avoid_tiles;
        
        bool operator()(const DungeonMap &dmap, const MapCoord &mc) const {
            // If the access is not A_CLEAR then we may not walk into this square
            if (dmap.getAccess(mc, H_WALKING) != A_CLEAR) return false;
            
            // If a tile is on the "avoid" list then we may not walk into this square
            vector<shared_ptr<Tile> > tiles;
            dmap.getTiles(mc, tiles);
            for (vector<shared_ptr<Tile> >::iterator it = tiles.begin(); it != tiles.end();
            ++it) {
                if (find(avoid_tiles.begin(), avoid_tiles.end(), (*it)->getOriginalTile()) != avoid_tiles.end()) {
                    return false;
                }
            }

            return true;
        }
    };

    struct ZombieCanFight {
        explicit ZombieCanFight(const std::vector<ItemType *> &fear_items_, 
                                const std::vector<ItemType *> &hit_items_)
            : fear_items(fear_items_), hit_items(hit_items_) { }

        const std::vector<ItemType *> &fear_items;
        const std::vector<ItemType *> &hit_items;

        bool operator()(DungeonMap &dmap, const MapCoord &mc) const {
            
            // A walking monster can always attack a knight (assuming the knight is not
            // carrying the feared item).
            // Note: this means a zombie will be able to attack an invisible knight if it
            // is next to such a knight. But zombies will not *target* invisible knights
            // from a distance.
            if (KnightAt(dmap, mc, fear_items)) return true;

            // A walking monster can fight a "bear trap" tile
            if (dmap.getItem(mc) && 
            std::find(hit_items.begin(), hit_items.end(), &dmap.getItem(mc)->getType()) != hit_items.end()) {
                return true;
            }
            
            // A walking monster can also try to smash furniture tiles (as long as they're
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
        explicit ZombieCanMove(const std::vector<shared_ptr<Tile> > &avoid_tiles,
                               const std::vector<ItemType *> &fear_items,
                               const std::vector<ItemType *> &hit_items)
            : zcw(avoid_tiles),
              zcf(fear_items, hit_items)
        { }
        
        const ZombieCanWalk zcw;
        const ZombieCanFight zcf;
        
        bool operator()(DungeonMap &dmap, const MapCoord &mc) const
        {
            return zcw(dmap,mc) || zcf(dmap,mc);
        }
    };
}

void WalkingMonsterAI::execute(TaskManager &tm)
{
    shared_ptr<WalkingMonster> mon = monster.lock();
    if (!mon || !mon->getMap()) return;  // our monster appears to have died.

    Mediator &mediator = Mediator::instance();

    pair<MapDirection,bool> p;
    p.second = false;
    
    // Find a target (or something to run away from!)
    shared_ptr<Knight> target = FindClosestKnight(mon, VisibleAndCarrying(fear_items));
    bool run_away;
    if (target) {
        run_away = true;
    } else {
        target = FindClosestKnight(mon, IsVisible());
        run_away = false;
    }
    
    // Choose a direction to move in:
    // (If there is no target, then there is a chance that the monster will stay
    // where it is and do nothing, rather than randomly walking about.)
    if (!(!target && g_rng.getBool(mediator.cfgProbability("monster_wait_chance")))) {
        p = ChooseDirection(mon, target? target->getPos() : MapCoord(), run_away,
                            ZombieCanMove(avoid_tiles, fear_items, hit_items));
    }

    // Move (if we can act)
    if (!mon->isStunned() && !mon->isMoving()) {

        if (p.second) {
            // A direction was chosen above. Turn to face this direction
            mon->setFacing(p.first);
            
            // Either fight or walk, depending on what's in the tile ahead.
            MapCoord sq_ahead = DisplaceCoord(mon->getPos(), mon->getFacing());
            const ZombieCanWalk zcw(avoid_tiles);
            const ZombieCanFight zcf(fear_items, hit_items);
            if (zcf(*mon->getMap(), sq_ahead)) {
                mon->swing();
            } else if (zcw(*mon->getMap(), sq_ahead)) {
                mon->move(MT_MOVE);
                mon->runMovementAction();
            }
        } else {
            // We have chosen to stay where we are. But we should at least
            // turn to face the player (this looks a bit better).
            if (target) {
                mon->setFacing(DirectionFromTo(mon->getPos(), target->getPos()));
            } else {
                mon->setFacing(MapDirection(g_rng.getInt(0,4)));
            }
        }
    }

    // Now wait for an appropriate time before making the next move. 
    ReplaceTask(tm, shared_from_this(), *mon, false);
}

WalkingMonsterType::WalkingMonsterType(lua_State *lua)
    : MonsterType(lua)
{
    // [... t]
    
    health = LuaGetRandomInt(lua, -1, "health");
    speed = LuaGetInt(lua, -1, "speed");
    anim = LuaGetPtr<Anim>(lua, -1, "anim");

    weapon = LuaGetPtr<ItemType>(lua, -1, "weapon");

    LuaGetItemList(lua, -1, "ai_fear", fear_items);
    LuaGetItemList(lua, -1, "ai_hit", hit_items);

    LuaGetTileList(lua, -1, "ai_avoid", avoid_tiles);
}

void WalkingMonsterType::newIndex(lua_State *lua)
{
    if (!lua_isstring(lua, 2)) return;
    const std::string k = lua_tostring(lua, 2);

    if (k == "health") {
        lua_pushvalue(lua, 3);
        health = LuaPopRandomInt(lua, "health");

    } else if (k == "speed") {
        speed = lua_tointeger(lua, 3);

    } else if (k == "anim") {
        anim = ReadLuaPtr<Anim>(lua, 3);

    } else if (k == "weapon") {
        weapon = ReadLuaPtr<ItemType>(lua, 3);

    } else if (k == "ai_fear") {
        lua_pushvalue(lua, 3);
        LuaPopItemList(lua, fear_items);

    } else if (k == "ai_hit") {
        lua_pushvalue(lua, 3);
        LuaPopItemList(lua, hit_items);

    } else if (k == "ai_avoid") {
        lua_pushvalue(lua, 3);
        LuaPopTileList(lua, avoid_tiles);
    }
}

shared_ptr<Monster> WalkingMonsterType::makeMonster(TaskManager &tm) const
{
    const int h = std::max(1, health.get());
    shared_ptr<WalkingMonster> monster(new WalkingMonster(*this, h, weapon, anim, speed));
    monster->setFacing(MapDirection(g_rng.getInt(0,4))); // random initial facing
    shared_ptr<Task> ai(new WalkingMonsterAI(monster, avoid_tiles, fear_items, hit_items));
    tm.addTask(ai, TP_LOW, tm.getGVT()+1);
    return monster;
}

bool WalkingMonsterType::okToCreateAt(const DungeonMap &dmap, const MapCoord &pos) const
{
    const ZombieCanWalk zcw(avoid_tiles);
    return zcw(dmap, pos);
}

//
// This routine makes walking monsters 'recoil' when they're hit.
// Also it runs the hook / sound action.

void WalkingMonster::damage(int amount, const Originator &attacker, int stun_until, bool inhibit_squelch)
{
    Mediator &mediator = Mediator::instance();
    shared_ptr<WalkingMonster> self(static_pointer_cast<WalkingMonster>(shared_from_this()));
    if (amount >= getHealth() && !inhibit_squelch) {
        mediator.runHook("HOOK_CREATURE_SQUELCH", self);
    }
    Monster::damage(amount, attacker, stun_until, inhibit_squelch);
    setAnimFrame(AF_PARRY,
                 stun_until != -1 ? stun_until
                                  : (mediator.getGVT() + mediator.cfgInt("walking_monster_damage_delay")));
}

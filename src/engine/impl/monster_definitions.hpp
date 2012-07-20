/*
 * monster_definitions.hpp
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
 * Defines two types of monster: WalkingMonster and FlyingMonster.
 *
 * The configuration of these into particular monster types (zombies, ogres,
 * vampire bats) is done entirely in the knights_data files; the engine
 * is (mostly) unaware of those particular monster types.
 *
 * Note: it is a design principle that MonsterTypes and Monsters
 * should not depend directly on MonsterManager.
 * 
 */

#ifndef MONSTER_DEFINITIONS_HPP
#define MONSTER_DEFINITIONS_HPP

#include "monster.hpp"
#include "monster_type.hpp"

#include "kconfig_fwd.hpp"
using namespace KConfig;

#include "boost/shared_ptr.hpp"

#include <vector>

class Anim;
class ItemType;
class Tile;

//
// Flying Monsters (vampire bats)
//

class FlyingMonster;

class FlyingMonsterType : public MonsterType {
    friend class FlyingMonster;
public:
    // reads table from top of stack (does not pop stack)
    FlyingMonsterType(lua_State *lua, KnightsConfigImpl *kc);

    virtual shared_ptr<Monster> makeMonster(TaskManager &tm) const;
    virtual MapHeight getHeight() const { return H_FLYING; }

private:
    const RandomInt *health;
    int speed;
    const Anim *anim;
    int dmg;
    const RandomInt *stun;
};

//
// We have a special class FlyingMonster (instead of just using Monster) because
// (among other reasons) flying monsters have their own special attack method.
//

class FlyingMonster : public Monster {
public:
    FlyingMonster(const FlyingMonsterType &type, int health, const Anim *anim,
                  int speed)
        : Monster(type, health, H_FLYING, 0, anim, speed),
          mtype(type),
          run_away_flag(false)
    { }

    virtual int bloodLevel() const { return 0; }
    virtual void damage(int amount, const Originator &originator, int stun_until, bool inhibit_squelch);
    
    bool getRunAwayFlag() const { return run_away_flag; } // used by AI
    void clearRunAwayFlag() { run_away_flag = false; }
    
    void bite(shared_ptr<Creature> cr);   // note: does NOT check that the creature is in range !!

private:
    const FlyingMonsterType &mtype;
    bool run_away_flag;
};



//
// Walking Monsters (zombies, ogres)
//

class WalkingMonsterType : public MonsterType {
public:
    WalkingMonsterType(lua_State *lua, KnightsConfigImpl *kc);  // reads table from top of lua stack; doesn't pop
    virtual shared_ptr<Monster> makeMonster(TaskManager &tm) const;
    virtual MapHeight getHeight() const { return H_WALKING; }
    
private:
    const RandomInt *health;
    int speed;
    const ItemType *weapon;
    const Anim *anim;

    std::vector<shared_ptr<Tile> > avoid_tiles;
    const ItemType * fear_item;
    const ItemType * hit_item;
};

//
// A new Monster subclass is created for WalkingMonsters, because we want to set blood level to 0,
// and also switch the animation frame when the monster gets hit.
//

class WalkingMonster : public Monster {
public:
    WalkingMonster(const MonsterType &type, int health, const ItemType *weapon,
                   const Anim *anim, int speed)
        : Monster(type, health, H_WALKING, weapon, anim, speed) { }
    virtual int bloodLevel() const { return 0; }
    virtual void damage(int amount, const Originator &originator, int stun_until, bool inhibit_squelch);

    virtual void onDownswing();
};

#endif

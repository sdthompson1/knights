/*
 * monster_definitions.hpp
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

/*
 * Defines Vampire Bats and Zombies.
 *
 */

#ifndef MONSTER_DEFINITIONS_HPP
#define MONSTER_DEFINITIONS_HPP

#include "monster.hpp"
#include "monster_type.hpp"

#include "kconfig_fwd.hpp"
using namespace KConfig;

class Anim;
class ItemType;

//
// Vampire Bats
//

class VampireBat;

class VampireBatMonsterType : public MonsterType {
    friend class VampireBat;
public:
    VampireBatMonsterType(const RandomInt *health_, int speed_, const Anim *anim_,
                          int dmg_, const RandomInt *stun_)
        : health(health_), speed(speed_), anim(anim_), dmg(dmg_), stun(stun_) { }
    virtual shared_ptr<Monster> makeMonster(MonsterManager &mm, TaskManager &tm) const;
    virtual MapHeight getHeight() const { return H_FLYING; }

private:
    const RandomInt *health;
    int speed;
    const Anim *anim;
    int dmg;
    const RandomInt *stun;
};

//
// We have a special class VampireBat (instead of just using Monster) because
// (among other reasons) vampire bats have their own special attack method.
//

class VampireBat : public Monster {
public:
    VampireBat(MonsterManager &mmgr,
               const VampireBatMonsterType &type, int health, const Anim *bat_anim,
               int speed)
        : Monster(mmgr, type, health, H_FLYING, 0, bat_anim, speed), mtype(type),
          run_away_flag(false) { }

    virtual int bloodLevel() const { return 0; }
    virtual void damage(int amount, Player *attacker, int stun_until, bool inhibit_squelch);
    
    bool getRunAwayFlag() const { return run_away_flag; } // used by AI
    void clearRunAwayFlag() { run_away_flag = false; }
    
    void bite(shared_ptr<Creature> cr);   // note: does NOT check that the creature is in range !!

private:
    const VampireBatMonsterType &mtype;
    bool run_away_flag;
};



//
// Zombies
//

class ZombieMonsterType : public MonsterType {
public:
    ZombieMonsterType(const RandomInt *health_, int speed_, const ItemType *weapon_,
                      const Anim *anim_)
        : health(health_), speed(speed_), weapon(weapon_), anim(anim_) { }
    virtual shared_ptr<Monster> makeMonster(MonsterManager &mm, TaskManager &tm) const;
    virtual MapHeight getHeight() const { return H_WALKING; }
private:
    const RandomInt *health;
    int speed;
    const ItemType *weapon;
    const Anim *anim;
};

//
// A new Monster subclass is created for Zombies, because we want to set blood level to 0,
// and also switch the animation frame when the zombie gets hit.
//

class Zombie : public Monster {
public:
    Zombie(MonsterManager &mmgr,
           const MonsterType &type, int health, const ItemType *weapon,
           const Anim *zombie_anim, int speed)
        : Monster(mmgr, type, health, H_WALKING, weapon, zombie_anim, speed) { }
    virtual int bloodLevel() const { return 0; }
    virtual void damage(int amount, Player *attacker, int stun_until, bool inhibit_squelch);
    virtual const char * getWeaponDownswingHook() const { return "HOOK_ZOMBIE"; } // zombies moo on downswing.
};


#endif

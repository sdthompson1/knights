/*
 * magic_actions.cpp
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
#include "dispel_magic.hpp"
#include "dungeon_view.hpp"
#include "knight.hpp"
#include "lockable.hpp"
#include "magic_actions.hpp"
#include "magic_map.hpp"
#include "mediator.hpp"
#include "monster_manager.hpp"
#include "player.hpp"
#include "potion_magic.hpp"
#include "task_manager.hpp"
#include "teleport.hpp"

#ifdef _MSC_VER
    // fix "bug" with MSVC static libraries and global constructors
    extern "C" void InitMagicActions()
    {
    }
#endif


//
// SetPotion, FlashMessage
//

namespace {
    void SetPotion(int gvt, shared_ptr<Knight> kt, PotionMagic pm, int dur)
    {
        if (!kt) return;
        int stop_time = gvt;
        if (dur > 0) stop_time += dur;
        kt->setPotionMagic(pm, stop_time);
    }

    void FlashMessage(shared_ptr<Knight> kt, const string &msg)
    {
        if (!kt) return;
        kt->getPlayer()->getDungeonView().flashMessage(msg, 4);
    }

    void FlashMessage(const ActionData &ad, const string &msg)
    {
        FlashMessage(dynamic_pointer_cast<Knight>(ad.getActor()), msg);
    }
}


//
// A_Attractor
//

void A_Attractor::execute(const ActionData &ad) const
{
    shared_ptr<Creature> cr(ad.getActor());
    if (cr && cr->getMap()) {
        shared_ptr<Knight> who_to_teleport = FindRandomOtherKnight(boost::dynamic_pointer_cast<Knight>(cr));
        if (who_to_teleport) {
            TeleportToRoom(who_to_teleport, cr);
        } else {
            // If no other knights exist then teleport myself randomly instead.
            TeleportToRandomSquare(cr);
        }
    }
}

A_Attractor::Maker A_Attractor::Maker::register_me;

LegacyAction * A_Attractor::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_Attractor;
}


//
// A_DispelMagic
//

void A_DispelMagic::execute(const ActionData &ad) const
{
    FlashMessage(ad, msg);
    DispelMagic(Mediator::instance().getPlayers());
}

A_DispelMagic::Maker A_DispelMagic::Maker::register_me;

LegacyAction * A_DispelMagic::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    return new A_DispelMagic(pars.getString(0));
}


//
// A_Healing
//

void A_Healing::execute(const ActionData &ad) const
{
    if (ad.getActor() && ad.getActor()->getHealth() < ad.getActor()->getMaxHealth()) {
        // reset his health to maximum
        FlashMessage(ad, msg);
        ad.getActor()->addToHealth( ad.getActor()->getMaxHealth() - ad.getActor()->getHealth() );
    }
}

A_Healing::Maker A_Healing::Maker::register_me;

LegacyAction * A_Healing::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    return new A_Healing(pars.getString(0));
}


//
// A_Invisibility
//

void A_Invisibility::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    FlashMessage(kt, msg);
    SetPotion(Mediator::instance().getGVT(), kt, INVISIBILITY, dur);
}

A_Invisibility::Maker A_Invisibility::Maker::register_me;

LegacyAction * A_Invisibility::Maker::make(ActionPars &pars) const
{
    pars.require(2);
    return new A_Invisibility(pars.getInt(0), pars.getString(1));
}

//
// A_Invulnerability
//

void A_Invulnerability::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    if (kt) {
        FlashMessage(kt, msg);
        int stop_time = Mediator::instance().getGVT();
        if (dur > 0) stop_time += dur;
        kt->setInvulnerability(true, stop_time);
    }
}

A_Invulnerability::Maker A_Invulnerability::Maker::register_me;

LegacyAction * A_Invulnerability::Maker::make(ActionPars &pars) const
{
    pars.require(2);
    return new A_Invulnerability(pars.getInt(0), pars.getString(1));
}


//
// A_MagicMapping
//

void A_MagicMapping::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    if (kt) MagicMapping(kt);
}

A_MagicMapping::Maker A_MagicMapping::Maker::register_me;

LegacyAction * A_MagicMapping::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_MagicMapping;
}


//
// A_OpenWays
//

void A_OpenWays::execute(const ActionData &ad) const
{
    DungeonMap *dm;
    MapCoord mc;
    shared_ptr<Tile> t;
    ad.getTile(dm, mc, t);
    if (!dm || !t) return;
    shared_ptr<Creature> actor = ad.getActor();
    
    // If the tile is a "lockable" (i.e. a door or chest) then open it.
    shared_ptr<Lockable> lock = dynamic_pointer_cast<Lockable>(t);
    if (lock) {
        lock->open(*dm, mc, ad.getOriginator());
    } else if (actor->hasStrength()) {
        // We turn off allow_strength for wand of open ways, so the normal melee code
        // will not attempt to damage the tile if the knight has strength. This is to
        // prevent the melee code from destroying the tile before the above code has a
        // chance to open it.
        // However for other types of tile we DO want the normal smashing to take
        // place. So we do it here:
        t->damage(*dm, mc, 9999, actor);
    }
}

A_OpenWays::Maker A_OpenWays::Maker::register_me;

LegacyAction * A_OpenWays::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_OpenWays;
}


//
// A_Paralyzation
//

void A_Paralyzation::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    SetPotion(Mediator::instance().getGVT(), kt, PARALYZATION, dur);
}

A_Paralyzation::Maker A_Paralyzation::Maker::register_me;

LegacyAction * A_Paralyzation::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    return new A_Paralyzation(pars.getInt(0));
}


//
// A_Poison
//

void A_Poison::execute(const ActionData &ad) const
{
    if (ad.getActor()) {
        FlashMessage(ad, msg);
        ad.getActor()->poison(ad.getOriginator());
    }
}

A_Poison::Maker A_Poison::Maker::register_me;

LegacyAction * A_Poison::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    return new A_Poison(pars.getString(0));
}


//
// A_PoisonImmunity
//

void A_PoisonImmunity::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    if (kt) {
        FlashMessage(kt, msg);
        const int stop_time = Mediator::instance().getGVT() + (dur > 0 ? dur : 0);
        kt->setPoisonImmunity(true, stop_time);
    }
}

A_PoisonImmunity::Maker A_PoisonImmunity::Maker::register_me;

LegacyAction * A_PoisonImmunity::Maker::make(ActionPars &pars) const
{
    pars.require(2);
    return new A_PoisonImmunity(pars.getInt(0), pars.getString(1));
}


//
// A_Quickness
//

void A_Quickness::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    FlashMessage(kt, msg);
    SetPotion(Mediator::instance().getGVT(), kt, QUICKNESS, dur);
}

A_Quickness::Maker A_Quickness::Maker::register_me;

LegacyAction * A_Quickness::Maker::make(ActionPars &pars) const
{
    pars.require(2);
    return new A_Quickness(pars.getInt(0), pars.getString(1));
}


//
// A_Regeneration
//

void A_Regeneration::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    FlashMessage(kt, msg);
    SetPotion(Mediator::instance().getGVT(), kt, REGENERATION, dur);
}

A_Regeneration::Maker A_Regeneration::Maker::register_me;

LegacyAction * A_Regeneration::Maker::make(ActionPars &pars) const
{
    pars.require(2);
    return new A_Regeneration(pars.getInt(0), pars.getString(1));
}


//
// A_RevealLocation
//

void A_RevealLocation::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    if (kt) {
        const int stop_time = Mediator::instance().getGVT() + (dur > 0 ? dur : 0);
        kt->setRevealLocation(true, stop_time);
    }
}

A_RevealLocation::Maker A_RevealLocation::Maker::register_me;

LegacyAction * A_RevealLocation::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    return new A_RevealLocation(pars.getInt(0));
}


//
// A_SenseItems
//

void A_SenseItems::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    if (kt) {
        const int stop_time = Mediator::instance().getGVT() + (dur > 0 ? dur : 0);
        SenseItems(kt, stop_time);
    }
}

A_SenseItems::Maker A_SenseItems::Maker::register_me;

LegacyAction * A_SenseItems::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    return new A_SenseItems(pars.getInt(0));
}


//
// A_SenseKnight
//

void A_SenseKnight::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    if (kt) {
        const int stop_time = Mediator::instance().getGVT() + (dur > 0 ? dur : 0);
        kt->setSenseKnight(true, stop_time);
    }
}

A_SenseKnight::Maker A_SenseKnight::Maker::register_me;

LegacyAction * A_SenseKnight::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    return new A_SenseKnight(pars.getInt(0));
}


//
// A_Strength
//

void A_Strength::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    FlashMessage(kt, msg);
    SetPotion(Mediator::instance().getGVT(), kt, STRENGTH, dur);
}

A_Strength::Maker A_Strength::Maker::register_me;

LegacyAction * A_Strength::Maker::make(ActionPars &pars) const
{
    pars.require(2);
    return new A_Strength(pars.getInt(0), pars.getString(1));
}


//
// A_Super
//

void A_Super::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    FlashMessage(kt, msg);
    SetPotion(Mediator::instance().getGVT(), kt, SUPER, dur);
}

A_Super::Maker A_Super::Maker::register_me;

LegacyAction * A_Super::Maker::make(ActionPars &pars) const
{
    pars.require(2);
    return new A_Super(pars.getInt(0), pars.getString(1));
}

//
// A_TeleportRandom
//

void A_TeleportRandom::execute(const ActionData &ad) const
{
    // Only Knights can teleport, because we don't want zombies to be teleported about
    // by pentagrams....
    shared_ptr<Creature> cr(ad.getActor());
    shared_ptr<Knight> kt = boost::dynamic_pointer_cast<Knight>(cr);
    if (kt && kt->getMap()) {
        shared_ptr<Knight> where_to_teleport_to = FindRandomOtherKnight(kt);
        if (where_to_teleport_to) {
            TeleportToRoom(kt, where_to_teleport_to);
        } else {
            // If no other knights exist then just teleport myself randomly
            TeleportToRandomSquare(kt);
        }
    }
}

A_TeleportRandom::Maker A_TeleportRandom::Maker::register_me;

LegacyAction * A_TeleportRandom::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_TeleportRandom;
}


//
// A_WipeMap
//

void A_WipeMap::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    if (kt) WipeMap(kt);
}

A_WipeMap::Maker A_WipeMap::Maker::register_me;

LegacyAction * A_WipeMap::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_WipeMap;
}


//
// A_ZombifyActor, A_ZombifyTarget
//

namespace {
    bool ZombifyCreature(shared_ptr<Creature> cr, const MonsterType &zom_type, const Originator &originator)
    {
        // Only Knights can be zombified. This prevents zombification
        // of vampire bats or other weird things like that. Of course,
        // if new monster types are added (and they are to be
        // zombifiable) then this rule might need to change ... (maybe
        // only zombify things which are not already of type "zom_type"
        // and are at height H_WALKING?)
        shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(cr);
        if (kt && kt->getMap()) {
            DungeonMap *dmap = kt->getMap();
            MapCoord mc = kt->getPos();
            MapDirection facing = kt->getFacing();
            kt->onDeath(Creature::ZOMBIE_MODE, originator);       // this drops items, etc.
            kt->rmFromMap();
            MonsterManager &mm = Mediator::instance().getMonsterManager();
            mm.placeMonster(zom_type, *dmap, mc, facing);
            return true;
        } else {
            return false;
        }
    }
}

void A_ZombifyActor::execute(const ActionData &ad) const
{
    ZombifyCreature(ad.getActor(), zom_type, ad.getOriginator());
}

A_ZombifyActor::Maker A_ZombifyActor::Maker::register_me;

LegacyAction * A_ZombifyActor::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    const MonsterType * mtype = pars.getMonsterType(0);
    if (!mtype) {
        pars.error();
        return 0;
    } else {
        return new A_ZombifyActor(*mtype);
    }
}

bool A_ZombifyTarget::possible(const ActionData &ad) const
{
    // Only knights can be zombified
    return dynamic_pointer_cast<Knight>(ad.getVictim());
}

bool A_ZombifyTarget::executeWithResult(const ActionData &ad) const
{
    return ZombifyCreature(ad.getVictim(), zom_type, ad.getOriginator());
}

A_ZombifyTarget::Maker A_ZombifyTarget::Maker::register_me;

LegacyAction * A_ZombifyTarget::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    const MonsterType * mtype = pars.getMonsterType(0);
    if (!mtype) {
        pars.error();
        return 0;
    } else {
        return new A_ZombifyTarget(*mtype);
    }
}

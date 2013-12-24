/*
 * script_actions.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2013.
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
#include "creature.hpp"
#include "dungeon_map.hpp"
#include "dungeon_view.hpp"
#include "item.hpp"
#include "knight.hpp"
#include "knights_callbacks.hpp"
#include "mediator.hpp"
#include "missile.hpp"
#include "monster.hpp"
#include "monster_manager.hpp"  // for Necromancy, *ZombieActivity
#include "player.hpp"
#include "room_map.hpp"
#include "script_actions.hpp"
#include "status_display.hpp"
#include "task_manager.hpp"
#include "teleport.hpp"
#include "tile.hpp"

#include <iostream> // used for A_DebugPrint
using namespace std;

#ifdef _MSC_VER
    // fix "bug" with MSVC static libraries and global constructors
    extern "C" void InitScriptActions()
    {
    }
#endif


//
// A_ChangeItem
//

void A_ChangeItem::execute(const ActionData &ad) const
{
    DungeonMap *dmap;
    MapCoord mc;
    ItemType *old_item;
    ad.getItem(dmap, mc, old_item);
    if (!dmap || mc.isNull() || !old_item || !item_type) return;
    dmap->rmItem(mc);
    shared_ptr<Item> new_item(new Item(*item_type));
    dmap->addItem(mc, new_item);
}

A_ChangeItem::Maker A_ChangeItem::Maker::register_me;

LegacyAction * A_ChangeItem::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    return new A_ChangeItem(pars.getItemType(0));
}



//
// A_ChangeTile
//

void A_ChangeTile::execute(const ActionData &ad) const
{
    DungeonMap *dmap;
    MapCoord mc;
    shared_ptr<Tile> old_tile;
    ad.getTile(dmap, mc, old_tile);
    if (!dmap || !old_tile || !tile) return;
    dmap->rmTile(mc, old_tile, ad.getOriginator());
    dmap->addTile(mc, tile->clone(false), ad.getOriginator());
}

A_ChangeTile::Maker A_ChangeTile::Maker::register_me;

LegacyAction * A_ChangeTile::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    return new A_ChangeTile(pars.getTile(0));
}


//
// A_CrystalStart
//

void A_CrystalStart::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    if (kt) kt->setCrystalBall(true);
}

A_CrystalStart::Maker A_CrystalStart::Maker::register_me;

LegacyAction * A_CrystalStart::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_CrystalStart;
}

//
// A_CrystalStop
//

void A_CrystalStop::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    if (kt) kt->setCrystalBall(false);
}

A_CrystalStop::Maker A_CrystalStop::Maker::register_me;

LegacyAction * A_CrystalStop::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_CrystalStop;
}

//
// A_Damage
//

void A_Damage::execute(const ActionData &ad) const
{
    // This damages the *actor* (used eg for bear traps)
    shared_ptr<Creature> cr = ad.getActor();
    if (cr) {
        const int stun = stun_time > 0 ? stun_time : 0;
        cr->damage(amount, ad.getOriginator(), Mediator::instance().getGVT() + stun, inhibit_squelch);
    }
}

A_Damage::Maker A_Damage::Maker::register_me;

LegacyAction * A_Damage::Maker::make(ActionPars &pars) const
{
    pars.require(2, 3);
    bool inhibit_squelch = (pars.getSize() == 3 && pars.getInt(2));
    return new A_Damage(pars.getInt(0), pars.getInt(1), inhibit_squelch);
}



//
// A_DebugPrint
// print a message to cout
//

void A_DebugPrint::execute(const ActionData &ad) const
{
    cout << msg << endl;
}

A_DebugPrint::Maker A_DebugPrint::Maker::register_me;

LegacyAction * A_DebugPrint::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    return new A_DebugPrint(pars.getString(0));
}


//
// A_FlashMessage
//

void A_FlashMessage::execute(const ActionData &ad) const
{
    shared_ptr<Creature> actor = ad.getActor();
    if (actor->getPlayer()) {
        actor->getPlayer()->getDungeonView().flashMessage(msg, num_times);
    }
}

A_FlashMessage::Maker A_FlashMessage::Maker::register_me;

LegacyAction * A_FlashMessage::Maker::make(ActionPars &pars) const
{
    pars.require(1,2);
    return new A_FlashMessage(pars.getString(0), pars.getSize()==1 ? 4 : pars.getInt(1));
}

//
// A_FlashScreen
//

void A_FlashScreen::execute(const ActionData &ad) const
{
    shared_ptr<Creature> actor = ad.getActor();

    // Currently, the rule is that the screen can only be flashed
    // by a Knight. Otherwise, zombies walking over pentagrams will
    // set off screen flashes, which is distracting.
    if (dynamic_cast<Knight*>(actor.get())) {
        Mediator::instance().flashScreen(actor, delay);
    }
}

A_FlashScreen::Maker A_FlashScreen::Maker::register_me;

LegacyAction * A_FlashScreen::Maker::make(ActionPars &pars) const
{
    pars.require(0,1);
    int delay = 0;
    if (pars.getSize() == 1) {
        delay = pars.getInt(0);
    }
    return new A_FlashScreen(delay);
}


//
// A_FullZombieActivity
//

void A_FullZombieActivity::execute(const ActionData &ad) const
{
    Mediator::instance().getMonsterManager().fullZombieActivity();
}

A_FullZombieActivity::Maker A_FullZombieActivity::Maker::register_me;

LegacyAction * A_FullZombieActivity::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_FullZombieActivity;
}



//
// A_Necromancy
//

bool A_Necromancy::possible(const ActionData &) const
{
    return !Mediator::instance().getMonsterManager().hasNecromancyBeenDone();
}

bool A_Necromancy::executeWithResult(const ActionData &ad) const
{
    if (possible(ad)) {

        DungeonMap *dmap;
        MapCoord pos;
        ad.getGenericPos(dmap, pos);

        if (dmap) {
            Mediator::instance().getMonsterManager().doNecromancy(nzoms, *dmap,
                    pos.getX() - range,     pos.getY() - range,
                    pos.getX() + range + 1, pos.getY() + range + 1);
            return true;
        }
    }

    return false;
}

A_Necromancy::Maker A_Necromancy::Maker::register_me;

LegacyAction * A_Necromancy::Maker::make(ActionPars &pars) const
{
    pars.require(2);
    return new A_Necromancy(pars.getInt(0), pars.getInt(1));
}


//
// A_Nop
//

void A_Nop::execute(const ActionData &) const
{
}

A_Nop::Maker A_Nop::Maker::register_me;

LegacyAction * A_Nop::Maker::make(ActionPars &) const
{
    return new A_Nop;
}


//
// A_NormalZombieActivity
//

void A_NormalZombieActivity::execute(const ActionData &ad) const
{
    Mediator::instance().getMonsterManager().normalZombieActivity();
}

A_NormalZombieActivity::Maker A_NormalZombieActivity::Maker::register_me;

LegacyAction * A_NormalZombieActivity::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_NormalZombieActivity;
}


//
// A_PitKill
// Kill the given creature (w/o blood/gore effects -- just directly remove them from the map).
// Only creatures at height H_WALKING are affected!
//

void A_PitKill::execute(const ActionData &ad) const
{
    shared_ptr<Creature> cr = ad.getActor();
    if (!cr || !cr->getMap()) return;
    if (cr->getHeight() != H_WALKING) return;
    cr->onDeath(Creature::PIT_MODE, ad.getOriginator());
    cr->rmFromMap();
}

A_PitKill::Maker A_PitKill::Maker::register_me;

LegacyAction * A_PitKill::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_PitKill;
}


//
// A_RevealStart
//

void A_RevealStart::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    if (kt) kt->setReveal2(true);
}

A_RevealStart::Maker A_RevealStart::Maker::register_me;

LegacyAction * A_RevealStart::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_RevealStart;
}

//
// A_RevealStop
//

void A_RevealStop::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    if (kt) kt->setReveal2(false);
}

A_RevealStop::Maker A_RevealStop::Maker::register_me;

LegacyAction * A_RevealStop::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_RevealStop;
}


//
// A_Secure
//

bool A_Secure::possible(const ActionData &ad) const
{
    shared_ptr<Creature> cr = ad.getActor();
    const Player * player = cr->getPlayer();
    if (cr && cr->getMap() && player) {
        return Mediator::instance().isSecurableHome(*player, cr->getMap(), cr->getPos(), cr->getFacing());
    } else {
        return false;
    }
}

bool A_Secure::executeWithResult(const ActionData &ad) const
{
    shared_ptr<Creature> cr = ad.getActor();
    Player * player = cr->getPlayer();
    if (cr && cr->getMap() && player) {
        return Mediator::instance().secureHome(*player,
                   *cr->getMap(), cr->getPos(), cr->getFacing(), plain_wall_tile);
    } else {
        return false;
    }
}

A_Secure::Maker A_Secure::Maker::register_me;

LegacyAction * A_Secure::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    shared_ptr<Tile> wall = pars.getTile(0);
    return new A_Secure(wall);
}


//
// A_ZombieKill
//

bool A_ZombieKill::possible(const ActionData &ad) const
{
    // See if the victim is a monster
    shared_ptr<Monster> mon = dynamic_pointer_cast<Monster>(ad.getVictim());
    if (mon) {
        // See if it has the correct type
        if (&mon->getMonsterType() == &zom_type) {
            return true;
        }
    }

    // otherwise the ZombieKill won't work
    return false;
}

bool A_ZombieKill::executeWithResult(const ActionData &ad) const
{
    // Kills the *victim* if it is a zombie (Used as a melee_action)
    shared_ptr<Monster> cr = dynamic_pointer_cast<Monster>(ad.getVictim());
    if (cr && cr->getMap() && &cr->getMonsterType() == &zom_type) {
        cr->onDeath(Creature::NORMAL_MODE, ad.getOriginator());
        cr->rmFromMap();
        return true;
    } else {
        return false;
    }
}

A_ZombieKill::Maker A_ZombieKill::Maker::register_me;

LegacyAction * A_ZombieKill::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    const MonsterType * mtype = pars.getMonsterType(0);
    if (!mtype) {
        pars.error();
        return 0;
    } else {
        return new A_ZombieKill(*mtype);
    }
}

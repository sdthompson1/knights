/*
 * script_actions.cpp
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

#include "random_int.hpp"
using namespace KConfig;

#include <iostream> // used for A_DebugPrint
using namespace std;

#ifdef _MSC_VER
    // fix "bug" with MSVC static libraries and global constructors
    extern "C" void InitScriptActions()
    {
    }
#endif



//
// A_AddTile
//

void A_AddTile::execute(const ActionData &ad) const
{
    // We look for a map coord from the Tile parameter.
    // (The usual use case is that this is called from a tile's on_destroy action.)
    DungeonMap *dmap;
    MapCoord mc;
    shared_ptr<Tile> dummy;
    ad.getTile(dmap, mc, dummy);
    if (!dmap) return;
    dmap->addTile(mc, tile->clone(false), ad.getOriginator());
}

A_AddTile::Maker A_AddTile::Maker::register_me;

Action * A_AddTile::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    return new A_AddTile(pars.getTile(0));
}


//
// A_After
//

namespace {
    class RunAfterAction : public Task {
    public:
        RunAfterAction(const Action *a, const ActionData &d) : action(a), data(d) { }
        virtual void execute(TaskManager &) {
            action->execute(data);
        }
    private:
        const Action *action;
        ActionData data;
    };
}

void A_After::execute(const ActionData &ad) const
{
    // We copy the ActionData and keep it around until after the time delay runs out.
    // This does mean that After() should be used with care. E.g. the actor is "locked"
    // for the duration of the time delay, and if he dies he won't be able to respawn
    // until the time delay runs out. This means that After should not generally be used
    // with long time delays.
    shared_ptr<Task> t(new RunAfterAction(action, ad));

    TaskManager &tm(Mediator::instance().getTaskManager());
    tm.addTask(t, TP_NORMAL, tm.getGVT() + delay);

    // We will also set up After to stun the actor until the after-action
    // runs. This is the semantics required by potion drinking, after all.
    // (A possible future extension is to provide a flag saying whether to
    // stun or not... or could just provide a separate A_Stun.)
    if (ad.getActor()) ad.getActor()->stunUntil(tm.getGVT() + delay);
}

A_After::Maker A_After::Maker::register_me;

Action * A_After::Maker::make(ActionPars &pars) const
{
    pars.require(2);
    return new A_After(pars.getInt(0), pars.getAction(1));
}


//
// A_ChangeItem
//

void A_ChangeItem::execute(const ActionData &ad) const
{
    DungeonMap *dmap;
    MapCoord mc;
    const ItemType *old_item;
    ad.getItem(dmap, mc, old_item);
    if (!dmap || mc.isNull() || !old_item || !item_type) return;
    dmap->rmItem(mc);
    shared_ptr<Item> new_item(new Item(*item_type));
    dmap->addItem(mc, new_item);
}

A_ChangeItem::Maker A_ChangeItem::Maker::register_me;

Action * A_ChangeItem::Maker::make(ActionPars &pars) const
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

Action * A_ChangeTile::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    return new A_ChangeTile(pars.getTile(0));
}


//
// A_CheckQuest
//

void A_CheckQuest::execute(const ActionData &ad) const
{
    shared_ptr<Knight> kt = dynamic_pointer_cast<Knight>(ad.getActor());
    if (kt) {
        vector<string> dummy;
        if (kt->getMap() && kt->getPlayer()->checkQuests(dummy)) {
            // Player has won the game
            Mediator::instance().updateQuestIcons(*kt->getPlayer(), WIN_FROM_COMPLETE_QUEST);
            Mediator::instance().winGame(*kt->getPlayer());
        }
    }
}

A_CheckQuest::Maker A_CheckQuest::Maker::register_me;

Action * A_CheckQuest::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_CheckQuest;
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

Action * A_CrystalStart::Maker::make(ActionPars &pars) const
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

Action * A_CrystalStop::Maker::make(ActionPars &pars) const
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
        const int amt = amount? amount->get() : 0;
        const int stun = stun_time? stun_time->get() : 0;
        cr->damage(amt, ad.getOriginator(), Mediator::instance().getGVT() + stun, inhibit_squelch);
    }
}

A_Damage::Maker A_Damage::Maker::register_me;

Action * A_Damage::Maker::make(ActionPars &pars) const
{
    pars.require(2, 3);
    bool inhibit_squelch = (pars.getSize() == 3 && pars.getInt(2));
    return new A_Damage(pars.getRandomInt(0), pars.getRandomInt(1), inhibit_squelch);
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

Action * A_DebugPrint::Maker::make(ActionPars &pars) const
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

Action * A_FlashMessage::Maker::make(ActionPars &pars) const
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

Action * A_FlashScreen::Maker::make(ActionPars &pars) const
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

Action * A_FullZombieActivity::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_FullZombieActivity;
}


//
// A_IfKnight
//

void A_IfKnight::execute(const ActionData &ad) const
{
    shared_ptr<Knight> knight = dynamic_pointer_cast<Knight>(ad.getActor());
    if (knight && action) {
        action->execute(ad);
    }
}

A_IfKnight::Maker A_IfKnight::Maker::register_me;

Action * A_IfKnight::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    return new A_IfKnight(pars.getAction(0));
}


//
// A_Necromancy
//

bool A_Necromancy::possible(const ActionData &) const
{
    return !Mediator::instance().getMonsterManager().hasNecromancyBeenDone();
}

void A_Necromancy::execute(const ActionData &ad) const
{
    if (possible(ad)) {
        pair<DungeonMap *,MapCoord> p = ad.getPos();
        if (p.first && !p.second.isNull()) {
            Mediator::instance().getMonsterManager().doNecromancy(nzoms, *p.first,
                    p.second.getX() - range, p.second.getY() - range,
                    p.second.getX() + range + 1, p.second.getY() + range + 1);
        }
    }
}

A_Necromancy::Maker A_Necromancy::Maker::register_me;

Action * A_Necromancy::Maker::make(ActionPars &pars) const
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

Action * A_Nop::Maker::make(ActionPars &) const
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

Action * A_NormalZombieActivity::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_NormalZombieActivity;
}


//
// A_OnSuccess
//

void A_OnSuccess::execute(const ActionData &ad) const
{
    if (ad.getSuccess()) {
        if (success_action) success_action->execute(ad);
    } else {
        if (fail_action) fail_action->execute(ad);
    }
}

A_OnSuccess::Maker A_OnSuccess::Maker::register_me;

Action * A_OnSuccess::Maker::make(ActionPars &pars) const
{
    pars.require(1,2);
    return new A_OnSuccess(pars.getAction(0),
                           pars.getSize()==2 ? pars.getAction(1) : 0);
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

Action * A_PitKill::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_PitKill;
}


//
// A_PlaySound
//

void A_PlaySound::execute(const ActionData &ad) const
{
    if (!frequency) return;
    DungeonMap *dmap = 0;
    MapCoord mc;
    shared_ptr<Creature> actor = ad.getActor();
    if (actor && actor->getMap()) {
        dmap = actor->getMap();
        mc = actor->getPos();
    } else {
        shared_ptr<Tile> dummy;
        ad.getTile(dmap, mc, dummy);
    }

    // "Lua pos" will override the pos from above
    // Fixes: #100 (Door sound not heard when the door is opened by someone in the room outside)
    if (!ad.getLuaPos().isNull()) {
        mc = ad.getLuaPos();
    }

    if (dmap) {
        Mediator::instance().playSound(*dmap, mc, *sound, frequency->get(), all);
    }
}

A_PlaySound::Maker A_PlaySound::Maker::register_me;

Action * A_PlaySound::Maker::make(ActionPars &pars) const
{
    pars.require(2, 3);
    if (pars.getSize() == 2) {
        return new A_PlaySound(pars.getSound(0), pars.getRandomInt(1), false);
    } else {
        return new A_PlaySound(pars.getSound(0), pars.getRandomInt(1), pars.getInt(2) != 0);
    }
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

Action * A_RevealStart::Maker::make(ActionPars &pars) const
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

Action * A_RevealStop::Maker::make(ActionPars &pars) const
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
        return Mediator::instance().isSecurableHome(*player, cr->getPos(), cr->getFacing());
    } else {
        return false;
    }
}

void A_Secure::execute(const ActionData &ad) const
{
    shared_ptr<Creature> cr = ad.getActor();
    const Player * player = cr->getPlayer();
    if (cr && cr->getMap() && player) {
        Mediator::instance().secureHome(*player,
                *cr->getMap(), cr->getPos(), cr->getFacing(), plain_wall_tile);
    }
}

A_Secure::Maker A_Secure::Maker::register_me;

Action * A_Secure::Maker::make(ActionPars &pars) const
{
    pars.require(1);
    shared_ptr<Tile> wall = pars.getTile(0);
    return new A_Secure(wall);
}
    

//
// A_Shoot
//

void A_Shoot::execute(const ActionData &ad) const
{
    DungeonMap *dm;
    MapCoord mc;
    shared_ptr<Tile> t;
    ad.getTile(dm, mc, t);
    if (dm) {
        mc.setX(mc.getX() + dx);
        mc.setY(mc.getY() + dy);
        CreateMissile(*dm, mc, dir, itype, false, false, ad.getOriginator(), true);
        Mediator::instance().runHook("HOOK_SHOOT", dm, mc);
    }
}

A_Shoot::Maker A_Shoot::Maker::register_me;

Action * A_Shoot::Maker::make(ActionPars &pars) const
{
    pars.require(4);
    int dx = pars.getInt(0);
    int dy = pars.getInt(1);
    MapDirection dir = pars.getMapDirection(2);
    const ItemType *itype = pars.getItemType(3);
    if (itype) return new A_Shoot(dx, dy, dir, *itype);
    else {
        pars.error();
        return 0;
    }
}

//
// A_TeleportTo
//

void A_TeleportTo::execute(const ActionData &ad) const
{
    // Only Knights are teleported. (We don't want zombies teleported by pentagrams)
    shared_ptr<Creature> cr(ad.getActor());
    if (cr && cr->getMap() && dynamic_cast<Knight*>(cr.get())) {
        TeleportToSquare(cr, *cr->getMap(),
                         MapCoord(cr->getPos().getX() + dx, cr->getPos().getY() + dy));
    }
}

A_TeleportTo::Maker A_TeleportTo::Maker::register_me;

Action * A_TeleportTo::Maker::make(ActionPars &pars) const
{
    pars.require(2);
    return new A_TeleportTo(pars.getInt(0), pars.getInt(1));
}

//
// A_UpdateQuestStatus
//

void A_UpdateQuestStatus::execute(const ActionData &ad) const
{
    shared_ptr<Creature> cr(ad.getActor());
    Knight *kt = dynamic_cast<Knight*>(cr.get());
    if (kt) {
        // Send updated quest icons.
        Mediator::instance().updateQuestIcons(*kt->getPlayer(), JUST_AN_UPDATE);
    }
}

A_UpdateQuestStatus::Maker A_UpdateQuestStatus::Maker::register_me;

Action * A_UpdateQuestStatus::Maker::make(ActionPars &pars) const
{
    pars.require(0);
    return new A_UpdateQuestStatus;
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

void A_ZombieKill::execute(const ActionData &ad) const
{
    // Kills the *victim* if it is a zombie (Used as a melee_action)
    shared_ptr<Monster> cr = dynamic_pointer_cast<Monster>(ad.getVictim());
    if (cr && cr->getMap() && &cr->getMonsterType() == &zom_type) {
        cr->onDeath(Creature::NORMAL_MODE, ad.getOriginator());
        cr->rmFromMap();
    }
}

A_ZombieKill::Maker A_ZombieKill::Maker::register_me;

Action * A_ZombieKill::Maker::make(ActionPars &pars) const
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

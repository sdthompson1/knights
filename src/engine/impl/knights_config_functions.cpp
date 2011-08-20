/*
 * knights_config_functions.cpp
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

#include "item_type.hpp"
#include "knights_config_functions.hpp"
#include "knights_config_impl.hpp"

#include "lua.hpp"

void PopKFileItemType(KnightsConfigImpl *kcfg, KFile *kf, ItemType *it)
{
    KConfig::RandomIntContainer &random_ints = kcfg->getRandomIntContainer();

    KFile::Table tab(*kf, "ItemType");
    
    tab.push("allow_strength");
    bool allow_strength = kcfg->popBool(true);
            
    tab.push("ammo");
    const ItemType *ammo = kcfg->popItemType(0);
            
    tab.push("backpack_graphic");
    Graphic *backpack_graphic = kcfg->popGraphic(0);
            
    tab.push("backpack_overdraw");
    Graphic *backpack_overdraw = kcfg->popGraphic(0);
        
    tab.push("backpack_slot");
    int backpack_slot = kf->popInt(0);

    tab.push("can_throw");
    bool can_throw = kcfg->popBool(false);

    tab.push("control");
    Control *control = kcfg->popControl(0);

    tab.push("editor_label"); kf->pop();
        
    tab.push("fragile");
    bool fragile = kcfg->popBool(false);

    tab.push("graphic");
    Graphic *graphic = kcfg->popGraphic(0);
        
    tab.push("key");
    int key = kf->popInt(0);
        
    tab.push("max_stack");
    int max_stack = kf->popInt(1);


    // Melee properties
    tab.push("melee_action");
    const Action *melee_action = kcfg->popAction(0);
        
    bool any = false, all = true;
    tab.push("melee_backswing_time");
    any |= (!kf->isNone()); all &= (!kf->isNone());
    int melee_backswing_time = kf->popInt(0);

    tab.push("melee_damage");
    any |= (!kf->isNone()); all &= (!kf->isNone());
    const RandomInt * melee_damage = kf->popRandomInt(random_ints, 0);
        
    tab.push("melee_downswing_time");
    any |= (!kf->isNone()); all &= (!kf->isNone());     
    int melee_downswing_time = kf->popInt(0);
        
    tab.push("melee_stun_time");
    any |= (!kf->isNone()); all &= (!kf->isNone());     
    const RandomInt * melee_stun_time = kf->popRandomInt(random_ints, 0);
        
    tab.push("melee_tile_damage");
    any |= (!kf->isNone()); all &= (!kf->isNone());     
    const RandomInt * melee_tile_damage = kf->popRandomInt(random_ints, 0);

    if (any && !all) {
        kf->error("not all melee properties were given");
    }


    // Missile properties
    any = false; all = true;
    if (can_throw) any = true;
        
    tab.push("missile_access_chance");
    any |= (!kf->isNone()); all &= (!kf->isNone());     
    int missile_acc_ch = kcfg->popProbability(0);
        
    tab.push("missile_anim");
    any |= (!kf->isNone()); all &= (!kf->isNone());     
    const Anim * missile_anim = kcfg->popAnim((Anim*)0);
        
    tab.push("missile_backswing_time");
    if (can_throw) { any |= (!kf->isNone()); all &= (!kf->isNone()); }
    int missile_backswing_time = kf->popInt(0);
        
    tab.push("missile_damage");
    any |= (!kf->isNone()); all &= (!kf->isNone());
    const RandomInt *missile_damage = kf->popRandomInt(random_ints, 0);
        
    tab.push("missile_downswing_time");
    if (can_throw) { any |= (!kf->isNone()); all &= (!kf->isNone()); }
    int missile_downswing_time = kf->popInt(0);
        
    tab.push("missile_hit_multiplier");
    any |= (!kf->isNone()); all &= (!kf->isNone());     
    int missile_hit_multiplier = kf->popInt(1);
        
    tab.push("missile_range");
    any |= (!kf->isNone()); all &= (!kf->isNone());     
    int missile_range = kf->popInt(0);
        
    tab.push("missile_speed");
    any |= (!kf->isNone()); all &= (!kf->isNone());     
    int missile_speed = kf->popInt(0);
        
    tab.push("missile_stun_time");
    any |= (!kf->isNone()); all &= (!kf->isNone());     
    const RandomInt * missile_stun_time = kf->popRandomInt(random_ints, 0);

    if (any && !all) {
        kf->error("not all missile properties were given");
    }


    tab.push("name");
    const std::string name = kf->popString("");
        
        
    tab.push("on_drop");
    Action *on_drop = kcfg->popAction(0);
    tab.push("on_hit");
    Action *on_hit = kcfg->popAction(0);
    tab.push("on_pick_up");
    Action *on_pick_up = kcfg->popAction(0);
    tab.push("on_walk_over");
    Action *on_walk_over = kcfg->popAction(0);
        
    tab.push("open_traps");
    bool open_traps = kcfg->popBool(false);

    tab.push("overlay");
    Overlay *overlay = kcfg->popOverlay(0);
        
    tab.push("parry_chance");
    int parry_chance = kcfg->popProbability(0);

    tab.push("reload_action");
    const Action * reload_action = kcfg->popAction(0);

    tab.push("reload_action_time");
    int reload_action_time = kf->popInt(250);
        
    tab.push("reload_time");
    int reload_time = kf->popInt(0);
        
    tab.push("stack_graphic");
    Graphic *stack_graphic = kcfg->popGraphic(graphic);

    tab.push("tutorial");
    const int tutorial_key = kf->popInt(0);
        
    tab.push("type");
    ItemSize item_size = kcfg->popItemSize(IS_NOPICKUP);
        
    it->construct(graphic, stack_graphic, backpack_graphic, backpack_overdraw, overlay, item_size, max_stack,
                  backpack_slot, fragile,
                  melee_backswing_time, melee_downswing_time, melee_damage,
                  melee_stun_time, melee_tile_damage, melee_action, parry_chance, 
                  can_throw, missile_range, missile_speed, missile_acc_ch,
                  missile_hit_multiplier, missile_backswing_time, missile_downswing_time,
                  missile_damage, missile_stun_time, missile_anim,
                  reload_time, ammo, reload_action, reload_action_time,
                  key, open_traps, control, on_pick_up, on_drop, on_walk_over, on_hit,
                  allow_strength, tutorial_key, name);
}
